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
**  @file monitor_plugins.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "npsgate_context.hpp"
#include "pluginmanager.h"
#include "monitor.hpp"

namespace NpsGate {

/*	GENERATE PLUGIN GRAPH
	Format:
	
	<plugin name>|<dest plugin name>

   */
bool Monitor::generate_plugin_graph(ClientRequest* in) {
	ClientRequest out;
	set<string>::iterator output_iter;
	map<string, PluginCore*>::iterator plugin_iter;
	map<string, PluginCore*>* pl_list = &context.plugin_manager->plugins;

	out.command = in->command;

	for(plugin_iter = pl_list->begin(); plugin_iter != pl_list->end(); plugin_iter++) {
		if(!plugin_iter->second) {
			//LOG_WARNING("PluginCore* is NULL.\n");
			continue;
		}
		string p_name = plugin_iter->second->name;
		set<string>& output_list = plugin_iter->second->output_list;
		for(output_iter = output_list.begin(); output_iter != output_list.end(); output_iter++) {
			out.data += p_name + "|" + (*output_iter) + "\n";
		}
	}

	transmit_response(&out);
	return true;
}

/*	GENERATE STATISTICS FOR EACH PLUGIN
	Format:
	
	CORE|<uptime> <bytes_in> <bytes_out> <bytes dropped> <pkts in> <pkts out> <pkts dropped>

	<name>|<packets_in> <packets_out> <packets_dropped> <packets_held> <queue_length>

   */
bool Monitor::generate_plugin_stats(ClientRequest* in) {
	ClientRequest out;
	char buffer[256];
	set<string>::iterator output_iter;
	map<string, PluginCore*>::iterator plugin_iter;
	map<string, PluginCore*>& pl_list = context.plugin_manager->plugins;
	PluginCore* c;
	PacketManager* pm = context.packet_manager;

	out.command = in->command;

	snprintf(buffer, 256, "CORE|%lu %llu %llu %llu %llu %llu %llu\n",
			(unsigned long int)difftime(time(NULL), uptime),
			pm->bytes_in, pm->bytes_out, pm->bytes_dropped,
			pm->packets_in, pm->packets_out, pm->packets_dropped);
	out.data += buffer;

	for(plugin_iter = pl_list.begin(); plugin_iter != pl_list.end(); plugin_iter++) {
		if(!plugin_iter->second) {
			//LOG_WARNING("PluginCore* is NULL.\n");
			continue;
		}
		
		c = plugin_iter->second;
		snprintf(buffer, 256, "%s|%llu %llu %llu %llu %u\n", plugin_iter->first.c_str(),
				c->packets_in, c->packets_out, c->packets_dropped,
				(c->packets_in == 0 ? 0 : c->packets_in - c->packets_out - c->packets_dropped),
				c->input_queue.Length());
		out.data += buffer;
	}

	transmit_response(&out);
	return true;
}

/*  QUERY PLUGIN CONFIGURATION FILE
	Format:
		get <plugin name>
		set <plugin name>
*/
bool Monitor::process_config(ClientRequest* in) {
	ClientRequest out;
	char* buffer = NULL;
	size_t buf_size;
	PluginCore* pc = NULL;


	string action = in->options["action"];
	string plugin_name = in->options["plugin"];
	LOG_TRACE("Config: Type=%s  Plugin=%s\n", action.c_str(), plugin_name.c_str());

	if(action == "get") {
		out.command = in->command;
		out.options["action"] = "get-response";
		out.options["result"] = "ok";

		pc = context.plugin_manager->plugins[plugin_name];
		if(!pc) {
			LOG_WARNING("Failed to locate plugin with name: %s\n", plugin_name.c_str());
			return false;
		}

		FILE* cfg = fopen(pc->cfg_name.c_str(), "r");
		LOG_DEBUG("Got plugin. Opening config file: %s\n", pc->filename.c_str());
		if(cfg) {
			while(0 < getline(&buffer, &buf_size, cfg)) {
				out.data += buffer;
			}
			fclose(cfg);
		} else {
			LOG_WARNING("Failed to open config file: %s\n", pc->filename.c_str());
		}
	} else if(action == "set") {
		pc = context.plugin_manager->plugins[plugin_name];
		if(!pc) {
			LOG_WARNING("Failed to locate plugin with name: %s\n", plugin_name.c_str());
			return false;
		}

		FILE* cfg = fopen(pc->cfg_name.c_str(), "w");
		if(cfg) {
			if(1 != fwrite(in->data.c_str(), in->data.length(), 1, cfg)) {
				LOG_WARNING("Failed to write %d bytes to config file.\n", in->data.length());
			}
			fclose(cfg);
		} else {
			LOG_WARNING("Failed to open config file: %s\n", pc->filename.c_str());
		}

		out.command = in->command;
		out.options["action"] = "set-response";
		out.options["plugin"] = plugin_name;
		out.options["result"] = "ok";
	}

	transmit_response(&out);
	return true;
}
}

