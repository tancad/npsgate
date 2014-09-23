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
**  @file npsgate.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <libconfig.h>
#include <boost/filesystem.hpp>

#include "pluginmanager.h"
#include "packet_manager.hpp"
#include "npsgate_core.hpp"
#include "logger.h"
#include "npsgatevar.hpp"
#include "monitor/monitor.hpp"

using namespace std;
using namespace NpsGate;

static struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "config", required_argument, NULL, 'c'},
	{ NULL, 0, NULL, 0 }
};

string _plugin_name = "CORE";

unsigned int NpsGateVar::alloc_count = 0;

static NpsGateContext context;
static const char* config_file = "nps_gate.config";
static bool exit_flag = false;

static void usage(const char* pgm_name) {
	printf("Usage: %s [options]\n", pgm_name);
	printf("Options:\n");
	printf("  -h, --help           Print this message and exit.\n");
	printf("  -c, --config=FILE    Use FILE as the main configuration file.\n");
}

static void parse_args(int argc, char** argv) {
	int opt;

	while(-1 != (opt = getopt_long(argc, argv, "hc:", longopts, NULL))) {
		switch(opt) {
			case 'c':
				config_file = optarg;
				break;
			case 'h':
				usage(argv[0]);
				exit(EXIT_SUCCESS);
				break;
			default:
				usage(argv[0]);
				exit(EXIT_FAILURE);
				break;
		}
	}
}

void termination_handler(int signum) {

	switch(signum) {
		case SIGUSR1:
			exit_flag = true;
			break;
		case SIGINT:
			exit_flag = true;
			break;
	}
	exit_flag = true;
}

void catch_sigpipe(int signum) {
	/* The only time we should receive a SIGPIPE is from the Monitor class
	   when a client disconnects during a write. In this case we handle
	   SIGPIPE here to prevent NpsGate from exiting. Error handling will be
	   done by the Monitor class. */
}

int main(int argc, char** argv) {
	struct sigaction term_action;

	parse_args(argc, argv);

	/* Setup signal handler for SIGINT so we can clean up everything nicely */
	term_action.sa_handler = termination_handler;
	sigemptyset(&term_action.sa_mask);
	term_action.sa_flags = 0;
	sigaction(SIGINT,  &term_action, NULL);

	term_action.sa_handler = termination_handler;
	sigemptyset(&term_action.sa_mask);
	term_action.sa_flags = 0;
	sigaction(SIGUSR1,  &term_action, NULL);

	/* Catch SIGPIPE. See notes in signal handler. */
	term_action.sa_handler = catch_sigpipe;
	sigemptyset(&term_action.sa_mask);
	term_action.sa_flags = 0;
	sigaction(SIGPIPE,  &term_action, NULL);



	/* Initialize the logger. By default send everything to stdout. Later we
	 will load the config file and redirect to a file if needed. */
	Logger::init(stdout);


	/* Load the config file and store in the NpsGateContext */
	try{
		context.config = new Config();
		context.config->readFile(config_file);
		context.config_path = boost::filesystem::path(config_file).parent_path();
	} catch (const FileIOException& fioex) {
		printf("Failed to load configuration file: %s\n", config_file);
		termination_handler(SIGINT);
	} catch (const ParseException& pex) {
		printf("Failed to parse configuration file '%s' at line %d\n",
			pex.getFile(), pex.getLine());
		termination_handler(SIGINT);
	}

	/* Initialize the logger with information from the config file. */
	Logger::config(context.config, context);


	/* Create instances of all our core modules and store in NpsGateContext */
	context.plugin_manager = new PluginManager(context);
	context.publish_subscribe = new PublishSubscribe(context);
	context.packet_manager = new PacketManager(context);
	context.monitor = new Monitor(context, MONITOR_INET, "1234");

	/* Load all the plugins and start their threads. */
	context.plugin_manager->load_plugins();

	context.monitor->main(&exit_flag);

	/* Busy wait until we terminate from some reason. */
	while(!exit_flag) {
		sleep(1);
	}

	delete context.monitor;
	delete context.plugin_manager;
	delete context.publish_subscribe;
	delete context.packet_manager;
	delete context.config;

	return 0;
}

