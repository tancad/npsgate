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
**  @file message.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef MESSAGE_HPP_INCLUDED
#define MESSAGE_HPP_INCLUDED

#include "npsgatevar.hpp"

namespace NpsGate {

	enum MessageType { 
		INVALID = 0,

		// Publish/Subscribe messages
		SUBSCRIBE = 100,
		SUBSCRIBE_UPDATE,

		// Request (pull) messages
		REQUEST = 200,
		REQUEST_RESPONSE
	};

	class Message {
		public:
			Message() { };
			Message(MessageType t, string n, boost::any v) { 
				type = t;
				fq_name = n;
				value = new NpsGateVar();
				value->set(v);
			}
			MessageType type;
			string fq_name;
			NpsGateVar* value;
			PluginCore* orig;
	};
}

#endif /* MESSAGE_HPP_INCLUDED */
