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
#include "dtn_bridge.h"

#include "lwip/tcp.h"

extern "C" {
	void NPSGATE_LOG(const char* format, ...);
};

class DTNBridge;
class NpsGateLWIP;

static uint32_t NETMASKS[] = {
	0b00000000000000000000000000000000,
	0b10000000000000000000000000000000,
	0b11000000000000000000000000000000,
	0b11100000000000000000000000000000,
	0b11110000000000000000000000000000,
	0b11111000000000000000000000000000,
	0b11111100000000000000000000000000,
	0b11111110000000000000000000000000,
	0b11111111000000000000000000000000,
	0b11111111100000000000000000000000,
	0b11111111110000000000000000000000,
	0b11111111111000000000000000000000,
	0b11111111111100000000000000000000,
	0b11111111111110000000000000000000,
	0b11111111111111000000000000000000,
	0b11111111111111100000000000000000,
	0b11111111111111110000000000000000,
	0b11111111111111111000000000000000,
	0b11111111111111111100000000000000,
	0b11111111111111111110000000000000,
	0b11111111111111111111000000000000,
	0b11111111111111111111100000000000,
	0b11111111111111111111110000000000,
	0b11111111111111111111111000000000,
	0b11111111111111111111111100000000,
	0b11111111111111111111111110000000,
	0b11111111111111111111111111000000,
	0b11111111111111111111111111100000,
	0b11111111111111111111111111110000,
	0b11111111111111111111111111111000,
	0b11111111111111111111111111111100,
	0b11111111111111111111111111111110,
	0b11111111111111111111111111111111
};

class LWIPSocket {
	public:
		LWIPSocket() : pcb(NULL), lwip(NULL), queued_data(NULL), pending_close(false) { }
		LWIPSocket(tcp_pcb* p, NpsGateLWIP* l) : pcb(p), lwip(l), queued_data(NULL), pending_close(false) { }

		inline uint32_t get_saddr() const { return ntohl(pcb->local_ip.addr); }
		inline uint32_t get_daddr() const { return ntohl(pcb->remote_ip.addr); }
		inline uint16_t get_sport() const { return pcb->local_port; }
		inline uint16_t get_dport() const { return pcb->remote_port; }

		inline void set_saddr(uint32_t saddr) { pcb->local_ip.addr = htonl(saddr); }
		inline void set_daddr(uint32_t daddr) { pcb->remote_ip.addr = htonl(daddr); }
		inline void set_sport(uint16_t sport) { pcb->local_port = sport; }
		inline void set_dport(uint16_t dport) { pcb->remote_port = dport; }

		inline string get_saddr_str() const { return addr_to_str(pcb->local_ip.addr); }
		inline string get_daddr_str() const { return addr_to_str(pcb->remote_ip.addr); }
		inline void set_saddr(const string& str) { pcb->local_ip.addr = str_to_addr(str); }
		inline void set_daddr(const string& str) { pcb->remote_ip.addr = str_to_addr(str); }

		int send(uint8_t* data, int len) {

			if(data && len > 0) {
				enqueue_data(data, len);
			}

			int sent = send_queued_data();
			//NPSGATE_LOG("Sent bytes: %u\n", sent);

			//if(queued_data) {
			//	NPSGATE_LOG("Queue Length: %u (%u)\n", queued_data->tot_len, pbuf_clen(queued_data));
			//}

			tcp_output(pcb);
			return len;
		}

		void close() {
			if(queued_data) {
				pending_close = true;
			} else {
				tcp_close(pcb);
			}
		}

		bool operator==(const LWIPSocket& s2) {
			return (pcb == s2.pcb) && (lwip == s2.lwip);
		}

		static string addr_to_str(uint32_t addr) { 
			char buffer[16];
			sprintf(buffer, "%hhu.%hhu.%hhu.%hhu", (addr>>24), (addr>>16), (addr>>8), addr);
			return string(buffer);
		}

		static uint32_t str_to_addr(const string& str) {
			uint8_t a, b, c, d;
			sscanf(str.c_str(), "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d);
			return (a<<24) + (b<<16) + (c<<8) + d;
		}

		static uint32_t prefix_to_netmask(uint32_t prefix) {
			return (prefix < 32 ? NETMASKS[prefix] : NETMASKS[31]);
		}

		friend class NpsGateLWIP;
	private:
		void enqueue_data(uint8_t* data, int len) {
			if(data && len > 0) {
				pbuf* new_data = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
				pbuf_take(new_data, data, len);
				if(queued_data) {
					pbuf_cat(queued_data, new_data);
				} else {
					queued_data = new_data;
				}
			}
		}

		int send_queued_data() {
			int sndbuf, byte_count = 0;
			pbuf* old;

			sndbuf = tcp_sndbuf(pcb);
			while(queued_data && sndbuf >= queued_data->len) {
				if(ERR_OK != tcp_write(pcb, queued_data->payload, queued_data->len, TCP_WRITE_FLAG_COPY)) {
					//NPSGATE_LOG("tcp_write() failed for %u bytes. Sndbuf: %u\n", queued_data->len, sndbuf);
					break;
				}
				byte_count = queued_data->len;
				//NPSGATE_LOG("Sent queued data: %u\n", queued_data->len);
				old = queued_data;
				queued_data = pbuf_dechain(queued_data);
				pbuf_free(old);
				sndbuf = tcp_sndbuf(pcb);
			}

			if(pending_close && !queued_data) {
				tcp_close(pcb);
			}
			
			return byte_count;
		}

		tcp_pcb* pcb;
		NpsGateLWIP* lwip;
		pbuf* queued_data;
		bool pending_close;
};


class NpsGateLWIP {
	public:
		NpsGateLWIP(DTNBridge*, uint16_t);
		~NpsGateLWIP();
		bool inject_packet(Packet* p);
		bool send_data(uint8_t* data, int len);
		bool timeout();

		LWIPSocket* find_socket(uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport);	
		LWIPSocket* connect(uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport);
		LWIPSocket* listen(uint32_t addr, uint16_t port);

		string stats();

		uint8_t* xmit_buffer;
	private:
		struct ip_addr ipaddr, netmask, gw;
		struct netif if_in, if_out;
		DTNBridge* stcp;
		list<LWIPSocket*> sockets;

		uint16_t mtu;

		static err_t accept_connection(void* arg, tcp_pcb* newpcb,  err_t err);
		static err_t connected(void* arg, tcp_pcb* pcb, err_t err);
		static err_t recv_data(void* arg, tcp_pcb* pcb, pbuf* p, err_t err);
		static err_t sent_data(void* arg, tcp_pcb* pcb, uint16_t len);
		static void error(void* arg, err_t err);

		static err_t lwip_driver_init(struct netif* netif);
		static void lwip_driver_input(struct netif* netif, Packet* pkt);
		static err_t lwip_driver_output(struct netif* netif, struct pbuf* p);
		static err_t lwip_driver_output2(struct netif* netif, struct pbuf* q, ip_addr_t* ipdaddr);
};

#endif /* NPSGATELWIP_H */
