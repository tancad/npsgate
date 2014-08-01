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
**  @file npsgate_lwip.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <pcap.h>
#include <crafter.h>
#include <unistd.h> 
#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "split_tcp.h"
#include "npsgate_lwip.h"

#include "lwip/def.h"
#include "lwip/sys.h"
#include "lwip/arch.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/tcp_impl.h"
#include "lwip/snmp.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/opt.h"

struct ListenSocket {
	tcp_pcb* lpcb;
	Packet* p;
};

extern "C" {
	void NPSGATE_LOG(const char* format, ...) {
		char buffer[1024];
		va_list args;
		const char* l;
		string pname;
		int offset;
		LogLevels level = DEBUG;
	
		switch(level) {
			case CRITICAL:	l = "CRIT ";	break;
			case WARNING:	l = "WARN ";	break;
			case INFO:		l = "INFO ";	break;
			case DEBUG:		l = "DEBUG";	break;
			case TRACE:		l = "TRACE";	break;
			case TODO:		l = "*TODO"; break;
		}
	
		va_start(args, format);
		offset = snprintf(buffer, 1024, "%s : ", l);
		vsnprintf(buffer + offset, 1024 - offset, format, args);
		va_end(args);
	
		fputs(buffer, stdout);
		vector<LoggerHookData>::iterator iter = Logger::hooks.begin();
		while(iter != Logger::hooks.end()) {
			(*iter).hook(level, buffer, (*iter).data);
			iter++;
		}

}


};

NpsGateLWIP::NpsGateLWIP(SplitTCP* s, uint16_t m) {
	stcp = s;
	mtu = m;
	xmit_buffer = (uint8_t*)malloc(m);

	/* Each interfaces needs an IP address associated to it, for NpsGate usage, we
	   don't really care what it is. */
	IP4_ADDR(&gw, 192, 0, 0, 1);
	IP4_ADDR(&ipaddr, 192, 0, 0, 1);
	IP4_ADDR(&netmask, 255, 0, 0, 0);

	LOG_DEBUG("Initializing NpsGateLWIP plugin.\n");

	lwip_init();

	/* Add two LWIP network interfaces, one for input, one for output */
	netif_add(&if_in, &ipaddr, &netmask, &gw, NULL, lwip_driver_init, ip_input);
	netif_add(&if_out, &ipaddr, &netmask, &gw, NULL, lwip_driver_init, ip_input);

	/* Bring the interfaces up */
	netif_set_up(&if_in);
	netif_set_up(&if_out);

	/* Set the output interface as the default */
	netif_set_default(&if_out);

	if_in.mtu = mtu;
	if_out.mtu = mtu;

	/* Since LWIP is written in C, it requires us to use static member functions as
	   callback functions. A pointer to the instance of this class is saved in the
	   state variable to provide a means to jump back to the C++ classes. */
	if_in.state = this;
	if_out.state = this;
}

NpsGateLWIP::~NpsGateLWIP() {
	map<tcp_pcb*, QueuedPbuf>::iterator iter = queued_pbufs.begin();
	while(iter != queued_pbufs.end()) {
		list<pbuf*>::iterator p = iter->second.pbuf_list.begin();
		while(p != iter->second.pbuf_list.end()) {
			pbuf_free(*p);
			p = iter->second.pbuf_list.erase(p);
		}
		iter++;
	}
	free(xmit_buffer);

	netif_remove(&if_in);
	netif_remove(&if_out);
}

// TODO: This needs to be done better.
static tcp_pcb* lpcb;

