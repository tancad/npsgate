
bin_PROGRAMS = npsgate
npsgate_SOURCES =				\
	npsgate.cpp					\
	logger.cpp					\
	plugincore.cpp				\
	pluginmanager.cpp			\
	jobqueue.cpp				\
	publish_subscribe.cpp		\
	monitor/monitor_main.cpp	\
	monitor/monitor_comms.cpp	\
	monitor/monitor_log.cpp		\
	monitor/monitor_plugins.cpp	\
	monitor/monitor_pubsub.cpp
npsgate_CPPFLAGS = $(BOOST_CPPFLAGS) $(LIBCONFIGXX_CFLAGS) $(CRAFTER_CFLAGS)
npsgate_LDFLAGS = $(BOOST_THREADS_LDFLAGS) $(LIBCONFIGXX_LIBS) $(CRAFTER_LIBS)
npsgate_LDADD = $(BOOST_THREAD_LIBS) $(LIBCONFIGXX_LIBS) $(CRAFTER_LIBS)

