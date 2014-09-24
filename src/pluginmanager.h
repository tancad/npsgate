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
  **  @file pluginmanager.h
  **  @author Lance Alt (lancealt@gmail.com)
  **  @date 2014/09/23
  **
  *******************************************************************************/

#ifndef PLUGINMAMAGER_H_INCLUDED
#define PLUGINMAMAGER_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

#include <string>
#include <crafter.h>

#include "npsgate_context.hpp"
#include "logger.h"
#include "plugincore.h"

using namespace std;
using namespace Crafter;

namespace NpsGate {

	class PluginCore;
	class JobQueueItem;
	class NpsGateContext;
	//typedef Queue<JobQueueItem*> JobQueue;

class PluginManager {
	public:
		PluginManager(const NpsGateContext&);
		virtual ~PluginManager(); 

		void load_plugin(const Setting& plugin_config);
		void unload_plugin(PluginCore* p);
		void load_plugins();

		NpsGate::JobQueue* get_input_queue(const string& name);
		string get_name_from_thread(pthread_t);

		friend class Monitor;

	private:
		const NpsGateContext& context;
		map<string, PluginCore*> plugins;
		map<pthread_t, string> plugin_threads;
};

}

#endif /* PLUGINMAMAGER_H_INCLUDED */