bool NpsGateLWIP::inject_packet(Packet* p) {
	IP* ip = p->GetLayer<IP>();
	TCP* tcp = p->GetLayer<TCP>();
	ListenSocket* l;
	ip_addr sipaddr, dipaddr;

	if(!ip || !tcp) {
		LOG_WARNING("Packet does not have a valid TCP/IP header.\n");
		return false;
	}

	/* When a new TCP packet is received, there are two possible options:
	   1. The packet has only the SYN flag set. In this case, we assume the packet is
	   attempting to establish a new connection. LWIP would reject this packet, since
	   there is no listening socket, so first create a new listening socket before
	   passing the SYN packet to LWIP.
	   2. The packet does not have the SYN flag set. In this case, the packet should
	   be associated with an already established connection, so we just pass it directly
	   to LWIP. */
	if(tcp->GetFlags() == TCP::SYN) {
		lpcb = tcp_new();
		unsigned int a, b, c, d;

		/* When this function returns, the packet is unneeded so the SplitTCP class
		   frees the packet, except in the case where this packet is creating a new
		   connection. In this case, the packet can not be freed until the accept_connection
		   function is called, so we just make a copy of it here. */
		p = new Packet(*p);

		LOG_TRACE("New Connection: %s:%u => %s:%u\n",
				ip->GetSourceIP().c_str(), tcp->GetSrcPort(),
				ip->GetSourceIP().c_str(), tcp->GetDstPort());

		// Convert IP addresses to ip_addr struct needed by lwip
		sscanf(ip->GetDestinationIP().c_str(), "%u.%u.%u.%u", &a, &b, &c, &d);
		IP4_ADDR(&dipaddr, a,b,c,d);
		sscanf(ip->GetSourceIP().c_str(), "%u.%u.%u.%u", &a, &b, &c, &d);
		IP4_ADDR(&sipaddr, a,b,c,d);

		l = new ListenSocket();

		// Create a new listening socket so that we can accept this
		// incoming SYN packet.
		ip_set_option(lpcb, SOF_REUSEADDR);
		err_t rval = tcp_bind(lpcb, &dipaddr, tcp->GetDstPort());
		switch(rval) {
			case ERR_OK:
				l->lpcb = tcp_listen(lpcb);
				if(!l->lpcb) {
					LOG_WARNING("tcp_listen(): failed to create new listen socket.\n");
					delete l;
					return false;
				}
				l->p = p;
				tcp_accept(l->lpcb, accept_connection);
				tcp_arg(l->lpcb, l);
				break;
			case ERR_BUF:
				LOG_WARNING("tcp_bind(): failed to allocate a new port.\n");
				tcp_abort(lpcb);
				break;
			case ERR_USE:
				LOG_WARNING("tcp_bind(): port already in use.\n");
				tcp_abort(lpcb);
				break;
			case ERR_VAL:
				LOG_WARNING("tcp_bind(): can only bind in state CLOSED.\n");
				tcp_abort(lpcb);
				break;
			default:
				LOG_WARNING("tcp_bind() returned an unknown error code.\n");
				tcp_abort(lpcb);
		}
	}

	lwip_driver_input(&if_in, p);

	return true;
}

bool NpsGateLWIP::send_data(uint8_t* data, int len) {
	Packet* p = new Packet();

	/* This doesn't return any success/failure indication. Hope it works. */
	p->PacketFromIP(data, len);

	stcp->forward_packet(stcp->get_default_output(), p);

	return true;
}

err_t NpsGateLWIP::accept_connection(void* arg, tcp_pcb* spcb,  err_t err) {
	ListenSocket* l = (ListenSocket*)arg;
	Packet* p = l->p;
	IP* ip = p->GetLayer<IP>();
	TCP* tcp = p->GetLayer<TCP>();
	char* dip = strdup(ip->GetDestinationIP().c_str());
	char* sip = strdup(ip->GetSourceIP().c_str());
	unsigned int a, b, c, d;
	ip_addr sipaddr, dipaddr;
	tcp_pcb* dpcb;

	if(err != ERR_OK) {
		LOG_WARNING("accept_connection(): err != ERR_OK\n");
		return err;
	}
	
	// Convert IP addresses to ip_addr struct needed by lwip
	sscanf(dip, "%u.%u.%u.%u", &a, &b, &c, &d);
	IP4_ADDR(&dipaddr, a,b,c,d);
	sscanf(sip, "%u.%u.%u.%u", &a, &b, &c, &d);
	IP4_ADDR(&sipaddr, a,b,c,d);

	// Create a new outgoing socket to the destination IP address
	dpcb = tcp_new();

	if(ERR_OK != tcp_bind(dpcb, &sipaddr, 0)) {
		LOG_CRITICAL("tcp_bind() did not return ERR_OK\n");
	}

	if(ERR_OK != tcp_connect(dpcb, &dipaddr, tcp->GetDstPort(), connected)) {
		LOG_CRITICAL("tcp_connect() did not return ERR_OK\n");
	}

	tcp_err(dpcb, error);


	/* Set the recv and error functions for the new pcb. */
	tcp_recv(spcb, recv_data);
	tcp_sent(spcb, sent_data);
	tcp_err(spcb, error);

	// Close the listening socket
	LOG_DEBUG("Closing TCP listening socket.\n");
	tcp_close(l->lpcb);
	delete l;

	/* Pairs the two sockets using the callback arg */
	tcp_arg(spcb, dpcb);
	tcp_arg(dpcb, spcb);

	free(dip);
	free(sip);

	delete p;

	return ERR_OK;
}

