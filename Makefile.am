###
### The Plugins.am file is created by the autogen.sh script
###
ACLOCAL_AMFLAGS=-I m4
include Plugins.am

install-data-local:
	cp -rn $(CONFIGURATION_DIR)/$(DEFAULT_CONFIGURATION)/* $(sysconfdir)/
	mv -n $(sysconfdir)/$(DEFAULT_CONFIGURATION).config $(sysconfdir)/npsgate.config
	rm $(sysconfdir)/$(DEFAULT_CONFIGURATION).config
