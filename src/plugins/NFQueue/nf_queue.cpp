/******************************************************************************
**
**  PortRouter plugin.
**  Copyright (c) 2014, Lance Alt
**
**  This file is part of NpsGate.
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published
**  by the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
** 
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU Lesser General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
** 
**
**  @file nf_queue.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/14
**
*******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h>
#include <signal.h>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <libnfnetlink/libnfnetlink.h>

#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "../../plugincore.h"

using namespace Crafter;
using namespace NpsGate;


class NFQueue : public NpsGatePlugin {
private:
	uint32_t queue_num;
	uint32_t queue_len;
	uint32_t mtu;	
	timeval read_timeout;
	vector<string> output_map;
	struct nfq_handle* nf_queue;
	struct nfq_q_handle* nf_input;

	struct NFQueueRule {
		string protocol;
		string source, destination;
		string source_port, destination_port, port;
	};
	vector<NFQueueRule> rules;
public:

	NFQueue(PluginCore* c) : NpsGatePlugin(c), mtu(10000) {
		read_timeout.tv_sec = 10;
		read_timeout.tv_usec = 0;
		queue_num = 0;
		output_map.resize(256);
	}

	~NFQueue() {
		BOOST_FOREACH(const NFQueueRule& r, rules) {
			remove_rule(r);
		}
		if(nf_input) {
			nfq_destroy_queue(nf_input);
		}
		if(nf_queue) {
			nfq_close(nf_queue);
		}
	}

	inline bool add_rule(const NFQueueRule& r) {
		return build_rule("-A", r);
	}

	inline bool remove_rule(const NFQueueRule& r) {
		return build_rule("-D", r);
	}

	bool build_rule(const string& action, const NFQueueRule& r) {
		stringstream call;
		char buffer[256];

		call << "iptables -t mangle -m multiport " << action << " FORWARD";
		if(!r.protocol.empty())			{ call << " -p "		<< r.protocol; }
		if(!r.source.empty())			{ call << " -s "		<< r.source; }
		if(!r.destination.empty())		{ call << " -d "		<< r.destination; }
		if(!r.source_port.empty())		{ call << " --sports "	<< r.source_port; }
		if(!r.destination_port.empty())	{ call << " --dports "	<< r.destination_port; }
		if(!r.port.empty())				{ call << " --ports "	<< r.port; }
		call << " -j NFQUEUE --queue-num " << queue_num << " 2>&1";

		LOG_INFO("Calling iptables: '%s'\n", call.str().c_str());
		FILE* fh = popen(call.str().c_str(), "r");
		if(!fh) {
			LOG_WARNING("Executing iptables command failed.\n");
			return false;
		}

		fgets(buffer, 256, fh);

		if(0 != pclose(fh)) {
			LOG_WARNING("iptables completed unsuccessfully. Response was: %s", buffer);
			return false;
		}

		return true;
	}
	
	bool init() {
		const Config* config = get_config();

		LOG_INFO("NFQueue plugin starting initialization.\n");

		parse_outputs();

		config->lookupValue("nfqueue.queue-number", queue_num);
		config->lookupValue("nfqueue.read-read_timeout", (int&)read_timeout.tv_sec);
		config->lookupValue("nfqueue.mtu", mtu);

		// Open a link to NFQUEUE using the default queue number. Bind to AF_INET, set mode to
		// copy entire packet to user-space and set the callback function.
		nf_queue = nfq_open();
		if(!nf_queue) {
			LOG_CRITICAL("Failed to open NFQUEUE!\n");
		}

		if(nfq_unbind_pf(nf_queue, AF_INET) < 0) {
			LOG_CRITICAL("Failed to unbind from AF_INET!\n");
		}

		if(nfq_bind_pf(nf_queue, AF_INET) < 0) {
			LOG_CRITICAL("Failed to bind to AF_INET!\n");
		}

		nf_input = nfq_create_queue(nf_queue, queue_num, &nf_queue_callback, this);
		if(!nf_input) {
			LOG_CRITICAL("Failed to create NF queue!\n");
		}

		if(nfq_set_mode(nf_input, NFQNL_COPY_PACKET, 0xffff) < 0) {
			LOG_CRITICAL("Failed to set packet copy mode!\n");
		}
		
		if(config->lookupValue("nfqueue.queue-len", queue_len)) {
			LOG_INFO("Setting max queue length to: %u\n", queue_len);
			if(-1 == nfq_set_queue_maxlen(nf_input, queue_len)) {
				LOG_WARNING("Failed to set max queue length to: %u\n", queue_len);
			}
			unsigned int new_size = nfnl_rcvbufsiz(nfq_nfnlh(nf_queue), queue_len * mtu);
			if(new_size != queue_len * mtu) {
				LOG_WARNING("Failed to set recv buffer size of %u. Size is: %u\n",
						queue_len * mtu, new_size);
			}
		}

		const Setting& root = config->getRoot();
		const Setting& capture_rules = root["nfqueue"]["capture"];

	
		LOG_INFO("There are %d capture rules.\n", capture_rules.getLength());
		for(int i = 0; i < capture_rules.getLength(); i++) {
			const Setting& rconf = capture_rules[i];
			NFQueueRule rule;

			rconf.lookupValue("protocol", rule.protocol);
			rconf.lookupValue("source", rule.source);
			rconf.lookupValue("destination", rule.destination);
			rconf.lookupValue("source-port", rule.source_port);
			rconf.lookupValue("destination-port", rule.destination_port);
			rconf.lookupValue("port", rule.port);
			
			if(!add_rule(rule)) {
				LOG_CRITICAL("Failed to add rule. Aborting.\n");
			}
			rules.push_back(rule);
		}

		return true;
	}

	bool process_packet(Packet* p) {
		IP* ip = p->GetLayer<IP>();
		if(!ip) {
			LOG_WARNING("Unable to parse IP header. Dropping packet.\n");
			return false;
		}


		// Check to see if there is a particular plugin we need to send this packet based on its
		// IP protocol field.
		unsigned int ip_prot = ip->GetProtocol();

		if(ip_prot <= output_map.size()) {
			string output = output_map[ip_prot];
			if(!output.empty()) {
				LOG_TRACE("Protocol %u. Forwarding to %s.\n", ip_prot, output.c_str());
				return forward_packet(output, p);
			}
		}

		LOG_TRACE("No output specified for protocol %u. Returning to netfilter queue.\n", ip_prot);
		return false;
	}

	bool main() {
		int rv, fd;
		fd_set fdset;
		char* raw = (char*)alloca(mtu);

		fd = nfq_fd(nf_queue);

		while (true){
			LOG_TRACE("Waiting for packet from nfqueue...\n");

			timeval timeout = read_timeout;
			FD_ZERO(&fdset);
			FD_SET(fd, &fdset);
			rv = select(fd+1, &fdset, NULL, NULL, &timeout);
			if(rv < 0) {
				LOG_WARNING("Select error.\n");
			} else if(rv == 0) {
				LOG_TRACE("Select read_timeout.\n");
			} else {
				if(FD_ISSET(fd, &fdset)) {
					if((rv = recv(fd, raw, mtu, 0)) < 0){
						LOG_WARNING("Error reading data\n");
					}else{
						LOG_DEBUG("Packet received!\n");
						nfq_handle_packet(nf_queue, raw, rv);
					}
				}
			}
		}

		LOG_INFO("recv() Returned: %d\n", rv);

		return true;
	}


	/* Reads the config file and parses all output directives. Each output should specify a protocol
	   and and plugin. All IP packets that have the specified protocol will be sent to the corresponding
	   plugin. Protocol/plugin mappings are stored in the 'output_map' vector. */
	void parse_outputs() {
		try {
			const Config* config = get_config();
			const Setting& root = config->getRoot();
			const Setting& conf_plugins = root["outputs"];
	
			LOG_INFO("Parsing %d output directives:\n", conf_plugins.getLength());
	
			for(int i = 0; i < conf_plugins.getLength(); i++) {
				const Setting& plugin_conf = conf_plugins[i];
				string name;
				int protocol;
		
				if(!(plugin_conf.lookupValue("protocol", protocol))) {
					LOG_WARNING("Output %d is missing 'protocol' key!\n", i);
					continue;
				}
				if(!(plugin_conf.lookupValue("plugin", name))) {
					LOG_WARNING("Output is missing the 'plugin' key! Protocol was: %d\n", protocol);
					continue;
				}

				LOG_INFO("Protocol %d => %s\n", protocol, name.c_str());
				output_map[protocol] = name;
			}
		} catch (SettingNotFoundException& ex) {
			return;
		}
	}

	static int nf_queue_callback(nfq_q_handle* queue, struct nfgenmsg* msg, nfq_data* nf_pkt, void* npsgate_plugin) {
		NpsGatePlugin* plugin = (NpsGatePlugin*)npsgate_plugin;
		uint32_t id = 0;
		uint8_t* pkt_data;
		int pkt_len;

		pkt_len = nfq_get_payload(nf_pkt, &pkt_data);

		struct nfqnl_msg_packet_hdr* ph = nfq_get_msg_packet_hdr(nf_pkt);
		if(ph) {
			id = ntohl(ph->packet_id);
		}

		Packet* pkt = new Packet();
		pkt->PacketFromIP((byte*)pkt_data,pkt_len);

		if(plugin->process_packet(pkt)){
			// Tell Netfilter that we own the packet now.
			//return nfq_set_verdict(queue, id, NF_STOP, 0, NULL);
			return nfq_set_verdict(queue, id, NF_DROP, 0, NULL);
		}else{
			// Tell Netfilter that to process the packet normally.
			delete pkt;
			return nfq_set_verdict(queue, id, NF_ACCEPT, 0, NULL);
		}
	}


};

NPSGATE_PLUGIN_CREATE(NFQueue);
NPSGATE_PLUGIN_DESTROY();
