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
**  @file npsgate_lwip.h
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef NPSGATELWIP_H
#define NPSGATELWIP_H
#include <pcap.h>
#include <crafter.h>
#include <unistd.h> 
#include <map>
#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "split_tcp.h"

#include "lwip/tcp.h"

struct QueuedPbuf {
	tcp_pcb* spcb;
	tcp_pcb* dpcb;
	list<pbuf*> pbuf_list;
};

class SplitTCP;

class NpsGateLWIP {
	public:
		NpsGateLWIP(SplitTCP*, uint16_t);
		~NpsGateLWIP();
		bool inject_packet(Packet* p);
		bool send_data(uint8_t* data, int len);
		bool timeout();

		string stats();

		uint8_t* xmit_buffer;
	private:
		struct ip_addr ipaddr, netmask, gw;
		struct netif if_in, if_out;
		SplitTCP* stcp;

		uint16_t mtu;

		static err_t accept_connection(void* arg, tcp_pcb* newpcb,  err_t err);
		static err_t connected(void* arg, tcp_pcb* pcb, err_t err);
		static err_t recv_data(void* arg, tcp_pcb* pcb, pbuf* p, err_t err);
		static err_t sent_data(void* arg, tcp_pcb* pcb, uint16_t len);
		static void error(void* arg, err_t err);
		static void transmit_pbuf(tcp_pcb* spcb, tcp_pcb* dpcb, pbuf* p);
		static void transmit_queued_pbufs(tcp_pcb*);

		list<tcp_pcb*> pcb_list;
		static list<tcp_pcb*> close_list;
		static map<tcp_pcb*, QueuedPbuf> queued_pbufs;

		static err_t lwip_driver_init(struct netif* netif);
		static void lwip_driver_input(struct netif* netif, Packet* pkt);
		static err_t lwip_driver_output(struct netif* netif, struct pbuf* p);
		static err_t lwip_driver_output2(struct netif* netif, struct pbuf* q, ip_addr_t* ipdaddr);
		
		static string lpcb_stats(tcp_pcb_listen* pcb);
		static string pcb_stats(tcp_pcb* pcb);
		static unsigned int lwip_listening_socket_count();
		static unsigned int lwip_active_socket_count();

};

extern "C" {
	void NPSGATE_LOG(const char* format, ...);
};
#endif /* NPSGATELWIP_H */
