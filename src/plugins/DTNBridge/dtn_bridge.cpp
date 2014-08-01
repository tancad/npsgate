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
**  @file dtn_bridge.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h> 
#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "dtn_bridge.h"
#include "npsgate_lwip.h"

#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "lwip/stats.h"

using namespace Crafter;
using namespace NpsGate;

/*static string addr_to_str(uint32_t addr) { 
	char buffer[16];
	sprintf(buffer, "%hhu.%hhu.%hhu.%hhu", (addr>>24), (addr>>16), (addr>>8), addr);
	return string(buffer);
}*/

static uint32_t str_to_addr(const string& str) {
	uint8_t a, b, c, d;
	sscanf(str.c_str(), "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d);
	return (a<<24) + (b<<16) + (c<<8) + d;
}

DTNBridge::DTNBridge(PluginCore* c) : NpsGatePlugin(c) {
	lwip = new NpsGateLWIP(this, 1500);
}

DTNBridge::~DTNBridge() {
	delete lwip;
}

bool DTNBridge::init() {
	string dtn_subnet;

	set_timeout(250);

	const Config* config = get_config();

	if(!config->lookupValue("dtnbridge.dtn_subnet", dtn_subnet)) {
		LOG_CRITICAL("Missing 'dtn_subnet' configuration directive.\n");
	}

	dtn_path = "DTNOutput";
	dtn_network = LWIPSocket::str_to_addr(dtn_subnet);
	dtn_netmask = LWIPSocket::prefix_to_netmask(24);

	return true;
}

bool DTNBridge::send_data(uint8_t* data, int len) {
	Packet* p = new Packet();

	p->PacketFromIP((byte*)data, len);

	forward_packet(get_default_output(), p);

	return true;
}

bool DTNBridge::process_packet(Packet* p) {
	IP* ip = p->GetLayer<IP>();
	string dstaddr = ip->GetDestinationIP();

	uint32_t daddr = LWIPSocket::str_to_addr(dstaddr);


	daddr &= dtn_netmask;

	if(daddr == dtn_network) {
		process_ip_packet(p);
	} else {
		process_dtn_packet(p);
	}

	drop_packet(p);
	return true;
}

bool DTNBridge::process_dtn_packet(Packet* p) {
	IP* ip = p->GetLayer<IP>();
	TCP* tcp = p->GetLayer<TCP>();
	if(!ip || !tcp) {
		LOG_WARNING("Packet is not a TCP/IP packet. Dropping!\n");
		return false;
	}

	uint32_t saddr = str_to_addr(ip->GetSourceIP());
	uint32_t daddr = str_to_addr(ip->GetDestinationIP());
	uint16_t sport = tcp->GetSrcPort();
	uint16_t dport = tcp->GetDstPort();
	uint16_t flags = tcp->GetFlags();

	LWIPSocket* s = lwip->find_socket(saddr, daddr, sport, dport);

	/* If we get a SYN, if there is an existing connection, drop. Otherwise,
	   create a new connection. */
	if(flags == TCP::SYN && s) {
		LOG_WARNING("Received a SYN for an existing connection. Dropping.\n");
		return false;
	} else if(flags == TCP::SYN) {
		lwip->connect(saddr, daddr, sport, dport);
		return true;
	}

	/* Should not be receiving SYN/ACKs over DTN */
	if(flags == (TCP::SYN | TCP::ACK)) {
		LOG_WARNING("Should not receive a SYN/ACK over DTN.\n");
		return false;
	}

	/* Handle packets not associated with a connection */
	if(!s) {
		LOG_WARNING("Received a packet not associated with a socket. Dropping.\n");
		return false;
	}

	if(flags & TCP::FIN) {
		s->close();
		return true;
	}

	RawLayer* data = p->GetLayer<RawLayer>();
	if(data) {
		s->send((uint8_t*)data->GetPayload().GetRawPointer(), data->GetSize());
		return true;
	}

	//LOG_WARNING("No data in this packet!\n");

	return false;
}

bool DTNBridge::process_ip_packet(Packet* p) {
	IP* ip = p->GetLayer<IP>();
	TCP* tcp = p->GetLayer<TCP>();
	if(!ip || !tcp) {
		LOG_WARNING("Packet is not a TCP/IP packet. Dropping!\n");
		return false;
	}

	//uint32_t saddr = str_to_addr(ip->GetSourceIP());
	uint32_t daddr = str_to_addr(ip->GetDestinationIP());
	//uint16_t sport = tcp->GetSrcPort();
	uint16_t dport = tcp->GetDstPort();
	uint16_t flags = tcp->GetFlags();

	/* If we get a SYN, first make sure LWIP is listening */
	if(flags == TCP::SYN) {
		lwip->listen(daddr, dport);
	}

	/* We always send the packet to LWIP */
	lwip->inject_packet(p);


	/* All packets with flags set (other than ACK) get sent regardless */
	if(flags & (~TCP::ACK)) {
		//LOG_INFO("Got packet with flags, sending over DTN.\n");
		forward_packet(dtn_path, p);
		return true;
	}

	/* Only data packets get sent over DTN */
	RawLayer* data = p->GetLayer<RawLayer>();
	if(data) {
		//LOG_INFO("Data in this packet, sending over DTN.\n");
		forward_packet(dtn_path, p);
		return true;
	}

	//LOG_INFO("No data in this packet, not sending over DTN.\n");

	return true;
}

bool DTNBridge::process_message(Message* m) {
	return true;
}

bool DTNBridge::message_timeout() {
	lwip->timeout();

	return true;
}

bool DTNBridge::main() {
	message_loop();
	return true;
}


NPSGATE_PLUGIN_CREATE(DTNBridge);
NPSGATE_PLUGIN_DESTROY();

