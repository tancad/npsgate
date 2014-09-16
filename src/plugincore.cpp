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
**  @file plugincore.cpp
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
#include <stdint.h>

#include <string>
#include <set>

#include "npsgate_context.hpp"
#include "npsgate_core.hpp"
#include "plugincore.h"
#include "jobqueue.h"
#include "logger.h"
#include "plugins/npsgate_plugin.hpp"

using namespace std;

namespace NpsGate {

PluginCore::PluginCore(const NpsGateContext& c, string pname) : context(c), 
	handle(NULL), config(NULL), create(NULL), destroy(NULL),
	packets_in(0), packets_out(0), packets_dropped(0) {
	name = pname;
	exit_flag = false;
	jobqueue_timeout = 0xffffffff;
}


PluginCore::~PluginCore() {
	if(handle) {
		unload();
	}
}

bool PluginCore::load(const string so_name, const string conf_name) {
	filename = so_name;
	cfg_name = conf_name;

	/* dlopen the shared object file */
	handle = ::dlopen(so_name.c_str(), RTLD_NOW);	
	if(!handle) {
		LOG_CRITICAL("dlopen() failed: %s\n", dlerror());
		return false;
	}
	LOG_DEBUG("Opened the shared library. handle=%p\n", handle);


	/* Find the create/destroy methods in the shared object */
	if(	!get_sym("npsgate_create", (void**)&create) || 
		!get_sym("npsgate_destroy", (void**)&destroy)) {
		LOG_CRITICAL("Invalid plugin format. Could not located 'create' and 'destroy'\n");
		return false;
	}


	/* Load the plugin configuration file. Plugin load fails if the
	   configuration files could not be loaded. */
	if(conf_name != "") {
		LOG_DEBUG("Attempting to load plugin config file '%s'\n", conf_name.c_str());
		try{
			config = new Config();
			config->readFile(conf_name.c_str());
		} catch (const FileIOException& fioex) {
			LOG_CRITICAL("Failed to load plugin configuration file: '%s'\n", conf_name.c_str());
			return false;
		} catch (const ParseException& pex) {
			LOG_CRITICAL("Failed to parse plugin configuration file '%s' at line %d\n",
				pex.getFile(), pex.getLine());
			return false;
		}
	}

	parse_outputs();
	parse_publications();

	LOG_DEBUG("Successfully loaded plugin: %s\n", so_name.c_str());
	return true;
}

bool PluginCore::unload() {
	/* If handle is NULL, this object has already been unloaded. */
	if(!handle) { 
		LOG_WARNING("Attempting to unload a module that has already been unloaded!\n");
		return true;
	}

	LOG_DEBUG("Unloading plugin: %s\n", name.c_str());

	/* Stop the plugin thread and wait for it to finish */
	LOG_DEBUG("Sending cancel to thread: %p\n", (void*)thread_id);
	exit_flag = true;
	pthread_cancel(thread_id);
	LOG_DEBUG("Waiting for child thread to terminate...\n");
	pthread_join(thread_id, NULL);
	LOG_DEBUG("Child thread %p terminated.\n", thread_id);

	/* Remove all subscriptions */
	LOG_DEBUG("Removing all subscriptions.\n");
	context.publish_subscribe->remove_all_subscriptions(this);

	/* Close the config file if there was one */
	if(config) {
		LOG_DEBUG("Closing and deleting the config file object.\n");
		delete config;
		config = NULL;
	}

	/* Call the plugin's destroy method */
	destroy(plugin);

//	if(dlclose(handle)) {
//		LOG_WARNING("dlclose(%p) failed: %s\n", handle, dlerror());
//		return false;
//	}

	/* Set handle to NULL to indicate this plugin is unloaded */
	handle = NULL;
	return true;
}

bool PluginCore::init() {
	LOG_TRACE("Creating plugin instance.\n");
	plugin = create(this);
	if(!plugin) {
		LOG_WARNING("Failed to create plugin instance!\n");
		return false;
	}

	LOG_DEBUG("Created plugin instance: %p\n", plugin);

	spawn_thread(0);

	return true;
}

Config* PluginCore::get_config() {
	return config;
}

/**
  * Virtual functions called by the plugins to interface with the core. Since
  * plugins are located in separate object files, all functions accessable by
  * the plugins must be virtual.
  */
bool PluginCore::forward_packet(string queue, Packet* p) {
	JobQueueItem* item = new JobQueueItem();

	if(output_list.end() == output_list.find(queue)) {
		LOG_CRITICAL("'%s' attempted to send packet to invalid plugin '%s'\n", filename.c_str(), queue.c_str());
		return false;
	}

	JobQueue* pq = context.plugin_manager->get_packet_queue(queue);
	if(!pq) {
		LOG_WARNING("Could not locate queue with name: %s\n", queue.c_str());
		return false;
	}

//	if(pq->Length() > 100) {
		//LOG_WARNING("Packet queue longer than 100, dropping packet!\n");
//		return false;
//	}

	context.packet_manager->ref_packet(p);

	LOG_TRACE("Enqueuing packet for '%s', p = %p\n", queue.c_str(), p);
	item->type = PACKET;
	item->packet = p;
	pq->Enqueue(item);

	packets_out++;

	return true;
}

bool PluginCore::drop_packet(Packet* p) {
	LOG_TRACE("Dropping packet %p\n", p);
	packets_dropped++;
//	context.packet_manager->unref_packet(p);
	return true;
}

/*
 * Direct publish to a particular module
 */
bool PluginCore::publish(const string module, const string fq_name, NpsGateVar* v) {
	JobQueue* pq = context.plugin_manager->get_packet_queue(module);	

	if(!pq) {
		LOG_DEBUG("Failed to located job queue for module '%s'\n", module.c_str());
		return false;
	}

	JobQueueItem* item = new JobQueueItem();

	item->type = MESSAGE;
	item->message = new Message();
	item->message->type = SUBSCRIBE_UPDATE;
	item->message->fq_name = fq_name;
	item->message->value = v;

	pq->Enqueue(item);
	return true;
}

bool PluginCore::publish(const string fq_name, NpsGateVar* v) {
	return context.publish_subscribe->publish(this, fq_name, v);
}

bool PluginCore::subscribe(const string fq_name) {
	return context.publish_subscribe->add_subscription(this, fq_name);
}

const NpsGateVar* PluginCore::request(string fq_name) {
	LOG_TODO("Request variable: %s\n", fq_name.c_str());
	//core->request(fq_name);
	return NULL;
}

const set<string>& PluginCore::get_outputs() {
	return output_list;
}

string PluginCore::get_default_output() {
	return default_output;
}


bool PluginCore::message_loop() {
	JobQueueItem* item;
				Packet* pkt;

	LOG_DEBUG("Plugin waiting for packet...\n");

	while(exit_flag == false) {
		item = input_queue.Dequeue(jobqueue_timeout);
		if(!item) {
			plugin->message_timeout();
			continue;
		}

		switch(item->type) {
			case PACKET:
				LOG_TRACE("Received packet, dispatching to plugin.\n");

				/* TODO: If the following code block is removed, for some reason the
				   Packet gets corrupted and the desitnation plugin is unable to strip
				   out layers!!! */
				pkt = item->packet;
				for(uint32_t i = 0; i < pkt->GetLayerCount(); i++) {
					Ethernet* eth = pkt->GetLayer<Ethernet>(i);
					eth = eth;
				}


				packets_in++;
				plugin->process_packet(item->packet);
				context.packet_manager->unref_packet(item->packet);
				delete item;
				break;
			case MESSAGE:
				LOG_TRACE("Received message, dispatching to plugin.\n");
				// We already have a reference to the NpsGateVar since it
				// was in our queue. (see publish_subscribe.cpp)
				plugin->process_message(item->message);

				// Unref the NpsGateVarl. If the plugin wants to keep it
				// around, they need to ref it again.
				item->message->value->unref();

				delete item->message;
				delete item;
				break;
		}
	}

	LOG_DEBUG("Exit flag true. Exiting message loop.\n");

	return true;
}

bool PluginCore::get_sym(const string name, void** func) {
	const char* errstr;
	dlerror();
	*func = dlsym(handle, name.c_str());
	errstr = dlerror();
	if(!*func && errstr) {
		LOG_TRACE("Could not find symbol '%s' in '%s'\n", name.c_str(), filename.c_str());
		return false;
	}
	LOG_TRACE("Found symbol '%s' in '%s' = %p\n", name.c_str(), filename.c_str(), *func);
	return true;
}

bool PluginCore::spawn_thread(int priority) {
	pthread_attr_t tattr;
	sched_param tpriority;
	int s;

	tpriority.sched_priority = priority;

	pthread_attr_init(&tattr);
	s = pthread_create(&thread_id, NULL, PluginCore::thread_bootstrap, this);

	pthread_setschedparam(thread_id, SCHED_OTHER, &tpriority);
	if(s) {
		LOG_CRITICAL("Failed to create plugin thread! Reason: %s\n", strerror(errno));
	}
	LOG_DEBUG("Thread spawned for plugin '%s'. TID = %p\n", filename.c_str(), thread_id);

	pthread_attr_destroy(&tattr);
	return true;
}

void* PluginCore::thread_bootstrap(void* arg) {
	PluginCore* p = (PluginCore*)arg;

//	LOG_TRACE("Starting plugin main.\n");
	p->plugin->main();

//	LOG_INFO("Plugin thread exited. Cleaning up.\n");
//	p->context.plugin_manager->unload_plugin(p);

	return NULL;
}

void PluginCore::parse_outputs() {

	if(!config) {
		LOG_WARNING("Plugin '%s' does not have a config file. I can not check outputs.\n",
				filename.c_str());
		return;
	}
	try {
		const Setting& root = config->getRoot();
		const Setting& conf_plugins = root["outputs"];

		LOG_INFO("There are %d outputs to check.\n", conf_plugins.getLength());

		for(int i = 0; i < conf_plugins.getLength(); i++) {
			const Setting& plugin_conf = conf_plugins[i];
			string name, library, config_file;
	
			if(!(plugin_conf.lookupValue("plugin", name))) {
				LOG_WARNING("Output is missing the 'plugin' key!\n");
				continue;
			}
	
			LOG_INFO("Output: %s\n", name.c_str());
			output_list.insert(name);
			if(i == 0) {
				default_output = name;
			}
		}
	} catch (SettingNotFoundException& ex) {
		LOG_WARNING("Plugin '%s' does not have an 'outputs' section. I can not check outputs.\n",
				filename.c_str());
		return;
	}
}

void PluginCore::parse_publications() {

	if(!config) {
		LOG_WARNING("Plugin '%s' does not have a config file. I can not check outputs.\n",
				filename.c_str());
		return;
	}
	try {
		const Setting& root = config->getRoot();
		const Setting& conf_publish = root["publish"];

		LOG_INFO("There are %d publications to check.\n", conf_publish.getLength());

		for(int i = 0; i < conf_publish.getLength(); i++) {
			const Setting& publish_conf = conf_publish[i];
			string name, description, default_val;
	
			if(!(publish_conf.lookupValue("name", name))) {
				LOG_WARNING("Output is missing the 'name' key!\n");
				continue;
			}
		
			if(!(publish_conf.lookupValue("description", description))) {
				LOG_WARNING("Output is missing the 'description' key. All publications should have a description.\n");
			}

			context.publish_subscribe->add_publication(this, name, description, NULL);
		}
	} catch (SettingNotFoundException& ex) {
		LOG_INFO("Plugin '%s' does not have an 'publish' section. Plugin will not publish any values.\n",
				filename.c_str());
		return;
	}
}


bool PluginCore::set_timeout(uint32_t t) {
	jobqueue_timeout = t;
	return true;
}

}

