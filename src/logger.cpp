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
**  @file logger.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <execinfo.h>
#include <cxxabi.h>

#include <libconfig.h++>

#include "logger.h"
#include "plugincore.h"
//#include "npsgate_context.hpp"

using namespace std;

namespace NpsGate {

void Logger::init(FILE* f) {
	output = f;
}

void Logger::config(Config* cfg, const NpsGateContext& c) {
	string log_filter;

	context = &c;

	cfg->lookupValue("NpsGate.halt_critical", halt_critical);
	cfg->lookupValue("NpsGate.log_level", log_level);

	LOG_INFO("Logger initialized. halt_critical=%d, log_level=%d\n", halt_critical, log_level);

	if(cfg->lookupValue("NpsGate.log_filter", log_filter)) {
		LOG_INFO("Found a log_filter setting. Parsing.\n");
		size_t last_delim = 0;
		size_t delim_pos;
		log_filter += "|";
		while(string::npos != (delim_pos = log_filter.find_first_of('|', last_delim))) {
			filters[log_filter.substr(last_delim, delim_pos-last_delim)] = "hello";
			LOG_INFO("Adding filter string: %s\n", log_filter.substr(last_delim, delim_pos-last_delim).c_str());
			last_delim = delim_pos + 1;
		}
	}

	if(cfg->lookupValue("NpsGate.log_plugin_filter", log_filter)) {
		LOG_INFO("Found a log_plugin_filter setting. Parsing.\n");
		size_t last_delim = 0;
		size_t delim_pos;
		log_filter += "|";
		while(string::npos != (delim_pos = log_filter.find_first_of('|', last_delim))) {
			filters[log_filter.substr(last_delim, delim_pos-last_delim)] = "hello";
			LOG_INFO("Adding plugin filter string: %s\n", log_filter.substr(last_delim, delim_pos-last_delim).c_str());
			last_delim = delim_pos + 1;
		}
	}

}

void Logger::log(LogLevels level, pthread_t tid, const char* file, int line, const char* format, ...) {
	char buffer[1024];
	va_list args;
	const char* l;
	const char* p;
	string pname;
	int offset;

	/* Strip off any leading ../ from the filename. */
	p = strrchr(file, '/');
	if(p) {
		file =  p + 1;
	}

	if(tid && context->plugin_manager) {
		pname = context->plugin_manager->get_name_from_thread(tid);
	}
	if(pname == "") {
		pname = "CORE";
	}

	if(!filter(level, file, pname)) {
		return;
	}

	switch(level) {
		case CRITICAL:	l = "CRIT ";	break;
		case WARNING:	l = "WARN ";	break;
		case INFO:		l = "INFO ";	break;
		case DEBUG:		l = "DEBUG";	break;
		case TRACE:		l = "TRACE";	break;
		case TODO:		l = "*TODO"; break;
	}

	va_start(args, format);
	offset = snprintf(buffer, 1024, "%s (%s:%d) <%s>: ", l, file, line, pname.c_str());
	vsnprintf(buffer + offset, 1024 - offset, format, args);
	va_end(args);

	fputs(buffer, output);
	vector<LoggerHookData>::iterator iter = hooks.begin();
	while(iter != hooks.end()) {
		(*iter).hook(level, buffer, (*iter).data);
		iter++;
	}

	if(level == CRITICAL && halt_critical) {
		fprintf(output, "**********************************************\n");
		fprintf(output, "** CRITICAL EXCEPTION RECEIVED, TERMINATING **\n");
		fprintf(output, "**********************************************\n");
		print_backtrace();
		kill(getpid(), SIGINT);
	}
}

bool Logger::filter(LogLevels level, const char* file, const string& plugin) {
	if(log_level < level) {
		return false;
	}
	if(level == CRITICAL || level == WARNING) {
		return true;
	}
	if(filters.size() == 0 && plugin_filters.size() == 0) {
		return true;
	}
	if(filters.end() != filters.find(file)) {
		return true;
	}
	if(plugin_filters.end() != plugin_filters.find(plugin)) {
		return true;
	}
	
	return false;
}

void Logger::print_backtrace() {
	static void* bt_buffer[100];
	static int bt_size;
	static char** funcs;
	static char* demangle = NULL;
	static int status;
	static size_t buf_size = 0;
	char* openp, *closep;

	bt_size = backtrace(bt_buffer, 100);
	funcs = backtrace_symbols(bt_buffer, bt_size);

	// Start on #2 so we don't include the print_backtrace() and log() in the backtrace.
	fprintf(output, "Backtrace:\n");
	for(int i = 2; i < bt_size; i++) {
		openp = strchr(funcs[i], '(');
		closep = strchr(openp, '+');

		*openp = '\0';
		if(closep) {
			*closep = '\0';
		}

		demangle = abi::__cxa_demangle(openp + 1, demangle, &buf_size, &status);
		switch(status) {
			case 0:
				fprintf(output, "%3d: %s   %s\n", i - 2, funcs[i], demangle);
				break;
			case -2:
				fprintf(output, "%3d: %s   %s\n", i - 2, funcs[i], openp + 1);
				break;
			case -1:
			case -3:
				fprintf(output, "%3d: ERROR\n", i - 2);
				break;
		}
	}
}

void Logger::add_hook(LoggerHook h, void* d) {
	LoggerHookData hd;

	hd.hook = h;
	hd.data = d;
	hooks.push_back(hd);
}

void Logger::remove_hook(LoggerHook, void*) {
}


FILE* Logger::output = stderr;
int Logger::log_level = 3;
bool Logger::halt_critical = true;
map<string, string> Logger::filters;
map<string, string> Logger::plugin_filters;
vector<LoggerHookData> Logger::hooks;
const NpsGateContext* Logger::context;

}

