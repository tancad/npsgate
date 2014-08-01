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
**  @file publish_subscribe.cpp
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

#include <string>
#include <list>

#include "logger.h"
#include "plugincore.h"
#include "pluginmanager.h"
#include "publish_subscribe.h"

using namespace std;
using namespace Crafter;

namespace NpsGate {

PublishSubscribe::PublishSubscribe(const NpsGateContext& c) : context(c) {
}

PublishSubscribe::~PublishSubscribe() {
	for(map<string, list<PluginCore*>* >::iterator it = subscriptions.begin(); it != subscriptions.end(); ++it) {
		delete it->second;
		subscriptions.erase(it);
	}
}

bool PublishSubscribe::add_subscription(PluginCore* p, const string fq_name) {
	list<PluginCore*>* plist;

	if(!p) {
		LOG_WARNING("PluginCore was NULL!\n");
		return false;
	}

	/* Check to see if this fq_name already exists, if not create it */
	if(subscriptions.find(fq_name) == subscriptions.end()) {
		plist = new list<PluginCore*>();
		subscriptions[fq_name] = plist;
	} else {
		plist = subscriptions[fq_name];
	}

	/* Check to see if this plugin is already subscribed to this fq_name */
	for(list<PluginCore*>::iterator it = plist->begin(); it != plist->end(); ++it) {
		if(*it == p) {
			return true;
		}
	}

	plist->push_back(p);

	return true;
}

bool PublishSubscribe::remove_subscription(PluginCore* p, const string fq_name) {
	list<PluginCore*>* plist;

	if(!p) {
		LOG_WARNING("PluginCore was NULL!\n");
		return false;
	}

	if(subscriptions.find(fq_name) != subscriptions.end()) {
		plist = subscriptions[fq_name];
		plist->remove(p);
	}

	return true;
}

bool PublishSubscribe::remove_all_subscriptions(PluginCore* p) {
	for(map<string, list<PluginCore*>* >::iterator it = subscriptions.begin(); it != subscriptions.end(); ++it) {
		remove_subscription(p, it->first);
	}
	return true;
}

bool PublishSubscribe::publish(PluginCore* p, const string fq_name, NpsGateVar* v) {
	list<PluginCore*>* plist;

	if(!p) {
		LOG_WARNING("PluginCore was NULL!\n");
		return false;
	}

	LOG_DEBUG("Publish: %s\n", fq_name.c_str());

	if(subscriptions.find(fq_name) == subscriptions.end()) {
		subscriptions[fq_name] = new list<PluginCore*>;
//		return true;
	}


	plist = subscriptions[fq_name];

	for(list<PluginCore*>::iterator it = plist->begin(); it != plist->end(); ++it) {
		JobQueueItem* item = new JobQueueItem();
		PluginCore* p = *it;

		item->type = MESSAGE;
		item->message = new Message();
		item->message->type = SUBSCRIBE_UPDATE;
		item->message->fq_name = fq_name;
		item->message->value = v;
		item->message->orig = p;

		/* Acquire a reference to the NpsGateVar for each destination */
		v->ref();

		LOG_DEBUG("Sending updated '%s' to plugin %p\n", fq_name.c_str(), p);
		p->input_queue.Enqueue(item);
	}

	return true;
}

bool PublishSubscribe::add_publication(PluginCore* p, const string fq_name, const string desc, NpsGateVar* v) {
	LOG_TRACE("Adding Publication: %s", fq_name.c_str());

	publications[p][fq_name].fq_name = fq_name;
	publications[p][fq_name].description = desc;
	publications[p][fq_name].last_value = v;
	if(v) {
		publications[p][fq_name].last_update = time(NULL);
	} else {
		publications[p][fq_name].last_update = 0;
	}

	return true;
}

//const map<string>& PublishSubscribe::list_subscriptions(PluginCore* p) {
//	list<string> s;

//	return s;
//}

const map<string,Publication>& PublishSubscribe::list_publications(PluginCore* p) {
	return publications[p];
}

}

