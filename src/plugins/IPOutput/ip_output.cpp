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
**  @file ip_output.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include "../npsgate_plugin.hpp"
#include "../logger.hpp"

using namespace Crafter;
using namespace NpsGate;


class IPOutput : public NpsGatePlugin {
public:
	IPOutput(PluginCore* c) : NpsGatePlugin(c) {
	}

	virtual bool init() {
		config = get_config();
		LOG_INFO("IPOutput plugin starting intialization.\n");

		if(!config) {
			LOG_CRITICAL("Error accessing config file!\n");
			return false;
		}

		if(!config->lookupValue("ipoutput.output_dev", output_dev)) {
			LOG_CRITICAL("Output device not specified!\n");
			return false;
		}

		config->lookupValue("ipoutput.src_mac", src_mac);
		config->lookupValue("ipoutput.dest_mac", dest_mac);


		LOG_INFO("IPOutput device is '%s'\n", output_dev.c_str());

		raw_socket = create_raw_socket(output_dev);

	//	NpsGateVar* link_up = new NpsGateVar();
	//	link_up->set(true);
	//	publish("IPInput.link_up", link_up);


		return true;
	}

	virtual bool process_packet(Packet* p) {
		bool rval = true;

		Ethernet* eth = p->GetLayer<Ethernet>();
		IP* ip = p->GetLayer<IP>();
		TCP* tcp = p->GetLayer<TCP>();


		if(eth) {
			LOG_TRACE("Received Ethernet packet. Ethertype: 0x%x\n", eth->GetType()); 
//			LOG_TRACE("Stripping Ethernet layer. Count: %d\n", p->GetLayerCount());
//			Packet p2 = p->SubPacket(1, p->GetLayerCount());
		}

		if(!ip) {
			LOG_WARNING("Received a non-IP packet. Dropping packet.\n");
			drop_packet(p);
			return true;
		}

		if(tcp) {
			LOG_TRACE("Sending packet: %u bytes (%u payload)\n", p->GetSize(), tcp->GetPayloadSize());
			LOG_INFO("%s:%u => %s:%u\n", ip->GetSourceIP().c_str(), tcp->GetSrcPort(),
					ip->GetDestinationIP().c_str(), tcp->GetDstPort());
		}

		if(!p->SocketSend(raw_socket)) {
		//if(!p->Send(output_dev)) {
			LOG_WARNING("Failed to send packet on interface: %s. Packet will be dropped!\n",
					output_dev.c_str());
			rval = false;
		}

		drop_packet(p);

		return rval;
	}

	virtual bool process_message(Message* m) {
		return true;
	}

	bool main() {
		message_loop();
		return true;
	}

private:
	const Config* config;
	string	output_dev;
	string src_mac;
	string dest_mac;
	int raw_socket;

	int create_raw_socket(string device) {
		int s;
		int val = 1;

		s = socket(AF_INET, SOCK_RAW, 4);
//		setsockopt(s, SOL_SOCKET, SO_DONTROUTE, &val, sizeof(val));
		setsockopt(s, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));
//		setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, device.c_str(), device.length());
		if(s == -1) {
			LOG_CRITICAL("Failed to create raw socket. Reason: %s\n", strerror(errno));
		}

		return s;
	}

};

NPSGATE_PLUGIN_CREATE(IPOutput);
NPSGATE_PLUGIN_DESTROY();
