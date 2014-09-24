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
**  would appreciate acknowledgement if the software is used.
**
**  @file nothing.cpp
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


class Nothing : public NpsGatePlugin {
public:
	Nothing(PluginCore* c) : NpsGatePlugin(c) { }
	~Nothing() { }

	bool init() {
		return true;
	}

	bool process_packet(Packet* p) {
		forward_packet(get_default_output(), p);
		return true;
	}

	bool process_message(Message* m) {
		return true;
	}

	bool main() {
		message_loop();
		return true;
	}

};

NPSGATE_PLUGIN_CREATE(Nothing);
NPSGATE_PLUGIN_DESTROY();
