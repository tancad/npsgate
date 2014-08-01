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
**  @file split_tcp.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h> 
#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "split_tcp.h"
#include "npsgate_lwip.h"

#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "lwip/stats.h"

using namespace Crafter;
using namespace NpsGate;
SplitTCP::SplitTCP(PluginCore* c) : NpsGatePlugin(c) {
	lwip = new NpsGateLWIP(this, 1500);
}

SplitTCP::~SplitTCP() {
	delete lwip;
}

bool SplitTCP::init() {
	set_timeout(250);
	return true;
}

bool SplitTCP::send_data(uint8_t* data, int len) {
	Packet* p = new Packet();

	p->PacketFromIP((byte*)data, len);

	forward_packet(get_default_output(), p);

	return true;
}

bool SplitTCP::process_packet(Packet* p) {
	lwip->inject_packet(p);

	drop_packet(p);
	return true;
}

bool SplitTCP::process_message(Message* m) {



	return true;
}

bool SplitTCP::message_timeout() {
/*	static time_t last_stats = 0;

	
	if(time(NULL) - last_stats > 5) {
		NpsGateVar* var = new NpsGateVar();
		var->set(lwip->stats());

		publish("SplitTCP.stats", var);

		string s = var->get<string>();

		LOG_DEBUG("STATS: %s\n", s.c_str());

		last_stats = time(NULL);
	}*/

	lwip->timeout();

	return true;
}

bool SplitTCP::main() {
	message_loop();
	return true;
}

NPSGATE_PLUGIN_CREATE(SplitTCP);
NPSGATE_PLUGIN_DESTROY();
