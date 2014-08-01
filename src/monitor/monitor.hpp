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
**  @file monitor.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef MONITOR_HPP_INCLUDED
#define MONITOR_HPP_INCLUDED

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "npsgate_context.hpp"
#include "pluginmanager.h"
#include "plugincore.h"

namespace NpsGate {

	enum MonitorType { MONITOR_FIFO, MONITOR_UNIX, MONITOR_INET };
	enum ClientState { CLIENT_STATE_NEW, CLIENT_STATE_OPTIONS, CLIENT_STATE_DATA, CLIENT_STATE_DONE };
	struct ClientRequest {
		ClientState state;
		string request;
		string command;
		map<string,string> options;
		string data;
	};

	class Monitor;

	typedef bool (Monitor::*MessageHandler)(ClientRequest*);
	
	class Monitor : public PluginCore {
		public:
			Monitor(NpsGateContext& c, MonitorType t, string s);
			~Monitor();

			void main(const bool* exit_flag);

		private:
			int listen_inet(string);
			bool process_request(int fd);
			bool process_line(int fd, char* line);
			void transmit_response(ClientRequest* cr);
			bool transmit_response(int fd, ClientRequest* cr);

			/* Logging */
			void log_init();
			void log(LogLevels level, const char* msg);
			static void logger_hook(LogLevels l, const char* msg, Monitor* m);
			
			/* Plugin Related Functions */
			bool generate_plugin_graph(ClientRequest* in); 
			bool generate_plugin_stats(ClientRequest* in);
			bool process_config(ClientRequest* in);

			/* Publish Subscribe */
			bool pubsub(ClientRequest* req);
			bool subscribe_cmd(ClientRequest* req);
			bool unsubscribe_cmd(ClientRequest* req);
			bool list_subscribed();
			bool list_all();
			bool subscription_receive(Message* m);
			bool list_publications(ClientRequest*);
			bool list_plugin_publications(ClientRequest* req);
			bool list_plugin_subscriptions(ClientRequest* req);

			map<string, bool (Monitor::*)(ClientRequest*)> msg_handlers;
			static const int MONITOR_MAX_CLIENTS=5;
			NpsGateContext& context;
			MonitorType type;
			string str;
			int fd;
			int clients[MONITOR_MAX_CLIENTS];
			map<int,ClientRequest*> client_requests;
			time_t uptime;
	};
}

#endif /* MONITOR_HPP_INCLUDED */