err_t NpsGateLWIP::connected(void* arg, tcp_pcb* pcb, err_t err) {
	if(err != ERR_OK) {
		LOG_WARNING("connected(): err != ERR_OK\n");
		return err;
	}

	/* We now have a completed SplitTCP connection. Set the recv and error functions
	   for the new pcb, then try sending any queued data we might have already. */
	tcp_recv(pcb, recv_data);
	tcp_sent(pcb, sent_data);
	tcp_err(pcb, error);

	transmit_queued_pbufs(pcb);

	return ERR_OK;
}

err_t NpsGateLWIP::recv_data(void* arg, tcp_pcb* pcb, pbuf* p, err_t err) {
	tcp_pcb* other_end = (tcp_pcb*) arg;

	if(err != ERR_OK) {
		LOG_WARNING("recv_data(): err != ERR_OK\n");
		return err;
	}

	/* The pbuf will be NULL when the remote host closed the connection. Close the pcb,
	   then remove this pcb from the other paired pcb. Check to see if the other end has
	   outstanding data waiting to be sent. If no oustanding data, then close the other end. */
	if(p == NULL) {
		LOG_DEBUG("Remote connection closed.\n");
		tcp_close(pcb);
		if(other_end) {
			tcp_arg(other_end, NULL);
			map<tcp_pcb*, QueuedPbuf>::iterator iter = queued_pbufs.find(other_end);
			if(iter->second.pbuf_list.empty()) {
				tcp_close(other_end);
			}
		}
		return ERR_OK;
	}

	if(p->len == 0) {
		LOG_DEBUG("Received zero length data.\n");
		return ERR_OK;
	}
	
	/* Acknowledge to LWIP that we have received this data so the receive window
	   can be opened back up. Then queue the pbuf for transmission. */
	tcp_recved(pcb, p->tot_len);
	transmit_pbuf(pcb, other_end, p);

	return ERR_OK;
}

err_t NpsGateLWIP::sent_data(void* arg, tcp_pcb* pcb, uint16_t len) {
	transmit_queued_pbufs(pcb);
	return ERR_OK;
}

void NpsGateLWIP::error(void* arg, err_t err) {
	tcp_pcb* other_end = (tcp_pcb*) arg;

	if(other_end == NULL) {
		LOG_WARNING("Received a NULL arg. Doing nothing.\n");
		return;
	}

	/* If the other end is a listening socket, this means that the connect
	   to the remote server failed. In this case, just close the listening socket. */
	if(other_end->state == LISTEN) {
		LOG_DEBUG("***** Remote connect failed. Closing listening socket.\n");
		tcp_arg((tcp_pcb*)other_end->callback_arg, NULL);
		tcp_close(other_end);
		return;
	}

	tcp_arg(other_end, NULL);
	LOG_DEBUG("***** TCP Error has occurred.\n");

	tcp_abort(other_end);
}

