/******************************************************************************
  **
  **  NpsGate.
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
  **  @file pluginmanager.cpp
  **  @author Lance Alt (lancealt@gmail.com)
  **  @date 2014/09/23
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
#include <boost/filesystem.hpp>

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

		load_plugin(plugin_conf);
	}

	LOG_DEBUG("Finished loading %u plugins.\n", plugins.size());
}

void PluginManager::load_plugin(const Setting& plugin_config) {
	string name, library, config_file;
	bool enabled = true;

	/* Lookup the name and library config items. Both are required. */
	if(!plugin_config.lookupValue("name", name)) {
		LOG_CRITICAL("Missing required plugin 'name' configuration option.\n");
	}
	if(!plugin_config.lookupValue("library", library)) {
		LOG_CRITICAL("Missing required plugin 'name' configuration option.\n");
	}
	
	/* Lookup the optional config and enabled config items. */
	plugin_config.lookupValue("config", config_file);
	plugin_config.lookupValue("enabled", enabled);

	boost::filesystem::path plugin_library_name();
	boost::filesystem::path plugin_config_full_path();
	boost::filesystem::path plugin_config_name();

	/* The path to the library works as follows:
	   1. If an absolute path, use as is.
	   2. If a relative path, build as follows:
	     a. Start with the path to the 'npsgate' executable
		 b. Append the 'plugindir' path from the main config file
		 c. Append the 'library' option for this plugin
	*/
	boost::filesystem::path plugin_library_path(library);
	if(plugin_library_path.is_absolute()) {
		// use as is, path is absolute
	} else {
		plugin_library_path = context.npsgate_execute_path;
		plugin_library_path /= context.plugin_dir;
		plugin_library_path /= library;
	}

	/* The path to the config file works as follows:
	   1. If an absolute path, use as is.
	   2. If a relative path, build as follows:
	     a. Start with the path to the main config file
		 b. Append the 'plugin_conf_dir' path from the main config file
		 c. Append the 'config' option for this plugin
	*/
	boost::filesystem::path plugin_config_file(config_file);
	if(plugin_config_file.is_absolute()) {
		// use as is, path is absolute
	} else {
		plugin_config_file = context.main_config_path;
		plugin_config_file /= context.plugin_config_dir;
		plugin_config_file /= config_file;
	}


	library = plugin_library_path.native();
	config_file = plugin_config_file.native();

	if(enabled) {
		PluginCore* p = new PluginCore(context, name);
		p->load(library, config_file);
		p->init();
		plugins[name] = p;
		plugin_threads[p->thread_id] = name;
	}

	LOG_INFO("Plugin Loaded: %s\n", name.c_str());
	LOG_INFO("  Library: %s\n", library.c_str());
	LOG_INFO("  Config: %s\n", config_file.c_str());
	LOG_INFO("  Enabled: %u\n", enabled);

}

JobQueue* PluginManager::get_input_queue(const string& name) {
	if(plugins.end() == plugins.find(name)) {
		LOG_WARNING("Could not find plugin with name: %s\n", name.c_str());
		return NULL;
	}

	return &(plugins[name]->input_queue);
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

string PluginManager::get_name_from_thread(const pthread_t tid) {
	map<pthread_t, string>::iterator it;

	it = plugin_threads.find(tid);
	if(it == plugin_threads.end()) {
		return string();
	}

	return it->second;
}

}

