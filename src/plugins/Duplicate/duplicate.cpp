/******************************************************************************
**
**  Duplicate plugin.
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
**  @file duplicate.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/16
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


class Duplicate : public NpsGatePlugin {
public:
	Duplicate(PluginCore* c) : NpsGatePlugin(c) { }
	~Duplicate() { }

	bool init() {
		plugin_list = get_outputs();

		/* Remove one of the output in the list and store in a
		   separate variable. This will be the plugin where the
		   "original" packet is sent to. */
		first_plugin = *plugin_list.begin();
		plugin_list.erase(first_plugin);

		return true;
	}

	bool process_packet(Packet* p) {

		/* Send a duplicate copy of the packet to each valid output */
		BOOST_FOREACH(const string& plugin, plugin_list) {
			Packet* new_p = new Packet(*p);
			forward_packet(plugin, new_p);
		}

		/* Send the original packet to the first output plugin */
		forward_packet(first_plugin, p);
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
	string first_plugin;
	set<string> plugin_list;
};

NPSGATE_PLUGIN_CREATE(Duplicate);
NPSGATE_PLUGIN_DESTROY();
