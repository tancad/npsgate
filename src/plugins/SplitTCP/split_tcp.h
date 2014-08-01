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
**  @file split_tcp.h
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef SPLIT_TCP_H
#define SPLIT_TCP_H
#include <pcap.h>
#include <crafter.h>
#include <unistd.h> 
#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
//#include "npsgate_driver.h"
//#include "npsgate_out_driver.h"
#include "npsgate_lwip.h"

//#include "lwip/def.h"
//#include "lwip/sys.h"
//#include "lwip/arch.h"
//#include "lwip/ip_addr.h"
//#include "lwip/netif.h"
//#include "lwip/init.h"
#include "lwip/ip.h"

#define NPSGATE_TIMER(DESC, CODE) {														\
	static uint32_t pkt_count = 0;														\
	static uint32_t lwip_sum = 0;														\
	static time_t last_update = 0;														\
	timeval start, end;																	\
																						\
	gettimeofday(&start, NULL);															\
	CODE;																				\
	gettimeofday(&end, NULL);															\
																						\
	if(end.tv_usec > start.tv_usec) {													\
		lwip_sum += (end.tv_usec - start.tv_usec);										\
		pkt_count++;																	\
	}																					\
																						\
	if(time(NULL) - last_update > 1) {													\
		LOG_DEBUG(DESC ": %u   %u\n", pkt_count, lwip_sum / pkt_count);					\
		pkt_count = lwip_sum = 0;														\
		last_update = time(NULL);														\
	}																					\
}	


using namespace Crafter;
using namespace NpsGate;


class SplitTCP : public NpsGatePlugin {
public:
	SplitTCP(PluginCore* c);
	~SplitTCP();
	bool init();
	bool send_data(uint8_t* data, int len);
	bool process_packet(Packet* p);
	bool process_message(Message* m);
	bool message_timeout();
	bool main();
private:
	struct ip_addr ipaddr, netmask, gw;
	struct netif if_in, if_out;
	NpsGateLWIP* lwip;

	uint32_t generate_sequence_num();
};


#endif /* SPLIT_TCP_H */
