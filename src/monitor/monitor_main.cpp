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
**  @file monitor_main.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "npsgate_context.hpp"
#include "pluginmanager.h"
#include "monitor.hpp"

namespace NpsGate {

Monitor::Monitor(NpsGateContext& c, MonitorType t, string s) : PluginCore(c, "Monitor"), context(c) {
	type = t;
	str = s;

	uptime = time(NULL);

	for(int i=0; i<MONITOR_MAX_CLIENTS; i++) {
		clients[i] = -1;
	}

	fd = listen_inet(s);

	log_init();

	/* Set the command handlers */
	msg_handlers["graph"] = &Monitor::generate_plugin_graph;
	msg_handlers["stats"] = &Monitor::generate_plugin_stats;
	msg_handlers["config"] = &Monitor::process_config;
	msg_handlers["pubsub"] = &Monitor::pubsub;
	msg_handlers["pubsub_subscribe"] = &Monitor::subscribe_cmd;
	msg_handlers["pubsub_unsubscribe"] = &Monitor::unsubscribe_cmd;
	msg_handlers["pubsub_publications"] = &Monitor::list_publications;
	msg_handlers["pubsub_publication_list"] = &Monitor::list_plugin_publications;
	msg_handlers["pubsub_subscription_list"] = &Monitor::list_plugin_subscriptions;
}

Monitor::~Monitor() {
	unlink(str.c_str());
	close(fd);
	for(int i=0; i<MONITOR_MAX_CLIENTS; i++) {
		if(clients[i] != -1) {
			close(clients[i]);
			clients[i] = -1;
		}
	}
}

void Monitor::main(const bool* exit_flag) {
	struct timeval tv;
	fd_set fds;
	int max_fd = fd;
	int rval, i;

	while(*exit_flag == false) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		for(i=0; i<MONITOR_MAX_CLIENTS; i++) {
			if(clients[i] != -1) {
				FD_SET(clients[i], &fds);
				if(max_fd < clients[i]) {
					max_fd = clients[i];
				}
			}
		}

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		rval = select(max_fd + 1, &fds, NULL, NULL, &tv);
		if(rval > 0) {
			for(i=0; i<MONITOR_MAX_CLIENTS; i++) {
				if(clients[i] != -1 && FD_ISSET(clients[i], &fds)) {
					if(false == process_request(clients[i])) {
						close(clients[i]);
						clients[i] = -1;
					}
				}
			}
			if(FD_ISSET(fd, &fds)) {
				for(i=0; i<MONITOR_MAX_CLIENTS; i++) {
					if(clients[i] == -1) {
						break;
					}
				}
				if(i < MONITOR_MAX_CLIENTS) {
					clients[i] = accept(fd, NULL, NULL);
					LOG_INFO("Acepted new connection.\n");
				} else {
					close(accept(fd, NULL, NULL));
					LOG_INFO("Too many connections! Rejecting new connection.\n");
				}
			}
		} else if(rval == 0) {
			// timeout
		} else if(rval == -1 && errno == EINTR) {
			// interrupted by signal
			break;
		} else {
			LOG_CRITICAL("Select error! Error: %s\n", strerror(errno));
			break;
		}

		JobQueueItem* item = input_queue.Dequeue(0);
		if(item && item->type == MESSAGE) {
			LOG_DEBUG("Monitor::main received message.\n");
			subscription_receive(item->message);
		}
	
	}


}
}

