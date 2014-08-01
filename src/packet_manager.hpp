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
**  @file packet_manager.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef PACKET_MANAGER_HPP_H_INCLUDED
#define PACKET_MANAGER_HPP_H_INCLUDED

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>

#include <string>
#include <list>
#include <crafter.h>

#include "npsgate_context.hpp"
#include "logger.h"
#include "plugincore.h"
#include "pluginmanager.h"

using namespace std;
using namespace Crafter;

namespace NpsGate {

	class NpsGateContext;

class PacketManager {
	public:
		PacketManager(const NpsGateContext& c) : context(c) {
			total_packets = total_refs = total_unrefs = total_frees = total_bad_unrefs = 0;
			bytes_in = bytes_out = packets_in = packets_out = bytes_dropped = packets_dropped = 0;
			pthread_mutex_init(&mutex, NULL);
		}
		~PacketManager() {
			print_stats();
		}

		void ref_packet(Packet* p) { 
			pthread_mutex_lock(&mutex);

			map<Packet*, unsigned int>::iterator itr = packet_list.find(p);
			if(itr == packet_list.end()) {
				LOG_TRACE("Adding packet %p to packet store. Ref=1\n", p);
				packet_list[p] = 1;
				total_packets++;
				bytes_in += p->GetSize();
				packets_in++;
			} else {
				LOG_TRACE("Adding reference to packet %p. Ref=%d\n", p, packet_list[p] + 1);
				packet_list[p]++;	
			}
			total_refs++;

			LOG_INFO("Total Packets: %d\n", total_refs - total_unrefs);
			pthread_mutex_unlock(&mutex);
		}

		void unref_packet(Packet* p) {
			pthread_mutex_lock(&mutex);

			map<Packet*, unsigned int>::iterator itr = packet_list.find(p);
			if(itr == packet_list.end()) {
				total_bad_unrefs++;
				for(itr = packet_list.begin(); itr != packet_list.end(); itr++) {
					LOG_WARNING("Packet: %p\n", itr->first);
				}
				LOG_CRITICAL("Bad unref to packet %p not maintained by PacketManager.\n", p);
				return;
			}

			total_unrefs++;

			LOG_TRACE("Removing reference to packet %p. Ref=%d\n", p, packet_list[p] - 1);
			packet_list[p]--;	
			if(packet_list[p] == 0) {
				LOG_TRACE("Reference count of %p is zero. Freeing.\n", p);
				packet_list.erase(p);
				delete p;
				total_frees++;
				bytes_out += p->GetSize();
				packets_out++;
			}
			pthread_mutex_unlock(&mutex);
		}

		unsigned int ref_count(Packet* p) {
			map<Packet*, unsigned int>::iterator iter = packet_list.find(p);
			if(iter == packet_list.end()) {
				return 0;
			}
	
			return iter->second;
		}

		unsigned int packet_count() {
			return packet_list.size();
		}

		friend class Monitor;
	private:
		pthread_mutex_t mutex;
		const NpsGateContext& context;
		map<Packet*, unsigned int> packet_list;

		/* Statistics variables */
		unsigned int total_packets;
		unsigned int total_refs;
		unsigned int total_unrefs;
		unsigned int total_frees;
		unsigned int total_bad_unrefs;
		unsigned long long bytes_in;
		unsigned long long bytes_out;
		unsigned long long bytes_dropped;
		unsigned long long packets_in;
		unsigned long long packets_out;
		unsigned long long packets_dropped;

		void print_stats() {
			LOG_INFO("    PacketManager Stat Count\n");
			LOG_INFO("------------------------------------\n");
			LOG_INFO("    Total Packets:    %u\n", total_packets);
			LOG_INFO("    Total Refs:       %u\n", total_refs);
			LOG_INFO("    Total Unrefs:     %u\n", total_unrefs);
			LOG_INFO("    Total Frees:      %u\n", total_frees);
			LOG_INFO("    Total Bad Unrefs: %u\n", total_bad_unrefs);
		}
};

}

#endif /* PACKET_MANAGER_HPP_H_INCLUDED */
