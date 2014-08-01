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
**  @file router.cpp
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

static uint32_t NETMASKS[] = {
	0b00000000000000000000000000000000,
	0b10000000000000000000000000000000,
	0b11000000000000000000000000000000,
	0b11100000000000000000000000000000,
	0b11110000000000000000000000000000,
	0b11111000000000000000000000000000,
	0b11111100000000000000000000000000,
	0b11111110000000000000000000000000,
	0b11111111000000000000000000000000,
	0b11111111100000000000000000000000,
	0b11111111110000000000000000000000,
	0b11111111111000000000000000000000,
	0b11111111111100000000000000000000,
	0b11111111111110000000000000000000,
	0b11111111111111000000000000000000,
	0b11111111111111100000000000000000,
	0b11111111111111110000000000000000,
	0b11111111111111111000000000000000,
	0b11111111111111111100000000000000,
	0b11111111111111111110000000000000,
	0b11111111111111111111000000000000,
	0b11111111111111111111100000000000,
	0b11111111111111111111110000000000,
	0b11111111111111111111111000000000,
	0b11111111111111111111111100000000,
	0b11111111111111111111111110000000,
	0b11111111111111111111111111000000,
	0b11111111111111111111111111100000,
	0b11111111111111111111111111110000,
	0b11111111111111111111111111111000,
	0b11111111111111111111111111111100,
	0b11111111111111111111111111111110,
	0b11111111111111111111111111111111
};

struct Network {
	uint32_t network;
	uint32_t netmask;
};

class Router : public NpsGatePlugin {
public:
	Router(PluginCore* c) : NpsGatePlugin(c) {
	}

	~Router() {
	}

	bool init() {
		const Config* config;
		Network* n;

		config = get_config();
		LOG_INFO("Router plugin starting intialization.\n");

		if(!config) {
			LOG_CRITICAL("Error accessing config file!\n");
			return false;
		}

		const Setting& root = config->getRoot();
		const Setting& conf_realms = root["outputs"];

	
		LOG_INFO("There are %d routes.\n", conf_realms.getLength());
		for(int i = 0; i < conf_realms.getLength(); i++) {
			const Setting& rconf = conf_realms[i];
			string plugin, network_str;
			char* network, *netmask, *saveptr;
			in_addr addr;
			
			n = new Network();
	
			if(!rconf.lookupValue("plugin", plugin)) {
				LOG_CRITICAL("Missing plugin name!\n");
			}
		
			if(!rconf.lookupValue("network", network_str)) {
				LOG_CRITICAL("Missing network!\n");
			}

			if(network_str == "0.0.0.0/0") {
				LOG_INFO("Adding default route: %s\n", plugin.c_str());
				default_route = plugin;
				continue;
			}

			network = strdup(network_str.c_str());

			strtok_r(network, "/", &saveptr);
			netmask = strtok_r(NULL, "/", &saveptr);

			if(inet_aton(network, &addr) == 0) {
				LOG_CRITICAL("Failed to parse network: %s\n", network);
				return false;
			}
			n->network = addr.s_addr;
			n->netmask = atoi(netmask);	// FIXME: Error checking
			n->netmask = htonl(NETMASKS[n->netmask]);

			LOG_INFO("Adding Route: %s/%s => %s\n", network, netmask, plugin.c_str());

			routes[n] = plugin;

			free(network);
		}


		return true;
	}

	bool process_packet(Packet* p) {
		map<Network*, string>::iterator iter;
		IP* ip = p->GetLayer<IP>();
		in_addr addr;
		uint32_t destip;
		
		if(!ip) {
			LOG_WARNING("Received a non-IP packet. Dropping packet!\n");
			drop_packet(p);
			return false;
		}

		if(inet_aton(ip->GetDestinationIP().c_str(), &addr) == 0) {
			LOG_CRITICAL("Failed to parse IP address: %s. Dropping packet.\n", ip->GetDestinationIP().c_str());
			drop_packet(p);
			return false;
		}
		destip = addr.s_addr;
	
		for(iter = routes.begin(); iter != routes.end(); iter++) {
			if((destip & iter->first->netmask) == iter->first->network) {
				LOG_INFO("Route found: %s => %s\n", ip->GetDestinationIP().c_str(), iter->second.c_str());
				forward_packet(iter->second, p);
				return true;
			}

		}

		LOG_INFO("No route found. Sending to default route: %s\n", default_route.c_str());
		forward_packet(default_route, p);

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
	map<Network*, string> routes;
	string default_route;

};

NPSGATE_PLUGIN_CREATE(Router);
NPSGATE_PLUGIN_DESTROY();
