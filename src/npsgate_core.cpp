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
**  @file npsgate_core.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <unistd.h>
#include <string>
#include <map>
#include <vector>

#include "logger.hpp"
#include "npsgate_core.hpp"
#include "npsgatevar.hpp"

using namespace std;

namespace NpsGate {

typedef vector<int> NpsGateSink;

void NpsGateCore::init() {

	//p->init(this);
}

void NpsGateCore::main() {
	while(1) {
		LOG_DEBUG("Core waiting forever...\n");
		sleep(5);
	}
}

int NpsGateCore::forward_packet(NpsGateSink sink, void* p) {
	LOG_TODO("I should be forwarding a packet.\n");
	return 0;
}

bool NpsGateCore::publish(const string fq_name, NpsGateVar* v) {
	LOG_TODO("I should be publishing a variable.\n");
	return NULL;
}

bool NpsGateCore::subscribe(const string fq_name, void* cb) {
	LOG_TODO("I should be adding a subscribe.\n");
	return NULL;
}

const NpsGateVar* NpsGateCore::request(const string fq_name) {
	LOG_TODO("I should be requesting a variable.\n");
	return NULL;
}

NpsGateSink* NpsGateCore::get_sink_by_name(string name) {
	LOG_TODO("I should be returning a sink.\n");
	return NULL;
}

}

