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
#include <boost/foreach.hpp>
#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "dtn_bridge.h"
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

NpsGateLWIP::NpsGateLWIP(DTNBridge* s, uint16_t m) {
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
	netif_remove(&if_in);
	netif_remove(&if_out);
}

LWIPSocket* NpsGateLWIP::find_socket(uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport) {
	BOOST_FOREACH(LWIPSocket* s, sockets) {
		if(saddr == s->get_saddr() && daddr == s->get_daddr() && sport == s->get_sport() && dport == s->get_dport()) {
			return s;
		}
	}
	return NULL;
}

LWIPSocket* NpsGateLWIP::connect(uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport) {
	tcp_pcb* pcb;
	ip_addr sipaddr, dipaddr;

	pcb = tcp_new();
	IP4_ADDR(&sipaddr, (saddr>>24), (saddr>>16)&0xff, (saddr>>8)&0xff, saddr&0xff);
	IP4_ADDR(&dipaddr, (daddr>>24), (daddr>>16)&0xff, (daddr>>8)&0xff, daddr&0xff);

	if(ERR_OK != tcp_bind(pcb, &sipaddr, sport)) {
		LOG_CRITICAL("tcp_bind() did not return ERR_OK\n");
		return NULL;
	}

	if(ERR_OK != tcp_connect(pcb, &dipaddr, dport, connected)) {
		LOG_CRITICAL("tcp_connect() did not return ERR_OK\n");
		return NULL;
	}

	LWIPSocket* sock = new LWIPSocket(pcb, this);
	sockets.push_back(sock);
	tcp_arg(pcb, sock);

	return sock;
}

LWIPSocket* NpsGateLWIP::listen(uint32_t addr, uint16_t port) {
	tcp_pcb* lpcb;
	ip_addr ipaddr;

	lpcb = tcp_new();
	IP4_ADDR(&ipaddr, (addr>>24), (addr>>16)&0xff, (addr>>8)&0xff, addr&0xff);

	ip_set_option(lpcb, SOF_REUSEADDR);
	if(ERR_OK != tcp_bind(lpcb, &ipaddr, port)) {
		LOG_CRITICAL("tcp_bind() did not return ERR_OK\n");
		return NULL;
	}

	lpcb = tcp_listen(lpcb);
	if(!lpcb) {
		LOG_WARNING("tcp_listen(): failed to create new listen socket.\n");
		return NULL;
	}

	
	LWIPSocket* sock = new LWIPSocket(lpcb, this);
	sockets.push_back(sock);

	tcp_accept(lpcb, accept_connection);
	tcp_arg(lpcb, sock);

	return sock;
}

bool NpsGateLWIP::inject_packet(Packet* p) {
	IP* ip = p->GetLayer<IP>();
	TCP* tcp = p->GetLayer<TCP>();

	if(!ip || !tcp) {
		LOG_WARNING("Packet does not have a valid TCP/IP header.\n");
		return false;
	}

	lwip_driver_input(&if_in, p);

	return true;
}


err_t NpsGateLWIP::accept_connection(void* arg, tcp_pcb* spcb,  err_t err) {
	LWIPSocket* lsock = (LWIPSocket*)arg;
	NpsGateLWIP* lwip = lsock->lwip;

	LWIPSocket* sock = new LWIPSocket(spcb, lwip);
	lwip->sockets.push_back(sock);

	tcp_arg(spcb, sock);
	tcp_recv(spcb, recv_data);
	tcp_sent(spcb, sent_data);
	tcp_err(spcb, error);

	lwip->sockets.remove(lsock);
	tcp_close(lsock->pcb);
	delete lsock;

	return ERR_OK;
}

err_t NpsGateLWIP::connected(void* arg, tcp_pcb* pcb, err_t err) {
	LWIPSocket* sock = (LWIPSocket*) arg;

	tcp_recv(pcb, recv_data);
	tcp_sent(pcb, sent_data);
	tcp_err(pcb, error);

	sock->send(NULL, 0);

	return ERR_OK;
}

err_t NpsGateLWIP::recv_data(void* arg, tcp_pcb* pcb, pbuf* p, err_t err) {
	/* NULL pbuf means the remote side has closed connection */
	if(!p) {
		return ERR_OK;
	}

	/* Acknowledge to LWIP that we have received this data so the receive window
	   can be opened back up. Then queue the pbuf for transmission. */
	tcp_recved(pcb, p->tot_len);

	return ERR_OK;
}

err_t NpsGateLWIP::sent_data(void* arg, tcp_pcb* pcb, uint16_t len) {
	LWIPSocket* sock = (LWIPSocket*) arg;
	if(arg) {
		sock->send(NULL, 0);
	}
	return ERR_OK;
}

void NpsGateLWIP::error(void* arg, err_t err) {
	LOG_DEBUG("***** TCP Error has occurred.\n");
}

bool NpsGateLWIP::send_data(uint8_t* data, int len) {
	Packet* p = new Packet();

	p->PacketFromIP(data, len);
	stcp->forward_packet(stcp->get_default_output(), p);
	return true;
}

err_t NpsGateLWIP::lwip_driver_init(struct netif* netif) {
	if(!netif) {
		LOG_CRITICAL("netif == NULL\n");
		return ERR_MEM;
	}

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

bool NpsGateLWIP::timeout() {
	tcp_tmr();
	return true;
}

