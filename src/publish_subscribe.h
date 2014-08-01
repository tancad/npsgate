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
**  @file publish_subscribe.h
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef PUBLISH_SUBSCRIBE_H_INCLUDED
#define PUBLISH_SUBSCRIBE_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

#include <string>
#include <list>
#include <crafter.h>

#include "npsgate_context.hpp"
#include "logger.h"
#include "plugincore.h"
#include "pluginmanager.h"

using namespace std;
using namespace Crafter;

namespace NpsGate {
	class PluginManager;
	class PluginCore;
	class NpsGateContext;


class Publication {
	public:
		string fq_name;
		string description;
		time_t last_update;
		NpsGateVar* last_value;
};

class PublishSubscribe {
	public:
		PublishSubscribe(const NpsGateContext&);
		~PublishSubscribe(); 

		bool add_subscription(PluginCore* p, const string fq_name);
		bool remove_subscription(PluginCore* p, const string fq_name);
		bool remove_all_subscriptions(PluginCore*);
		bool publish(PluginCore* p, const string fq_name, NpsGateVar* v);

		bool add_publication(PluginCore* p, const string fq_name, const string desc, NpsGateVar* v);
		//list<string> list_subscriptions(PluginCore* p);
		const map<string,Publication>& list_publications(PluginCore* p);

		friend class Monitor;
	private:
		const NpsGateContext& context;
		PluginManager* plugin_mgr;
		map<string, list<PluginCore*>* > subscriptions;
		map<PluginCore*, map<string, Publication> > publications;
};

}

#endif /* PUBLISH_SUBSCRIBE_H_INCLUDED */
