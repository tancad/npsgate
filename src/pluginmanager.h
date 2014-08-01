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
**  @file pluginmanager.h
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef PLUGINMAMAGER_HPP_INCLUDED
#define PLUGINMAMAGER_HPP_INCLUDED

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

		void load_plugins();
		NpsGate::JobQueue* get_packet_queue(string name);

		void unload_plugin(PluginCore* p);

		string get_name_from_thread(pthread_t);

		friend class Monitor;

	private:
		const NpsGateContext& context;
		map<string, PluginCore*> plugins;
		map<pthread_t, string> plugin_threads;
};

}

#endif /* PLUGINMAMAGER_H_INCLUDED */
