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
**  @file priority_queue.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

// Queue class that has thread synchronisation and maintains queues of multiple priorities.
// Priorities are integer values, serviced in order 0, 1, 2, 3...

#ifndef PRIORITY_QUEUE_H_INCLUDED
#define PRIORITY_QUEUE_H_INCLUDED

#include <list>
#include <map>
//#include <queue>
#include <boost/thread.hpp>
#include "queue.hpp"

using namespace std;
//using namespace boost;
//using namespace boost::this_thread;

namespace NpsGate {

template <typename T>
class PriorityQueue
{
	private:
		map<int, Queue<T> > m_queues;		// Map STL queue to store data
		boost::mutex m_mutex;			// The mutex to synchronise on
		boost::condition_variable m_cond;	// The condition to wait for
		list<int> m_priorities;			// The set of priorities in use
 		int Length();				// Total number of elements in queue(s)

	public:
		void Enqueue(const T& data);	// Add data to the queue and notify others
		void Enqueue(const T& data, int priority);	// Add data to the queue and notify others
		T Dequeue();			// Get data from the queue. Wait for data if not available
		T Dequeue(int& priority);
};

template <typename T> void PriorityQueue<T>::Enqueue(const T& data)
{
	this->Enqueue(data, 10);
}

// Add data to the queue and notify others
template <typename T> void PriorityQueue<T>::Enqueue(const T& data, int priority)
{
	// Test whether priority exists
	if (m_queues.find(priority) == m_queues.end()){	// Create queue for new priority
		// Acquire lock on all queues
		m_mutex.lock();
		m_priorities.push_back(priority);
		m_priorities.sort();
		m_queues.insert(pair<int, Queue<T> >(priority, Queue<T>()));
		//m_cond.insert(pair<int, condition_variable>(priority, new condition_variable()));
		m_mutex.unlock();
	}
 
	// Add the data to the queue
	m_queues[priority].Enqueue(data);
 
	// Notify others that data is ready
	m_cond.notify_one();
 
} // Lock is automatically released here

template <typename T> T PriorityQueue<T>::Dequeue()
{
	int priority;
	return Dequeue(priority);
}

// Get data from the queue. Wait for data if not available
template <typename T> T PriorityQueue<T>::Dequeue(int& priority)
{
 
	// Acquire lock on all queues
	boost::unique_lock<boost::mutex> lock(m_mutex);
 
	// When there is no data, wait till someone fills it.
	// Lock is automatically released in the wait and obtained
	// again after the wait
	while (this->Length()==0) m_cond.wait(lock);
 
	list<int>::iterator it = m_priorities.begin();
	while (m_queues[*it].size() == 0){
		it++;
	}
	priority = *it;

	// Retrieve the data from the queue
	return m_queues[priority].Dequeue();
 
} // Lock is automatically released here

template <typename T> int PriorityQueue<T>::Length()
{
	int len = 0;
	for (typename map<int, queue<T> >::iterator it=m_queues.begin(); it!=m_queues.end(); it++){
		len += it->second.Length();
	}
	return len;
}
/*
#ifndef PACKET_JOB_DEFINED
#define PACKET_JOB_DEFINED
struct PacketJob{
	unsigned char* pkt;
	int len;
};
#endif */
}	// namespace NpsGate
#endif
