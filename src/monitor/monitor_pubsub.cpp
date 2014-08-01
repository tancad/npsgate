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
**  @file monitor_pubsub.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <string>
#include <typeinfo>
#include <cxxabi.h>
#include <boost/lexical_cast.hpp>

#include "monitor.hpp"

namespace NpsGate {

bool Monitor::pubsub(ClientRequest* req) {
	if(req->options.find("action") == req->options.end()) {
		LOG_WARNING("'action' option not defined. Ignoring request.\n");
		return false;
	}

	string action = req->options["action"];
	LOG_DEBUG("Action is: %s\n", action.c_str());

	if(action == "subscribe") {
//		return subscribe(req);
	} else if(action == "unsubscribe") {
//		return unsubscribe(req);
	} else if(action == "list") {
		return list_subscribed();
	} else if(action == "publications") {
		return list_publications(req);
	}

	LOG_WARNING("Unhandled action '%s'.\n", action.c_str());
	return false;
}


bool Monitor::subscribe_cmd(ClientRequest* req) {
	map<string, string>::iterator fq_name = req->options.find("fq_name");
	if(fq_name == req->options.end()) {
		LOG_WARNING("Subscribe request did not contain a 'fq_name' option.\n");
		return false;
	}

	if(false == subscribe(fq_name->second)) {
		LOG_WARNING("Failed to subscribe to '%s'.\n", fq_name->second.c_str());
		return false;
	}

	return true;
}

bool Monitor::unsubscribe_cmd(ClientRequest* req) {
	map<string, string>::iterator fq_name = req->options.find("fq_name");
	if(fq_name == req->options.end()) {
		LOG_WARNING("Unsubscribe request did not contain a 'fq_name' option.\n");
		return false;
	}


	return true;
}

bool Monitor::list_plugin_publications(ClientRequest* req) {
	ClientRequest out;
	string msg;

	map<string, string>::iterator plugin = req->options.find("plugin");
	if(plugin == req->options.end()) {
		LOG_WARNING("Unsubscribe request did not contain a 'plugin' option.\n");
		return false;
	}
	string& plugin_name = plugin->second;

	map<string, PluginCore*>::iterator pc =  context.plugin_manager->plugins.find(plugin_name);
	if(pc == context.plugin_manager->plugins.end()) {
		LOG_WARNING("Can not find plugin with name '%s'.\n",  plugin_name.c_str());
		return false;
	}

	map<PluginCore*, map<string, Publication> >::iterator iter = context.publish_subscribe->publications.find(pc->second);
	if(iter == context.publish_subscribe->publications.end()) {
		LOG_WARNING("Can not find plugin with name '%s'.\n",  plugin_name.c_str());
		return false;
	}
	
	map<string, Publication>::iterator pub = iter->second.begin();
	while(pub != iter->second.end()) {
		msg += pub->first + "|" + pub->second.fq_name + "\n";
		pub++;
	}

	LOG_INFO("Publisher List: %s\n", msg.c_str());

	out.command = "pubsub_publication_list";
	out.options["plugin"] = plugin_name;
	out.data = msg;
	transmit_response(&out);

	return true;
}

bool Monitor::list_plugin_subscriptions(ClientRequest* req) {
	return true;
}

bool Monitor::list_subscribed() {
	ClientRequest out;
	string msg;

	LOG_DEBUG("Listing subscriptions\n");
	list<PluginCore*>::iterator p;
	map<string, list<PluginCore*>*>::iterator iter = context.publish_subscribe->subscriptions.begin();
	while(iter != context.publish_subscribe->subscriptions.end()) {
		msg += iter->first;
		for(p = iter->second->begin();p != iter->second->end(); p++) {
			msg += "|";
			msg += (*p)->name;
		}
		msg += "\n";
		iter++;
	}
	
	out.command = "pubsub";
	out.data = msg;
	transmit_response(&out);

	return true;
}

bool Monitor::list_all() {
	return true;
}

bool Monitor::subscription_receive(Message* m) {
	NpsGateVar* v = m->value;
	ClientRequest out;
	char* type_name;
	int status;

	type_name = abi::__cxa_demangle(v->type().name(), 0, 0, &status);

	LOG_DEBUG("Monitor::subscription_receive.\n");
	out.command = "pubsub_update";
	out.options["fq_name"] = m->fq_name;
	out.options["type"] = type_name;
	out.options["plugin"] = m->orig->name;
	out.options["module"] = m->orig->filename;

	free(type_name);

//	if(v->type() == typeid(string)) {
		out.data = v->get<string>();
//	}

	LOG_DEBUG("DATA: %s\n", out.data.c_str());

	transmit_response(&out);

	return true;
}

bool Monitor::list_publications(ClientRequest* in) {
	ClientRequest out;
	string data;

	out.command = "pubsub_publications";
	out.options["plugin"] = in->options["plugin"];

	PluginCore* pc = context.plugin_manager->plugins[in->options["plugin"]];
	map<string, Publication> pubs = context.publish_subscribe->list_publications(pc);
	map<string, Publication>::iterator iter;

	for(iter = pubs.begin(); iter != pubs.end(); iter++) {
		Publication& pub = iter->second;
		data += pub.fq_name + "|" + pub.description + "|" + boost::lexical_cast<string>(pub.last_update) + "\n";
	}

	out.data = data;

	transmit_response(&out);

	return true;
}

};








