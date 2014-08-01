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
**  @file npsgate_plugin.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef NPSGATE_PLUGIN_HPP_INCLUDED
#define NPSGATE_PLUGIN_HPP_INCLUDED

#include <string>
#include <map>
#include <vector>

#include <crafter.h>

#include "../plugincore.h"
#include "../npsgatevar.hpp"
#include "../message.hpp"

/* Helper macros to create the necessary plugin functions with C linkage. Each
   plugin should call each macro once. The macros create the npsgate_create and
   npsgate_destroy functions which create an instance of the plugin class. */
#define NPSGATE_PLUGIN_CREATE(type)													\
	extern "C" {																	\
	void npsgate_destroy(NpsGatePlugin*);											\
	NpsGatePlugin* npsgate_create(PluginCore* pc) {									\
		NpsGatePlugin* p = new typeof(type) (pc);									\
		if(!p->init()) {															\
			npsgate_destroy(p);														\
			return NULL;															\
		}																			\
																					\
		return p;																	\
	}																				\
	}

#define NPSGATE_PLUGIN_DESTROY()													\
	extern "C" void npsgate_destroy(NpsGatePlugin* p) {								\
		if(p) {																		\
			p->exit_handler();														\
			delete p;																\
		}																			\
	}																				\


using namespace std;
using namespace Crafter;
/**************************************************************
 **
 ** NpsGatePlugin class is the base class for all plugins.
 ** 
 ** Each plugin should override the 4 virtual functions and the
 ** virtual destructor.
 **
 ** This class is a thin wrapper around the PluginCore class.
 **
 **************************************************************/

namespace NpsGate {

class PluginCore;

typedef void (*NpsGateCallback)(void);

class NpsGatePlugin {
	public:
		NpsGatePlugin(PluginCore* c) {
			core = c;
			LOG_TRACE("Plugin base class for '%s' initialized. PluginCore: 0x%x\n", core->name.c_str(), core);
		};
		virtual ~NpsGatePlugin() { };

		virtual bool init() { return false; };
		virtual bool main() { return false; };
		virtual bool process_packet(Packet* p) { return false; };
		virtual bool process_message(Message* m) { return false; };
		virtual bool message_timeout() { return false; };
		virtual void exit_handler() { };

		inline bool forward_packet(string sink, Packet* p) { 
			return core->forward_packet(sink, p);
		}

		inline bool drop_packet(Packet* p) {
			return core->drop_packet(p);
		}

		inline bool publish(const string fq_name, NpsGateVar* v) {
			return core->publish(fq_name, v);
		}

		inline bool subscribe(const string fq_name) {
			return core->subscribe(fq_name);
		}

		inline const NpsGateVar* request(string fq_name) {
			return core->request(fq_name);
		}

		inline const set<string>& get_outputs() {
			return core->get_outputs();
		}

		inline string get_default_output() {
			return core->get_default_output();
		}

		inline void message_loop() {
			core->message_loop();
		}

		inline const Config* get_config() {
			return core->get_config();
		}

		inline bool set_timeout(uint32_t t) {
			return core->set_timeout(t);
		}

	private:
		PluginCore* core;
};

}

#endif /* NPSGATE_PLUGIN_HPP_INCLUDED */