void NpsGateLWIP::transmit_pbuf(tcp_pcb* spcb, tcp_pcb* dpcb, pbuf* p) {
	map<tcp_pcb*, QueuedPbuf>::iterator iter;
	iter = queued_pbufs.find(dpcb);

	/* If there are already queued pbufs for this connection, just append this new
	   data to the end of the pbuf chain. If there isn't data, create a new
	   QueuedPbuf object to the queued pbuf map. */
	if(iter == queued_pbufs.end()) {
		QueuedPbuf qpb;

		qpb.dpcb = dpcb;
		qpb.spcb = spcb;
		qpb.pbuf_list.push_back(p);

		queued_pbufs[dpcb] = qpb;
	} else {
		iter->second.pbuf_list.push_back(p);
	}

	/* Attempt to transmit data now. */
	transmit_queued_pbufs(dpcb);
}

void NpsGateLWIP::transmit_queued_pbufs(tcp_pcb* pcb) {
	map<tcp_pcb*, QueuedPbuf>::iterator iter = queued_pbufs.find(pcb);
	tcp_pcb* other_end = (tcp_pcb*)pcb->callback_arg;

	/* Nothing to send */
	if(iter == queued_pbufs.end()) {
		return;
	}
	
	/* Check if send buffer is zero, if so, we can't send anything right now */
	int len = tcp_sndbuf(pcb);
	if(len == 0) {
		return;
	}

	list<pbuf*>& pbuf_list = iter->second.pbuf_list;
	list<pbuf*>::iterator p = pbuf_list.begin();

	while(p != pbuf_list.end()) {
		if((*p)->tot_len != (*p)->len) {
			LOG_WARNING("pbuf tot_len != len, coalescing pbuf.  tot_len=%u, len=%u\n",(*p)->tot_len, (*p)->len);
			(*p) = pbuf_coalesce(*p, PBUF_RAW);
		}
		
		if(ERR_OK != tcp_write(pcb, (*p)->payload, (*p)->tot_len, TCP_WRITE_FLAG_COPY)) {
		//	LOG_DEBUG("tcp_write() failed for %u bytes. Will try again later.\n", len);
			break;
		}

		pbuf_free(*p);
		p = pbuf_list.erase(p);
	}
	tcp_output(pcb);

	if(other_end == NULL && pbuf_list.empty()) {
		LOG_DEBUG("Other end is closed, and we have finished transmitting data. Closing connection.\n");
		close_list.push_back(pcb);
	}
}

