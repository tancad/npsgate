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
**  @file jobqueue.h
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

// Queue class that has thread synchronisation

#ifndef JOBQUEUE_H_INCLUDED
#define JOBQUEUE_H_INCLUDED

#include <pthread.h>
#include <queue>
#include <crafter.h>

#include "message.hpp"
#include "queue.hpp"

using namespace std;
using namespace Crafter;

namespace NpsGate {

enum JobQueueType { PACKET, MESSAGE };


class JobQueueItem {
	public:
		JobQueueType type;
		union {
			Packet* packet;
			Message* message;
		};

		bool operator<(const JobQueueItem& b) {
			if(type == PACKET && b.type == MESSAGE) {
				return true;
			}
			return false;
		}
};

class JobQueue2
{
	private:
		priority_queue<JobQueueItem*> m_queue;	

		pthread_cond_t	cond;
		pthread_mutex_t lock;
 
	public:
		JobQueue2() {
			pthread_cond_init(&cond, NULL);
			pthread_mutex_init(&lock, NULL);
		};
		void Enqueue(JobQueueItem* data);
		JobQueueItem* Dequeue(uint32_t timeout);
		int Length();
};

typedef Queue<JobQueueItem*> JobQueue;
//typedef JobQueue2 JobQueue;

}	// namespace NpsGate

#endif /* JOBQUEUE_H_INCLUDED */
