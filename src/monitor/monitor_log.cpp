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
**  @file monitor_log.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include "monitor/monitor.hpp"
namespace NpsGate {

void Monitor::log_init() {
	Logger::add_hook((LoggerHook)Monitor::logger_hook, this);
}

void Monitor::log(LogLevels level, const char* msg) { 
	ClientRequest* out = new ClientRequest();
	
	out->command = "log";
	out->data = msg;
	for(int i = 0; i < MONITOR_MAX_CLIENTS; i++) {
		if(-1 != clients[i]) {
			transmit_response(clients[i], out);
		}
	}
	delete out;
}

void Monitor::logger_hook(LogLevels l, const char* msg, Monitor* m) {
	m->log(l, msg);
}

}
