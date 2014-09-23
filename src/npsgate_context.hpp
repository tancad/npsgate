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
**  @file npsgate_context.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef NPSGATE_CONTEXT_HPP_INCLUDED
#define NPSGATE_CONTEXT_HPP_INCLUDED

#include <libconfig.h++>
#include <boost/filesystem.hpp>

#include "packet_manager.hpp"
#include "publish_subscribe.h"
#include "pluginmanager.h"
//#include "monitor/monitor.hpp"


using namespace libconfig;

namespace NpsGate {

	class Monitor;

	class NpsGateContext {
		public:
			PluginManager* plugin_manager;
			PublishSubscribe* publish_subscribe;
			PacketManager* packet_manager;
			Config* config;
			Monitor* monitor;
			boost::filesystem::path library_path;
			boost::filesystem::path config_path;
	};
}

#endif /* NPSGATE_CONTEXT_HPP_INCLUDED */
