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
**  @file monitor_comms.cpp
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

int Monitor::listen_inet(string port) {
	struct addrinfo hints, *servinfo, *p;
	int fd, yes = 1, rval;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((rval = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0) {
		LOG_CRITICAL("getaddrinfo() failed. Reason: %s\n", gai_strerror(rval));
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			continue;
		}
		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			LOG_CRITICAL("setsockopt(SO_REUSEADDR) failed. Reason: %s\n", strerror(errno));
		}
		if(bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
			LOG_WARNING("Bind failed. Reason: %s\n", strerror(errno));
			continue;
		}

		break;
	}

	if(p == NULL) {
		LOG_CRITICAL("Failed to create INET socket.\n");
		return -1;
	}

	freeaddrinfo(servinfo);

	if(listen(fd, 5) == -1) {
		LOG_CRITICAL("Failed to listen on INET socket. Reason: %s\n", strerror(errno));
	}

	return fd;
}

bool Monitor::process_request(int fd) {
	char buffer[256];
	char* line, *endptr;
	size_t len = 256;
	int rval;

	map<int,ClientRequest*>::iterator iter = client_requests.find(fd);

	rval = recv(fd, buffer, len, MSG_DONTWAIT);
	if(rval <= 0) {
		if(iter != client_requests.end()) {
			delete iter->second;
			client_requests.erase(iter);
			LOG_DEBUG("Removing ClientRequest.\n");
		}
		return false;
	}

	line = buffer;
	endptr = strchr(line, '\n');

	if(endptr == NULL) {
		LOG_TRACE("Partial line received, saving for next time.\n");
	}
	*endptr = '\0';
	endptr++;


	while(endptr) {
		process_line(fd, line);

		line = endptr;
		endptr = strchr(line, '\n');
		if(endptr) {
			*endptr = '\0';
			endptr++;
		}
	}

	if((buffer - endptr) < rval) {
		LOG_TRACE("Saving partial line: %s\n", line);
	}

	return true;
}

bool Monitor::process_line(int fd, char* line) {
	char* command, *endptr, *key, *value;
	ClientRequest* req;

	LOG_TRACE("Received line from client: '%s'\n", line);

	map<int,ClientRequest*>::iterator iter = client_requests.find(fd);

	if(iter == client_requests.end()) {
		req = new ClientRequest();
		client_requests[fd] = req;
	} else {
		req = iter->second;
	}

	switch(req->state) {
		case CLIENT_STATE_NEW:
			command = strtok_r(line, " \t\r\n", &endptr);
			if(!command) {
				LOG_WARNING("Could not parse command!\n");
			}

			LOG_TRACE("Client sent command: %s\n", command);
			req->command = command;
			req->state = CLIENT_STATE_OPTIONS;
			break;
		case CLIENT_STATE_OPTIONS:
			if(*line == '\0') {
				LOG_TRACE("End of options.\n");
				req->state = CLIENT_STATE_DATA;
				break;
			}

			key = strtok_r(line, "=", &endptr);
			value = strtok_r(NULL, "=", &endptr);
			if(key == NULL || value == NULL) {
				LOG_WARNING("Received invalid key/value pair. Ignoring.\n");
				break;
			}

			LOG_TRACE("Client sent option: %s=%s\n", key, value);
			req->options[key] = value;
			break;
		case CLIENT_STATE_DATA:
			if(!strcmp(line, "\v")) {
				LOG_TRACE("End of data.\n");
				req->state = CLIENT_STATE_DONE;
				break;
			}

			LOG_TRACE("Client sent data: %s\n", line);
			req->data += line;
			req->data += "\n";
			break;
		default:
			LOG_WARNING("Client is in an unknown state!\n");
			break;
	}

	if(req->state != CLIENT_STATE_DONE) {
		return true;
	}

	LOG_DEBUG("Complete Command: '%s'\n", req->command.c_str());

	map<string, bool (Monitor::*)(ClientRequest*)>::iterator handler = msg_handlers.find(req->command);
	if(handler != msg_handlers.end()) {
		bool (Monitor::*func)(ClientRequest*) = handler->second;
		(this->*func)(req);
	} else {
		LOG_WARNING("Unhandled command: %s\n", req->command.c_str());
	}

	client_requests.erase(fd);
	delete req;

	return true;
}

void Monitor::transmit_response(ClientRequest* cr) {
	for(int i = 0; i < MONITOR_MAX_CLIENTS; i++) {
		if(-1 != clients[i]) {
			if(!transmit_response(clients[i], cr)) {
				LOG_INFO("Transmission failed. Closing file descriptor.\n");
				close(clients[i]);
				clients[i] = -1;
			}
		}
	}
}

bool Monitor::transmit_response(int fd, ClientRequest* cr) {
	char buffer[1024];	
	int len;

	len =snprintf(buffer, 1024, "%s response\n", cr->command.c_str());
	if(-1 == write(fd, buffer, len)) {
		LOG_WARNING("write() failed. Reason: %s\n", strerror(errno));
		return false;
	}

	map<string,string>::iterator iter = cr->options.begin();
	for(;iter != cr->options.end(); ++iter) {
		len = snprintf(buffer, 1024, "%s=%s\n", iter->first.c_str(), iter->second.c_str());
		if(-1 == write(fd, buffer, len)) {
			LOG_WARNING("write() failed. Reason: %s\n", strerror(errno));
			return false;
		}
	}

	if(-1 == write(fd, "\n", 1)) {
		LOG_WARNING("write() failed. Reason: %s\n", strerror(errno));
		return false;
	}

	len = cr->data.length();
	if(-1 == write(fd, cr->data.c_str(), len)) {
		LOG_WARNING("write() failed. Reason: %s\n", strerror(errno));
		return false;
	}

	len = snprintf(buffer, 1024, "\v\n");
	if(-1 == write(fd, buffer, len)) {
		LOG_WARNING("write() failed. Reason: %s\n", strerror(errno));
		return false;
	}

	return true;
}

}

