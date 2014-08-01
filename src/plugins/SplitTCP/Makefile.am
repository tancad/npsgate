
lib_LTLIBRARIES = split_tcp2.la
split_tcp2_la_CFLAGS = -ggdb -O0 -Ilwip/src/include -Iinclude -Ilwip/src/include/ipv4 -Ilwip/src/include/ipv6
split_tcp2_la_CXXFLAGS = -Ilwip/src/include -Iinclude -Ilwip/src/include/ipv4 -Ilwip/src/include/ipv6
split_tcp2_la_LDFLAGS = -module -avoid-version -shared ${CRAFTER_LIBS}
split_tcp2_la_LIBADD = ${CRAFTER_LIBS}
split_tcp2_la_SOURCES = split_tcp.cpp					\
						sys_arch.c						\
						npsgate_lwip.cpp				\
						lwip/src/core/init.c			\
						lwip/src/core/dhcp.c			\
						lwip/src/core/mem.c				\
						lwip/src/core/memp.c			\
						lwip/src/core/netif.c			\
						lwip/src/core/pbuf.c			\
						lwip/src/core/raw.c				\
						lwip/src/core/stats.c			\
						lwip/src/core/sys.c				\
						lwip/src/core/tcp.c				\
						lwip/src/core/tcp_in.c			\
						lwip/src/core/tcp_out.c			\
						lwip/src/core/udp.c				\
						lwip/src/core/timers.c			\
						lwip/src/core/inet_chksum.c		\
						lwip/src/core/ipv4/autoip.c		\
						lwip/src/core/ipv4/icmp.c		\
						lwip/src/core/ipv4/igmp.c		\
						lwip/src/core/ipv4/ip4.c		\
						lwip/src/core/ipv4/ip4_addr.c	\
						lwip/src/core/ipv4/ip_frag.c	\
						lwip/src/api/tcpip.c			\
						lwip/src/api/err.c				\
						lwip/src/api/netifapi.c			\
						lwip/src/api/api_lib.c			\
						lwip/src/api/api_msg.c			\
						lwip/src/api/sockets.c



