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
**  @file logger.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef LOGGER_HPP_INCLUDED
#define LOGGER_HPP_INCLUDED

#include <stdio.h>
#include <stdarg.h>
#include <map>
#include <libconfig.h++>

//#include "npsgate_context.hpp"

#define LOG_CRITICAL(format, ...) NpsGate::Logger::log(CRITICAL, pthread_self(), __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) NpsGate::Logger::log(WARNING, pthread_self(), __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) NpsGate::Logger::log(INFO, pthread_self(), __FILE__, __LINE__, format, ##__VA_ARGS__)

#ifdef ENABLE_DEBUG
#define LOG_DEBUG(format, ...) NpsGate::Logger::log(DEBUG, pthread_self(), __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_TRACE(format, ...) NpsGate::Logger::log(TRACE, pthread_self(), __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_TODO(format, ...) NpsGate::Logger::log(TODO, pthread_self(), __FILE__, __LINE__, format, ##__VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#define LOG_TRACE(format, ...)
#define LOG_TODO(format, ...)
#endif



using namespace std;
using namespace libconfig;


namespace NpsGate {

enum LogLevels {
	CRITICAL=1, WARNING, INFO, DEBUG, TRACE, TODO
};

class PluginCore;
class NpsGateContext;

class Logger {
	public:
		static void init(FILE* f);
		static void config(Config* cfg, const NpsGateContext& c);

		static void log(LogLevels level, pthread_t tid, const char* file, int line, const char* format, ...);

	private:
		static FILE* output;
		static int log_level;
		static bool halt_critical;
		static map<string, string> filters;
		static map<string, string> plugin_filters;
		static const NpsGateContext* context;

		static bool filter(LogLevels, const char*, const string&);
};

}

#endif /* LOGGER_HPP_INCLUDED */
