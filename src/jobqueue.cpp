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
**  @file jobqueue.cpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

#include <pthread.h>
#include <queue>
#include <crafter.h>

#include "jobqueue.h"

using namespace std;
using namespace Crafter;

namespace NpsGate {

void JobQueue2::Enqueue(JobQueueItem* data)
{
	pthread_mutex_lock(&lock);

	m_queue.push(data);
	pthread_cond_signal(&cond);

	pthread_mutex_unlock(&lock);
}
 
JobQueueItem* JobQueue2::Dequeue(uint32_t timeout)
{
	struct timespec ts;

	pthread_mutex_lock(&lock);

	clock_gettime(CLOCK_REALTIME, &ts);
	ts.tv_sec += timeout / 1000000;
	ts.tv_nsec += (timeout % 1000000) * 1000;

	int rc = pthread_cond_timedwait(&cond, &lock, &ts);

	if(rc == ETIMEDOUT) {
		pthread_mutex_unlock(&lock);
		return NULL;
	}

	JobQueueItem* result = m_queue.top();
	m_queue.pop();

	pthread_mutex_unlock(&lock);

	return result;
}
 
int JobQueue2::Length() {
	return m_queue.size();
}

}

