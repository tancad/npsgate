/******************************************************************************
  **
  **  NpsGate.
  **  Copyright (c) 2014, Lance Alt
  **
  **  This file is part of NpsGate.
  **
  **  This program is free software: you can redistribute it and/or modify
  **  it under the terms of the GNU General Public License as published
  **  by the Free Software Foundation, either version 3 of the License, or
  **  (at your option) any later version.
  ** 
  **  This program is distributed in the hope that it will be useful,
  **  but WITHOUT ANY WARRANTY; without even the implied warranty of
  **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  **  GNU General Public License for more details.
  ** 
  **  You should have received a copy of the GNU Lesser General Public License
  **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  ** 
  **
  **  @file npsgate.cpp
  **  @author Lance Alt (lancealt@gmail.com)
  **  @date 2014/09/23
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
static const char* config_file = "npsgate.config";
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

	/* Determine the directory where the npsgate executable resides */
	context.npsgate_execute_path = boost::filesystem::path(argv[0]).parent_path();


	/* Initialize the logger. By default send everything to stdout. Later we
	 will load the config file and redirect to a file if needed. */
	Logger::init(stdout);


	if(!boost::filesystem::exists(config_file)) {
		if(!strcmp(config_file, "npsgate.config")) {
			fprintf(stderr, "\nDefault configuration file './npsgate.config' not found.\n\n");
			fprintf(stderr, "To load a specific configuration file, use the '-c <path to file>' command line option.\n\n\n");
		} else {
			fprintf(stderr, "\n\nConfiguration file specified by the '-c' option not found. Exiting.\n\n\n");
		}
		return -1;
	}

	/* Load the config file and store in the NpsGateContext */
	try{
		context.config = new Config();
		context.config->readFile(config_file);
		context.main_config_path = boost::filesystem::path(config_file).parent_path();

		context.plugin_dir = (const string&) context.config->lookup("NpsGate.plugindir");
		context.plugin_config_dir = (const string&) context.config->lookup("NpsGate.plugin_conf_dir");
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


	LOG_DEBUG("Main config file loaded\n");
	LOG_DEBUG("  npsgate execute path: %s\n", context.npsgate_execute_path.c_str());
	LOG_DEBUG("  plugin dir: %s\n", context.plugin_dir.c_str());
	LOG_DEBUG("  main config path: %s\n", context.main_config_path.c_str());
	LOG_DEBUG("  plugin config dir: %s\n", context.plugin_config_dir.c_str());

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

