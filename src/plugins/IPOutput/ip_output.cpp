/******************************************************************************
  **
  **  NpsGate IPOutput plugin.
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
  **  @file ip_output.cpp
  **  @author Lance Alt (lancealt@gmail.com)
  **  @date 2014/09/23
  **
  *******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h>

#include "../npsgate_plugin.hpp"
#include "../logger.hpp"

using namespace Crafter;
using namespace NpsGate;


class IPOutput : public NpsGatePlugin {
public:
	IPOutput(PluginCore* c) : NpsGatePlugin(c) {
	}

	virtual bool init() {
		raw_socket = create_raw_socket();

		return (raw_socket == -1 ? false : true);
	}

	virtual bool process_packet(Packet* p) {
		bool rval = false;
		IP* ip = p->GetLayer<IP>();

		if(!ip) {
			LOG_WARNING("Received a non-IP packet. Dropping packet.\n");
		} else if(!p->SocketSend(raw_socket)) {
			LOG_WARNING("Failed to send packet. Packet will be dropped!\n");
		} else {
			rval = true;
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
	int raw_socket;

	int create_raw_socket() {
		int s;
		int val = 1;

		s = socket(AF_INET, SOCK_RAW, 4);
		setsockopt(s, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));
		if(s == -1) {
			LOG_CRITICAL("Failed to create raw socket. Reason: %s\n", strerror(errno));
		}

		return s;
	}

};

NPSGATE_PLUGIN_CREATE(IPOutput);
NPSGATE_PLUGIN_DESTROY();
