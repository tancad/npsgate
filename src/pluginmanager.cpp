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
**  @file pluginmanager.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

#include <map>
#include <string>
#include <crafter.h>
#include <libconfig.h++>

#include "npsgate_context.hpp"
#include "logger.h"
#include "plugins/npsgate_plugin.hpp"
#include "plugincore.h"
#include "pluginmanager.h"

using namespace std;
using namespace libconfig;
using namespace Crafter;

namespace NpsGate {

PluginManager::PluginManager(const NpsGateContext& c) : context(c) {
}

PluginManager::~PluginManager() {
	LOG_INFO("PluginManager terminating. Sending termination to all plugins...\n");
	for(map<string, PluginCore*>::iterator it = plugins.begin(); it != plugins.end(); ++it) {
		it->second->unload();
		delete it->second;
	}
	LOG_INFO("All plugins successfully terminated.\n");
}

void PluginManager::load_plugins() {
	string name, plugin_path, plugin_config_path;
	Config* cfg = context.config;

	const Setting& root = cfg->getRoot();
	const Setting& conf_plugins = root["plugins"];

	/* Get the plugin path. Add a trailing '/' if it is not there. */
	if(cfg->lookupValue("NpsGate.plugindir", plugin_path)) {
		if(plugin_path.find_last_of('/') >= plugin_path.length()) {
			plugin_path += "/";
		}
	} else {
		LOG_CRITICAL("'plugindir' not set in config file.\n");
		return;
	}

	/* Get the plugin config path. Add a trailing '/' if it is not there. */
	if(cfg->lookupValue("NpsGate.plugin_conf_dir", plugin_config_path)) {
		if(plugin_config_path.find_last_of('/') >= plugin_config_path.length()) {
			plugin_config_path += "/";
		}
	} else {
		LOG_CRITICAL("'plugindir' not set in config file.\n");
		return;
	}



	LOG_INFO("There are %d plugins to check.\n", conf_plugins.getLength());
	for(int i = 0; i < conf_plugins.getLength(); i++) {
		const Setting& plugin_conf = conf_plugins[i];
		string name, library, config_file;
		bool enabled = true;

		/* Lookup the name and library config items. Both are required. */
		if(!(plugin_conf.lookupValue("name", name) &&
			 plugin_conf.lookupValue("library", library))) {
			LOG_CRITICAL("For plugins: name and library config items are required.\n");
		}
		
		/* Lookup the optional config and enabled config items. */
		plugin_conf.lookupValue("config", config_file);
		plugin_conf.lookupValue("enabled", enabled);

		LOG_INFO("Plugin: name='%s', library='%s', config='%s', enabled=%d\n",
				name.c_str(), config_file.c_str(), library.c_str(), enabled);

		/* Skip disabled plugins */
		if(false == enabled) {
			LOG_INFO("Plugin '%s' is disabled. Skipping.\n", name.c_str());
			continue;
		}

		/* Build path to the plugin file */
		if(library[0] != '/') {
			library = plugin_path + library;
		}

		/* Build path to the (optional) plugin config file */
		if(config_file != "" && config_file[0] != '/') {
			config_file = plugin_config_path + config_file;
		}

		PluginCore* p = new PluginCore(context, name);
		p->load(library, config_file);
		p->init();
		plugins[name] = p;
		plugin_threads[p->thread_id] = name;
	}

	LOG_DEBUG("Finished loading %u plugins.\n", plugins.size());
}

JobQueue* PluginManager::get_packet_queue(string name) {
	PluginCore* p;

	if(plugins.end() == plugins.find(name)) {
		LOG_WARNING("Could not find plugin with name: %s\n", name.c_str());
		return NULL;
	}

	p = plugins[name];
	LOG_TRACE("Returning input_queue for '%s': %p\n", name.c_str(), &(p->input_queue));
	return &(p->input_queue);
}

void PluginManager::unload_plugin(PluginCore* p) {
	map<string, PluginCore*>::iterator it;

	/* Erase the plugin from the plugin list */
	for(it = plugins.begin(); it != plugins.end(); ++it) {
		if(it->second == p) {
			plugins.erase(it);
			break;
		}
	}

	/* Unload the plugin and delete it */
	p->unload();
	delete p;
}

string PluginManager::get_name_from_thread(pthread_t tid) {
	map<pthread_t, string>::iterator it;
	string name;

	it = plugin_threads.find(tid);
	if(it == plugin_threads.end()) {
		return "";
	}

	return it->second;
}

}

