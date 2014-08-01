#ifndef DTN_BRIDGE_H
#define DTN_BRIDGE_H
#include <pcap.h>
#include <crafter.h>
#include <unistd.h> 
#include "../npsgate_plugin.hpp"
#include "../logger.hpp"
#include "npsgate_lwip.h"

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


class DTNBridge : public NpsGatePlugin {
public:
	DTNBridge(PluginCore* c);
	~DTNBridge();
	bool init();
	bool send_data(uint8_t* data, int len);
	bool process_packet(Packet* p);
	bool process_dtn_packet(Packet* p);
	bool process_ip_packet(Packet* p);
	bool process_message(Message* m);
	bool message_timeout();
	bool main();
private:
	struct ip_addr ipaddr, netmask, gw;
	struct netif if_in, if_out;
	NpsGateLWIP* lwip;

	string dtn_path;
	string ip_path;
	uint32_t dtn_network;
	uint32_t dtn_netmask;

	uint32_t generate_sequence_num();
};


#endif /* DTN_BRIDGE_H */
