/******************************************************************************
  **
  **  NpsGate.
  **  Copyright (c) 2014, Lance Alt
  **
  **  This file is part of NpsGate.
  **
  **  This program is free software: you can redistribute it and/or modify
  **  it under the terms of the GNU General Public License as published
  **  by the Free Software Foundation, either version 3 of the License, or
  **  (at your option) any later version.
  ** 
  **  This program is distributed in the hope that it will be useful,
  **  but WITHOUT ANY WARRANTY; without even the implied warranty of
  **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  **  GNU General Public License for more details.
  ** 
  **  You should have received a copy of the GNU Lesser General Public License
  **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  ** 
  **
  **  @file npsgate_context.hpp
  **  @author Lance Alt (lancealt@gmail.com)
  **  @date 2014/09/23
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
			boost::filesystem::path npsgate_execute_path;
			boost::filesystem::path plugin_dir;
			boost::filesystem::path main_config_path;
			boost::filesystem::path plugin_config_dir;
	};
}

#endif /* NPSGATE_CONTEXT_HPP_INCLUDED */
