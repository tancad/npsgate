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
**  @file pcapoutput.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h>

#include "../npsgate_plugin.hpp"
#include "../logger.hpp"

using namespace Crafter;
using namespace NpsGate;


class PCAPOutput : public NpsGatePlugin {
public:
	PCAPOutput(PluginCore* c) : NpsGatePlugin(c) {
		packet_count = 0;
	}

	~PCAPOutput() {
		ClosePcapDumper(pcap_handle, pcap_dumper);
	}

	virtual bool init() {
		LOG_INFO("PCAPOutput plugin starting intialization.\n");

		config = get_config();

		/* Get the output file name from the config. Fail if not set. */
		if(!config->lookupValue("pcapoutput.output_file", output_file)) {
			LOG_CRITICAL("'output_file' directive not set in config file!\n");
			return false;
		}

		// Assume that the data link layer is Ethernet
		OpenPcapDumper(DLT_EN10MB, output_file, pcap_handle, pcap_dumper);

		if(pcap_handle == NULL || pcap_dumper == NULL) {
			LOG_CRITICAL("Failed to open PCAP dumper!\n");
			return false;
		}

		return true;
	}

	virtual bool process_packet(Packet* p) {
		timeval tv;
		pcap_pkthdr header;
		LOG_DEBUG("Received a packet of length: %d\n", p->GetSize());

		/* Fill in the PCAP header struct */
		gettimeofday(&tv, NULL);
		header.ts = tv;
		header.caplen = p->GetSize();
		header.len = p->GetSize();

		/* Publish the packet_size */
//		NpsGateVar* var1 = new NpsGateVar();
//		var1->set_int(p->GetSize());
//		publish("pcapoutput.packet_size", var1);

		/* Publish the packet_count */
//		NpsGateVar* var2 = new NpsGateVar();
//		var2->set_int(++packet_count);
//		publish("pcapoutput.packet_count", var2);

		/* Dump the packet to the output, then drop the packet */
		DumperPcap(pcap_dumper, &header, p->GetRawPtr());
		drop_packet(p);

		return true;
	}

	virtual bool process_message(Message* m) {
//		if(m) {
//			LOG_INFO("Received message: fq_name='%s', val='%d'\n",
//					m->fq_name.c_str(), m->value->get_int());
//		}
		return true;
	}

	bool main() {
		LOG_DEBUG("Inside main. Entering message_loop.\n");
		message_loop();
		return true;
	}

private:
	int		link_layer_type;
	pcap_t*	pcap_handle;
	pcap_dumper_t* pcap_dumper;
	string	filter_string;
	string output_file;
	const Config* config;

	int packet_count;
};

NPSGATE_PLUGIN_CREATE(PCAPOutput);
NPSGATE_PLUGIN_DESTROY();
