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
**  @file pcapinput.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h>
#include <signal.h>

#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "../../plugincore.h"

using namespace Crafter;
using namespace NpsGate;

class PCAPInput : public NpsGatePlugin {

public:

	PCAPInput(PluginCore* c) : NpsGatePlugin(c) {
		packet_count = 0;
		kill_on_eof = false;
		real_time = true;
		drift_time = 150;
	}

	bool init() {
		LOG_INFO("PCAPInput plugin starting intialization.\n");
		config = get_config();

		if(!config->lookupValue("pcapinput.input_file", input_file)) {
			LOG_CRITICAL("Input file not specified in configuration file!\n");
			return false;
		}

		config->lookupValue("pcapinput.kill_on_eof", kill_on_eof);
		config->lookupValue("pcapinput.real_time", real_time);
		config->lookupValue("pcapinput.drift_time", drift_time);

		OpenOffPcap(&link_layer_type, pcap_handle, "test.pcap", filter_string);

		return true;
	}

	bool process_packet(Packet* p) {
		Packet* old_pkt;
		LOG_DEBUG("Received a packet of length: %d\n", p->GetSize());

		Ethernet* eth = p->GetLayer<Ethernet>();
		if(eth) {
			LOG_DEBUG("Read Ethernet header, stripping.\n");
			old_pkt = p;
			p = new Packet();
			*p = old_pkt->SubPacket(1, old_pkt->GetLayerCount());
			delete old_pkt;
		}

		forward_packet(get_default_output(), p);

//		NpsGateVar* var1 = new NpsGateVar();
//		var1->set_int(p->GetSize());
//		publish("pcapinput.packet_size", var1);

//		var1->unref();

//		NpsGateVar* var2 = new NpsGateVar();
//		var2->set_int(++packet_count);
//		publish("pcapinput.packet_count", var2);

		return true;
	}

	bool main() {
		struct timeval last_tv, tv;
		uint32_t last_sec = 0, last_usec = 0;
		int32_t delay_sec, delay_usec;
		pcap_pkthdr* hdr;
		const u_char* data;

		while(1 == pcap_next_ex(pcap_handle, &hdr, &data)) {
			// Create a new Crafter packet from the PCAP data
			Packet* pkt = new Packet();
			pkt->PacketFromLinkLayer(data, hdr->len, link_layer_type);

			gettimeofday(&tv, NULL);

			/* Use realistic delays between packets */
			if(real_time && last_sec != 0 && last_usec != 0) {
				/* Calculate the time difference between this packet and the last packet */
				delay_sec = hdr->ts.tv_sec - last_sec;
				delay_usec = hdr->ts.tv_usec - last_usec;

				/* Get the current time, and subtract the actual time between processing
				   the last packet */
				gettimeofday(&tv, NULL);
				delay_sec -= (tv.tv_sec - last_tv.tv_sec);
				delay_usec -= (tv.tv_usec - last_tv.tv_usec) + 150;
				if(delay_usec < 0 && delay_sec > 0) {
					delay_sec--;
					delay_usec = tv.tv_usec + 1000000 - last_tv.tv_usec;
				}

				/* Sleep the necessary amount of time */
				if(delay_sec > 0) {
					sleep(delay_sec);
				}
				if(delay_usec > 0) {
					usleep(delay_usec);
				}
			}
			last_sec = hdr->ts.tv_sec;
			last_usec = hdr->ts.tv_usec;
			last_tv.tv_sec = tv.tv_sec;
			last_tv.tv_usec = tv.tv_usec;

			process_packet(pkt);
		}

		LOG_INFO("PCAPInput plugin finished reading input file. Terminating thread.\n");
		if(kill_on_eof) {
			LOG_INFO("kill_on_eof is set! PCAPInput sending SIGUSR1.\n");
			kill(getpid(), SIGUSR1);
		}
		return true;
	}

private:
	int		link_layer_type;
	pcap_t*	pcap_handle;
	string	filter_string;
	string  input_file;
	const Config* config;
	bool kill_on_eof;
	bool real_time;
	int drift_time;

	int packet_count;

};

NPSGATE_PLUGIN_CREATE(PCAPInput);
NPSGATE_PLUGIN_DESTROY();
