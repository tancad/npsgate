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
**  @file router.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/06
**
*******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h> 
#include <boost/foreach.hpp>
#include "../npsgate_plugin.hpp"
#include "../logger.hpp"

using namespace Crafter;
using namespace NpsGate;

struct PortRange {
	string plugin;
	uint16_t start;
	uint16_t end;
};

class PortRouter : public NpsGatePlugin {
public:
	PortRouter(PluginCore* c) : NpsGatePlugin(c) { }
	~PortRouter() { }

	bool init() {
		const Config* config;

		config = get_config();
		LOG_INFO("PortRouter plugin starting intialization.\n");

		if(!config) {
			LOG_CRITICAL("Error accessing config file!\n");
			return false;
		}

		const Setting& root = config->getRoot();
		const Setting& outputs = root["outputs"];

		LOG_INFO("There are %u outputs:\n", outputs.getLength());
		ports.resize(outputs.getLength());

		for(int i = 0; i < outputs.getLength(); i++) {
			const Setting& pconf = outputs[i];
			uint32_t port_num;
			PortRange pr;
			string port_str;
			
			if(!pconf.lookupValue("plugin", pr.plugin)) {
				LOG_CRITICAL("Missing plugin name!\n");
			}
		
			if(!pconf.lookupValue("port", port_num)) {
				if(port_num > UINT16_MAX) {
					LOG_WARNING("Specified port %u is greater than %u. This output will be ignored.\n", port_num, UINT16_MAX);
					continue;
				}
				pr.end = pr.start = port_num;
			} else if(!pconf.lookupValue("port", port_str)) {
				if(2 != sscanf(port_str.c_str(), "%hu-%hu", &pr.start, &pr.end)) {
					LOG_WARNING("Failed to parse port specification '%s' into port range. This output will be ignored.\n", port_str.c_str());
					continue;
				}
			} else {
				LOG_WARNING("Missing port specification for output '%s', this output will be ignored.\n", pr.plugin.c_str());
				continue;
			}

			ports.push_back(pr);
			LOG_INFO("Adding Port Route: %u-%u => %s\n", pr.start, pr.end, pr.plugin.c_str());
		}

		return true;
	}

	bool process_packet(Packet* p) {
		IP* ip = p->GetLayer<IP>();
		TCP* tcp = p->GetLayer<TCP>();
		UDP* udp = p->GetLayer<UDP>();
		uint32_t dport;
		
		if(!ip) {
			LOG_WARNING("Received a non-IP packet. Dropping packet!\n");
			drop_packet(p);
			return false;
		}

		if(tcp) {
			dport = tcp->GetDstPort();
		} else if(udp) {
			dport = udp->GetDstPort();
		} else {
			LOG_WARNING("Received packet did not contain a TCP or UDP layer. Dropping packet!\n");
			drop_packet(p);
			return false;
		}

		BOOST_FOREACH(PortRange& pr, ports) {
			if(dport >= pr.start && dport <= pr.end) {
				LOG_TRACE("Port range found for port %u. Routing to '%s' (%u-%u).\n",
						dport, pr.plugin.c_str(), pr.start, pr.end);
				forward_packet(pr.plugin, p);
			}
		}

		LOG_TRACE("No port range found for port %u. Sending to default plugin '%s'.\n",
				dport, get_default_output().c_str());
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

private:
	vector<PortRange> ports;
};

NPSGATE_PLUGIN_CREATE(PortRouter);
NPSGATE_PLUGIN_DESTROY();
