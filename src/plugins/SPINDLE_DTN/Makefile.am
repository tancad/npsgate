
lib_LTLIBRARIES = spindle_input.la
spindle_input_la_SOURCES = spindle_input.cpp bpa_interface.hpp
spindle_input_la_LDFLAGS = -module -avoid-version -shared ${CRAFTER_LIBS}
spindle_input_la_LIBADD = ${CRAFTER_LIBS} ${BOOST_THREAD_LIBS} libdtnapi.a

lib_LTLIBRARIES = spindle_output.la
spindle_output_la_SOURCES = spindle_output.cpp bpa_interface.hpp
spindle_output_la_LDFLAGS = -module -avoid-version -shared ${CRAFTER_LIBS}
spindle_output_la_LIBADD = ${CRAFTER_LIBS} ${BOOST_THREAD_LIBS} libdtnapi.a


