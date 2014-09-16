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
**  @file dtn_input.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <vector>
#include <map>
#include <valarray>
#include <iostream>
#include <sstream>
#include <string>
#include <crafter.h>
#include <boost/thread.hpp>
#include <libconfig.h++>

#include "bpa_interface.hpp"
#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "bpa_interface.hpp"

using namespace std;
using namespace Crafter;
using namespace NpsGate;

class DTNInput : public NpsGatePlugin {
	private:
		uint16_t m_mtu;
		uint16_t m_timeout;
		BpaInterface* m_dtn;

		dtn_endpoint_id_t m_ignore_eid;
		map<string,string>* m_table;
		map<string,string>::iterator m_table_it;
		string recv_endpoint;
		string send_endpoint;
		string m_admin_endpoint;
		const Config* config;
	
		string getEIDfromIP(string ip)
		{
			/*vector<string> quads;
			split(quads, ip, is_any_of("."));
			for (int i=0; i<4; i++){
				quads[i] = dec2bin(quads[i]);
			}*/
//			string bin_ip = dec2bin(ip);
			for (int i=31; i>7; i--){
//				m_table_it = m_table->find(bin_ip.substr(0,i));
				if (m_table_it != m_table->end()){
					//cout << "EID match is " << m_table_it->second << endl;
					return m_table_it->second;
				}
			}
			LOG_DEBUG("No EID match found!");
			string broadcast("mcast://");
			broadcast += m_admin_endpoint;
			return broadcast;
		}
 
	public:
		DTNInput(PluginCore* c) : NpsGatePlugin(c) {
			m_timeout = 2;
			m_mtu = 1500;
		}

		bool init() {
			config = get_config();

//			m_dtn = dtn;
//			m_table = table;
//			m_dtn->parse_eid(&m_ignore_eid, ignore);
	
			if(!config->lookupValue("dtninput.recv-endpoint", recv_endpoint)) {
				LOG_CRITICAL("Missing recv-endpoint setting.\n");
			}
			if(!config->lookupValue("dtninput.send-endpoint", send_endpoint)) {
				LOG_CRITICAL("Missing send-endpoint setting.\n");
			}

			m_dtn = new BpaInterface(recv_endpoint, send_endpoint);

			LOG_INFO("Starting BPA Interface.\n");
			LOG_INFO("\tRecv endpoint: %s\n", recv_endpoint.c_str());
			LOG_INFO("\tSend endpoint: %s\n", send_endpoint.c_str());

			//config.lookupValue("max-bundle-size", m_max_bundle_size);
			//config.lookupValue("default-bundle-exp", m_expiration);
			//config.lookupValue("max-packet-buffer-time", m_max_packet_buffer_time);
			return true;
		}
 
		bool process_packet(Packet* p) {
			return forward_packet(get_default_output(), p);
		}

		bool main() {
			int ret;
			dtn_bundle_spec_t bundle_spec;
			dtn_bundle_payload_t payload;
			byte* raw = (byte *)malloc(sizeof(byte) * m_mtu);

			while (true)
			{
				LOG_DEBUG("Bundle reader looping at...\n");
				memset(&bundle_spec, 0, sizeof(bundle_spec));
				memset(&payload, 0, sizeof(payload));

				ret = m_dtn->recv((char*)raw, m_mtu, m_timeout);
				if(ret > 0) {
					Packet* pkt = new Packet();
					pkt->PacketFromIP(raw, ret);

					IP* ip = pkt->GetLayer<IP>();
					if(ip){
						process_packet(pkt);
					}else{
						LOG_WARNING("Unable to parse IP header");
					}
				} else if (ret < 0) {
					LOG_CRITICAL("DTN Received failed!\n");
				}

			}
			free(raw);
			return true;
		}
};

NPSGATE_PLUGIN_CREATE(DTNInput);
NPSGATE_PLUGIN_DESTROY();
