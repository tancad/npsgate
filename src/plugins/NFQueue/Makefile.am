
lib_LTLIBRARIES = nfqueue.la
nfqueue_la_SOURCES = nf_queue.cpp
nfqueue_la_LDFLAGS = -module -avoid-version -shared ${CRAFTER_LIBS}
nfqueue_la_LIBADD = ${CRAFTER_LIBS} libnetfilter_queue.a -lnfnetlink


