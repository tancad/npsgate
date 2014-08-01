/******************************************************************************
**
**  This file is part of NpsGate.
**
**  This software was developed at the Naval Postgraduate School by employees
**  of the Federal Government in the course of their official duties. Pursuant
**  to title 17 Section 105 of the United States Code this software is not
**  subject to copyright protection and is in the public domain. NpsGate is an
**  experimental system. The Naval Postgraduate School assumes no responsibility
**  whatsoever for its use by other parties, and makes no guarantees, expressed
**  or implied, about its quality, reliability, or any other characteristic. We
**  would appreciate acknowledgment if the software is used.
**
**  @file nf_queue.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h>
#include <signal.h>
#include <boost/algorithm/string.hpp>

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
	int	m_queue;	// Netfilter queue to create
	uint32_t m_queue_len;
	int 	m_mtu;		// MTU of interface
	timeval m_timeout;	// Timeout for read
	vector<string> m_rules;	// IPTables rules for NFQueue
	vector<string> output_map;
	struct nfq_handle* nf_queue;
	struct nfq_q_handle* nf_input;
	const Config* m_config;

public:

	NFQueue(PluginCore* c) : NpsGatePlugin(c), m_mtu(10000) {
		m_timeout.tv_sec = 10;
		m_queue = 0;
		output_map.resize(256);
	}

	~NFQueue() {
		delRules();
		if(nf_input) {
			nfq_destroy_queue(nf_input);
		}
		if(nf_queue) {
			nfq_close(nf_queue);
		}
	}

	string dec2string(uint32_t dec) {
		char buf[12];
		sprintf(buf, "%d", dec);
		return string(buf);
	}
	
	void addRules() {
		for(vector<string>::const_iterator iter = m_rules.begin(); iter != m_rules.end(); ++iter){
			LOG_INFO("Adding iptables rule for %s.\n", iter->c_str());
			string call = "iptables -t mangle -A FORWARD " + *iter + " -j NFQUEUE --queue-num " + dec2string(m_queue);
			
			if (system(call.c_str()) != 0){
				LOG_WARNING("Iptables add rule error.\n");
			}

		}
	}
	
	void delRules() {
		for(vector<string>::const_iterator iter = m_rules.begin(); iter != m_rules.end(); ++iter){
			 LOG_INFO("Deleting iptables rule for %s\n", iter->c_str());
			string call = "iptables -t mangle -D FORWARD " + *iter + " -j NFQUEUE --queue-num " + dec2string(m_queue);
			
			if (system(call.c_str()) != 0){
				LOG_WARNING("Iptables delete rule error.\n");
			}
		}
	}

	bool init() {

		LOG_INFO("NFQueue plugin starting initialization.\n");
		m_config = get_config();

		parse_outputs();

		m_config->lookupValue("nfqueue.queue-number", m_queue);

		if(!m_config->lookupValue("nfqueue.mtu", m_mtu)) {
			LOG_CRITICAL("Missing TAP interface MTU setting!\n");
		}

		if(!m_config->lookupValue("nfqueue.read-timeout", (int&)m_timeout.tv_sec)) {
			LOG_CRITICAL("Missing TAP interface timeout setting!\n");
		}

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

		nf_input = nfq_create_queue(nf_queue, m_queue, &nf_queue_callback, this);
		if(!nf_input) {
			LOG_CRITICAL("Failed to create NF queue!\n");
		}

		if(nfq_set_mode(nf_input, NFQNL_COPY_PACKET, 0xffff) < 0) {
			LOG_CRITICAL("Failed to set packet copy mode!\n");
		}
		
		if(!m_config->lookupValue("nfqueue.mtu", m_mtu)) {
			m_mtu = 1500;	
		}
	
		if(m_config->lookupValue("nfqueue.queue-len", m_queue_len)) {
			LOG_INFO("Setting max queue length to: %u\n", m_queue_len);
			if(-1 == nfq_set_queue_maxlen(nf_input, m_queue_len)) {
				LOG_WARNING("Failed to set max queue length to: %u\n", m_queue_len);
			}
			unsigned int new_size = nfnl_rcvbufsiz(nfq_nfnlh(nf_queue), m_queue_len * m_mtu);
			if(new_size != m_queue_len * m_mtu) {
				LOG_WARNING("Failed to set recv buffer size of %u. Size is: %u\n",
						m_queue_len * m_mtu, new_size);
			}
		}

	
		try {
			const Setting &subnets = m_config->lookup("nfqueue.subnets");
			int numNets = subnets.getLength();
			LOG_INFO("%d subnet(s) defined.\n", numNets);
			

			// We must copy the device name string because the string class apparently changes
			// the pointer each time c_str is called which will cause a "No such device" error
			// when calling the ioctl().
			//char* dev_name = strdup(m_tun->getName().c_str());

			for (int i=0; i<numNets; i++){
				string cidr = subnets[i];
				string rule = "-d " + cidr;
				m_rules.push_back(rule);
				LOG_INFO("Added %s to rules.\n", rule.c_str());
			}
			addRules();
		} catch(SettingNotFoundException& ex) {
			LOG_INFO("No 'subnets' directive found. No iptables rules will be added.\n");
			return true;
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
			if(output != "") {
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
		char* raw = (char*)alloca(m_mtu);

		fd = nfq_fd(nf_queue);

		while (true){
			LOG_TRACE("Waiting for packet from nfqueue...\n");

			timeval timeout = m_timeout;
			FD_ZERO(&fdset);
			FD_SET(fd, &fdset);
			if (select(fd+1, &fdset, NULL, NULL, &timeout) < 0) LOG_WARNING("Select error.\n");
			if (FD_ISSET(fd, &fdset)) {
				if((rv = recv(fd, raw, m_mtu, 0)) < 0){
					LOG_WARNING("Error reading data\n");
				}else{
					LOG_DEBUG("Packet received!\n");
					nfq_handle_packet(nf_queue, raw, rv);
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
