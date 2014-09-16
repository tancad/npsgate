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
**  @file dtn_output.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <map>
#include <string>
#include <crafter.h>

#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "bpa_interface.hpp"

using namespace Crafter;
using namespace NpsGate;

class DTNOutput : public NpsGatePlugin {
private:
	const Config* config;
	BpaInterface* m_dtn;
	BpaInterface* m_adm_dtn;
	string m_source_endpoint;
	string m_send_adm_endpoint;
	string m_dest_endpoint;
	string m_multicast_group;
	int m_max_bundle_size;	// Max allowable bundle payload size in bytes
	int m_expiration;	// bundle expiration time [s]
	//int m_max_packet_buffer_time;	// Max allowable time to delay packets for aggregation [ms]
	map<string,stringstream*> m_payload;	// Temporary per-destination storage for payloads
	int delay;
	

public:
	DTNOutput(PluginCore* c) : NpsGatePlugin(c), delay(0) {
	}

	virtual bool init() {
		LOG_INFO("DTNOutput plugin starting initialization.\n");
		
		config = get_config();
		if(!config) {
			LOG_CRITICAL("Error accessing config file!\n");
			return false;
		}

		if(!config->lookupValue("dtnoutput.source-endpoint", m_source_endpoint)) {
			LOG_CRITICAL("Missing 'send-endpoint' configuration setting\n");
		}
		if(!config->lookupValue("dtnoutput.dest-endpoint", m_dest_endpoint)) {
			LOG_CRITICAL("Missing 'recv-endpoint' configuration setting\n");
		}

		config->lookupValue("dtnoutput.delay", delay);

		try{
			//config->lookupValue("dtnoutput.admin-endpoint", m_admin_endpoint);
			config->lookupValue("dtnoutput.multicast-group", m_multicast_group);
			config->lookupValue("dtnoutput.max-bundle-size", m_max_bundle_size);
			config->lookupValue("dtnoutput.default-bundle-exp", m_expiration);
			//config->lookupValue("dtnoutput.max-packet-buffer-time", m_max_packet_buffer_time);
		}
		catch(const SettingNotFoundException &nfex){
			LOG_CRITICAL("Missing BPA interface setting!\n");
			return false;
		}
		//LOG_INFO("DTNOutput src EID is '%s'\n", m_send_endpoint.c_str());
		//m_send_adm_endpoint = m_send_endpoint + "/rx";
		
		//m_dtn = new BpaInterface(m_send_endpoint);
		m_dtn = new BpaInterface(m_source_endpoint, m_dest_endpoint);
		//m_adm_dtn = new BpaInterface(m_send_adm_endpoint);

		return true;
	}
	
	

	virtual bool process_packet(Packet* p) {
		bool rval = true;
		LOG_INFO("DTNOutput processing packet\n");

		m_dtn->send((char*)p->GetRawPtr(), p->GetSize());
		drop_packet(p);

		LOG_WARNING("Delaying: %u\n", delay);
		usleep(delay);


		return rval;
	}

	virtual bool process_message(Message* m) {
		return true;
	}

	bool main() {
		message_loop();
		return true;
	}

};

NPSGATE_PLUGIN_CREATE(DTNOutput);
NPSGATE_PLUGIN_DESTROY();
