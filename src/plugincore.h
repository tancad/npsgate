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
**  @file plugincore.h
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef PLUGINCORE_HPP_INCLUDED
#define PLUGINCORE_HPP_INCLUDED

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

#include <string>
#include <set>
#include <crafter.h>
#include <libconfig.h++>

#include "logger.h"
#include "message.hpp"
#include "jobqueue.h"
#include "pluginmanager.h"
#include "publish_subscribe.h"
#include "npsgate_context.hpp"

using namespace std;
using namespace Crafter;


namespace NpsGate {


//typedef Queue<JobQueueItem*> JobQueue;

	class NpsGatePlugin;
	class NpsGateVar;
	class PluginCore;

typedef void* dlhandle_t ;
typedef NpsGatePlugin* (*NpsGatePluginCreateFunc)(const PluginCore*);
typedef void (*NpsGatePluginDestroyFunc)(NpsGatePlugin*);

class PluginCore {
	public:
		PluginCore(const NpsGateContext&,string);
		virtual ~PluginCore(); 

		bool load(const string so_name, const string conf_name);
		bool unload();

		bool init();
		Config* get_config();

		virtual bool forward_packet(string queue, Packet* p);
		virtual bool drop_packet(Packet* p);
		virtual bool publish(const string fq_name, NpsGateVar* v);
		virtual bool publish(const string module, const string fq_name, NpsGateVar* v);
		virtual bool subscribe(const string fq_name);
		virtual const NpsGateVar* request(string fq_name);
		virtual bool message_loop();

		virtual const set<string>& get_outputs();
		virtual string get_default_output();

		virtual bool set_timeout(uint32_t);

		JobQueue input_queue;
		string name;
		pthread_t thread_id;
		bool exit_flag;

		friend class Monitor;

	private:
		void parse_outputs();
		void parse_publications();
		bool get_sym(const string name, void** func);
		bool spawn_thread(int priority);
		static void* thread_bootstrap(void* arg);

		const NpsGateContext& context;

		uint32_t jobqueue_timeout;

		string filename;
		dlhandle_t handle;

		NpsGatePlugin* plugin;

		string cfg_name;
		Config* config;
		set<string> output_list;
		string default_output;

		NpsGatePluginCreateFunc create;
		NpsGatePluginDestroyFunc destroy;

		// Statistics for each plugin
		unsigned long long packets_in;
		unsigned long long packets_out;
		unsigned long long packets_dropped;
		unsigned long long packet_held;
};

}

#endif /* PLUGINCORE_H_INCLUDED */
