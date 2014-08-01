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
**  @file bpa_interface.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#ifndef BPA_INTERFACE_H_INCLUDED
#define BPA_INTERFACE_H_INCLUDED

#include "applib/dtn_api.h"
#include "../logger.hpp"

using namespace std;
using namespace NpsGate;

class BpaInterface
{
	private:
		dtn_handle_t dtn_handle;
	    dtn_bundle_spec_t bundle_spec;
		dtn_reg_id_t regid;
		dtn_reg_info_t reginfo;

		string src_endpoint;
		string dest_endpoint;

	    dtn_bundle_spec_t m_spec;
	    dtn_bundle_payload_t dtn_payload;

		void register_eid() {
//			int ret = dtn_find_registration(dtn_handle, &reginfo.endpoint, &regid);
//			if(ret == 0) {
//				LOG_DEBUG("Found existing registration, binding.\n");
//				if(DTN_SUCCESS != dtn_bind(dtn_handle, regid)) {
//					LOG_CRITICAL("Failed to bind to registration. Reason: %s\n", dtn_strerror(dtn_errno(dtn_handle)));
//				}
//				return;
//			} else if(dtn_errno(dtn_handle) != DTN_ENOTFOUND) {
//				LOG_CRITICAL("Error in dtn_find_registration(): %s\n", dtn_strerror(dtn_errno(dtn_handle)));
//			}
			int ret;

			if(0 != (ret = dtn_register(dtn_handle, &reginfo, &regid))) {
				LOG_CRITICAL("Error creating registration: %s\n", dtn_strerror(dtn_errno(dtn_handle)));
			}

			LOG_INFO("DTN registration complete.\n");
		}
	
	public:
		BpaInterface(string s, string d) : src_endpoint(s), dest_endpoint(d) {

			int rval = dtn_open(&dtn_handle);
			if(DTN_SUCCESS != rval) {
				LOG_CRITICAL("Failed to create DTN handle: %s\n", dtn_strerror(rval));
			}
			
			memset(&bundle_spec, 0, sizeof(bundle_spec));

			if(!parse_eid(&bundle_spec.source, src_endpoint)) {
				LOG_CRITICAL("Failed to parse source endpoint: %s\n", src_endpoint.c_str());
			}

			if(!parse_eid(&bundle_spec.dest, dest_endpoint)) {
				LOG_CRITICAL("Failed to parse destination endpoint: %s\n", src_endpoint.c_str());
			}

			memset(&reginfo, 0, sizeof(reginfo));
			dtn_copy_eid(&reginfo.endpoint, &bundle_spec.source);
			reginfo.flags = DTN_SESSION_PUBLISH;
			reginfo.regid = DTN_REGID_NONE;
			reginfo.expiration = 60 * 60;

			register_eid();

			LOG_INFO("BPA Initialization Complete.\n");
			LOG_INFO("\tSource Endpoint: %s\n", src_endpoint.c_str());
			LOG_INFO("\tDesitination Endpoint: %s\n", dest_endpoint.c_str());

		}

		~BpaInterface()
		{
			dtn_close(dtn_handle);
		}

		bool parse_eid(dtn_endpoint_id_t* eid, string str)
		{
			if (!dtn_parse_eid_string(eid, str.c_str())) {
				LOG_DEBUG("Found literal EID: %s\n", str.c_str());
				return true;
			} else if (!dtn_build_local_eid(dtn_handle, eid, str.c_str())) {
				LOG_DEBUG("Found local EID: %s\n", str.c_str());
				return true;
			}

			LOG_DEBUG("Invalid EID: %s\n", str.c_str());
			return false;
		}

		bool send(char* payload, uint32_t size) {
			dtn_bundle_id_t bundle_id;
			bundle_spec.expiration = 60 * 60;
			bundle_spec.priority = COS_NORMAL;
			bundle_spec.dopts = DOPTS_SINGLETON_DEST;
			if(DTN_ESIZE == dtn_set_payload(&dtn_payload, DTN_PAYLOAD_MEM, payload, size)) {
				LOG_CRITICAL("Failed to set payload. Size was: %u\n", size);
			}

			memset(&bundle_id, 0, sizeof(bundle_id));

			if(0 != dtn_send(dtn_handle, regid, &bundle_spec, &dtn_payload, &bundle_id)) {
         		LOG_WARNING("Failed to send bundle: %s\n", dtn_strerror(dtn_errno(dtn_handle)));
			}

			return true;
		}

		int recv(char* payload, uint32_t max_size, float timeout)
		{
			dtn_bundle_spec_t bundle_spec;
			dtn_bundle_payload_t dtn_payload;

			LOG_DEBUG("Calling dtn_recv with timeout [%f]\n", timeout);

			if(0 != dtn_recv(dtn_handle, &bundle_spec, DTN_PAYLOAD_MEM, &dtn_payload, timeout * 1000.0)) {
				if(dtn_errno(dtn_handle) == DTN_ETIMEOUT) {
					LOG_DEBUG("DTN Recv timeout.\n");
           			return 0;
				} else {
	        		LOG_CRITICAL("DTN Recv Error: %s\n", dtn_strerror(dtn_errno(dtn_handle)));
				}
				return -1;
    		}

			if(dtn_payload.buf.buf_len > max_size) {
				LOG_WARNING("Received %u DTN bytes, but buffer is only %u bytes. Throwing away data!\n", dtn_payload.buf.buf_len, max_size);
				memcpy(payload, dtn_payload.buf.buf_val, max_size);
				return max_size;
			}

			LOG_INFO("Received %u DTN bytes.\n", dtn_payload.buf.buf_len);
			memcpy(payload, dtn_payload.buf.buf_val, dtn_payload.buf.buf_len);

			dtn_free_payload(&dtn_payload);

			return dtn_payload.buf.buf_len;
		}
};

#endif /* BPA_INTERFACE_H_INCLUDED */
