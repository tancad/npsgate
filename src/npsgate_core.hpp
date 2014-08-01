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
**  @file npsgate_core.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef NPSGATECORE_HPP_INCLUDED
#define NPSGATECORE_HPP_INCLUDED

#include <string>
#include <map>
#include <vector>

#include "plugincore.h"
#include "npsgatevar.hpp"
//#include "../queue.hpp"

using namespace std;

namespace NpsGate {

typedef vector<int> NpsGateSink;

class NpsGateCore {
	public:
		NpsGateCore() {
		};
		~NpsGateCore() { };

		void init();
		void main();

		/* Forwards a packet to another plugin. The plugin is
		   defined as the NpsGateSink */
		virtual int forward_packet(NpsGateSink sink, void* p);

		/* This method effectively "update" a variable with the
		   specified fq_name */
		virtual bool publish(const string fq_name, NpsGateVar* v);

		/* This method will subscribe to the specified fq_name and
		   set the callback */
		virtual bool subscribe(const string fq_name, void* cb);

		/* This method asyncroniously gets the the specified fq_name
		   (non-callback method) */
		virtual const NpsGateVar* request(const string fq_name);

		/* Gets a sink by name */
		NpsGateSink* get_sink_by_name(string name);

	private:
		vector<PluginCore*> plugins;
};

}

#endif /* NPSGATECORE_HPP_INCLUDED */
