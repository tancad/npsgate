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
**  @file npsgatevar.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef NPSGATEVAR_HPP_INCLUDED
#define NPSGATEVAR_HPP_INCLUDED

#include <string>
#include <vector>
#include <map>

#include "logger.h"
#include "boost/any.hpp"

using namespace std;
using boost::any_cast;

namespace NpsGate {

class NpsGateVar {
	public:
		NpsGateVar() {
			ref_count = 1;
			alloc_count++;
		};

		virtual ~NpsGateVar() {
			alloc_count--;
		}

		inline void ref() { ref_count++; }
		inline void unref() {
			if(0 == --ref_count) {
				delete this;
			}
		}

		template <typename T>
		T get() { return any_cast<T>(value); }
		template <typename T>
		void set(T v) { value = v; }
		const type_info& type() { return value.type(); }

		static unsigned int alloc_count;
	private:
		unsigned int ref_count;
		boost::any value;
};

}

#endif /* NPSGATEVAR_HPP_INCLUDED */