err_t NpsGateLWIP::lwip_driver_init(struct netif* netif) {
	if(!netif) {
		LOG_CRITICAL("netif == NULL\n");
		return ERR_MEM;
	}

	NETIF_INIT_SNMP(netif, snmpifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

	netif->name[0] = 'n';
	netif->name[1] = 'g';
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP;
	netif->output = NpsGateLWIP::lwip_driver_output2;
	netif->linkoutput = NpsGateLWIP::lwip_driver_output;

	return ERR_OK;
}

void NpsGateLWIP::lwip_driver_input(struct netif* netif, Packet* pkt) {
	struct pbuf* p;
	const byte* pkt_ptr;
	uint16_t len;

	len = pkt->GetSize();

	if(len > netif->mtu) {
		LOG_WARNING("Received packet larger than MTU! Size: %u (MTU: %u). Packet dropped!\n", len, netif->mtu);
		return;
	}

	p = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
	p->next = NULL;
	if(!p) {
		LOG_WARNING("Failed to allocate pbuf with size: %u. Packet dropped!\n", len);
		return;
	}

	pkt_ptr = pkt->GetRawPtr();
	memcpy(p->payload, pkt_ptr, p->len);
	
	if(ERR_OK != netif->input(p, netif)) {
		pbuf_free(p);
		LOG_WARNING("Failed to input packet to lwip.\n");
	}
}

err_t NpsGateLWIP::lwip_driver_output(struct netif* netif, struct pbuf* p) {
	NpsGateLWIP* nglwip = (NpsGateLWIP*)netif->state;

	if(p->tot_len > netif->mtu) {
		LOG_WARNING("Sending packet larger than MTU! Size: %u (MTU: %u). Dropping packet!\n", p->tot_len, netif->mtu);
		return ERR_MEM;
	}

	/* Copy the packet from the pbuf in to our transmit buffer. */
	pbuf_copy_partial(p, nglwip->xmit_buffer, p->tot_len, 0);
	nglwip->send_data(nglwip->xmit_buffer, p->tot_len);

	return ERR_OK;
}

err_t NpsGateLWIP::lwip_driver_output2(struct netif* netif, struct pbuf* q, ip_addr_t* ipdaddr) {
	return netif->linkoutput(netif, q);
}

string NpsGateLWIP::stats() {
	char buffer[256];
	string str;

	snprintf(buffer, 256,
			"sockets: %u\n"
			"lsockets: %u\n",
			lwip_listening_socket_count(),
			lwip_active_socket_count());

	str = buffer;

	struct tcp_pcb_listen* lpcb = tcp_listen_pcbs.listen_pcbs;
	while(lpcb) {
		str += lpcb_stats(lpcb);
		lpcb = lpcb->next;
	}

	struct tcp_pcb* apcb = tcp_active_pcbs;
	while(apcb) {
		str += pcb_stats(apcb);
		apcb = apcb->next;
	}

	return str;
}

string NpsGateLWIP::lpcb_stats(tcp_pcb_listen* pcb) {
	char buffer[32];
	in_addr ip_addr;

	ip_addr.s_addr = pcb->local_ip.addr;
	snprintf(buffer, 32, "%s:%u\n", inet_ntoa(ip_addr), pcb->local_port);

	return string(buffer);
}

string NpsGateLWIP::pcb_stats(tcp_pcb* pcb) {
	char buffer[128];
	char saddr[16], daddr[16];
	tcp_seg* seg;
	pbuf* pbuf;
	int unsent, unacked, oos, refused, qpb_size = 0;
	in_addr sip_addr, dip_addr;

	for(seg = pcb->unsent, unsent=0; seg; seg = seg->next, unsent++);
	for(seg = pcb->unacked, unacked=0; seg; seg = seg->next, unacked++);
	for(seg = pcb->ooseq, oos=0; seg; seg = seg->next, oos++);
	for(pbuf = pcb->refused_data, refused=0; pbuf && pbuf->len != pbuf->tot_len; pbuf=pbuf->next, refused+=pbuf->len);

	map<tcp_pcb*, QueuedPbuf>::iterator iter = queued_pbufs.find(pcb);
	if(iter != queued_pbufs.end()) {
//		qpb_size = iter->second.p->tot_len;
	}

	sip_addr.s_addr = pcb->local_ip.addr;
	dip_addr.s_addr = pcb->remote_ip.addr;
	strncpy(saddr, inet_ntoa(sip_addr), 16);
	strncpy(daddr, inet_ntoa(dip_addr), 16);
	snprintf(buffer, 128, "%s:%u => %s:%u\t%u\t%u\t%u\t%u\t%u\t%u\t%u",
			saddr, pcb->local_port,
			daddr, pcb->remote_port, 
			pcb->rcv_wnd, pcb->rcv_ann_wnd, unsent, unacked, oos, refused, qpb_size);
	return string(buffer);
}

unsigned int NpsGateLWIP::lwip_listening_socket_count() {
	struct tcp_pcb_listen* listen = tcp_listen_pcbs.listen_pcbs;
	int i;

	for(i=0; listen != NULL; i++) {
		listen = listen->next;
	}

	return i;
}

unsigned int NpsGateLWIP::lwip_active_socket_count() {
	struct tcp_pcb* active = tcp_active_pcbs;
	int i;

	for(i=0; active != NULL; i++) {
		active = active->next;
	}

	return i;
}


bool NpsGateLWIP::timeout() {
//	list<tcp_pcb*>::iterator iter = close_list.begin();
//	while(iter != close_list.end()) {
//		tcp_close(*iter);
//		iter = close_list.erase(iter);
//	}

	tcp_tmr();
	return true;
}


map<tcp_pcb*, QueuedPbuf> NpsGateLWIP::queued_pbufs;
list<tcp_pcb*> NpsGateLWIP::close_list;
