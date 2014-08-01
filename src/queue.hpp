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
**  @file queue.hpp
**  @author Lance Alt (lancealt@gmail.com)
**  @date 2014/09/01
**
*******************************************************************************/

// Queue class that has thread synchronisation

#ifndef QUEUE_H_INCLUDED
#define QUEUE_H_INCLUDED

#include <queue>
#include <boost/thread.hpp>

using namespace std;
//using namespace boost;
//using namespace boost::this_thread;

namespace NpsGate {

template <typename T>
class Queue
{
	private:
		queue<T> m_queue;			// Use STL queue to store data
		boost::mutex m_mutex;			// The mutex to synchronise on
		boost::condition_variable m_cond;	// The condition to wait for
 
	public:
		Queue(){}
		Queue(Queue const& src) : m_queue(src.m_queue){}
		void Enqueue(const T& data);	// Add data to the queue and notify others
		T Dequeue();			// Get data from the queue. Wait for data if not available
		T Dequeue(uint32_t timeout);
		int Length();
};

// Add data to the queue and notify others
template <typename T> void Queue<T>::Enqueue(const T& data)
{
	// Acquire lock on the queue
	boost::unique_lock<boost::mutex> lock(m_mutex);
 
	// Add the data to the queue
	m_queue.push(data);
 
	// Notify others that data is ready
	m_cond.notify_one();
 
} // Lock is automatically released here
 
// Get data from the queue. Wait for data if not available
template <typename T> T Queue<T>::Dequeue()
{
 
	// Acquire lock on the queue
	boost::unique_lock<boost::mutex> lock(m_mutex);
 
	// When there is no data, wait till someone fills it.
	// Lock is automatically released in the wait and obtained
	// again after the wait
	while (m_queue.size()==0) m_cond.wait(lock);
 
	// Retrieve the data from the queue
	T result=m_queue.front();
	m_queue.pop();
	return result;
 
} // Lock is automatically released here

// TODO: Implement dequeue with timeout.
template <typename T> T Queue<T>::Dequeue(uint32_t timeout)
{
	// Acquire lock on the queue
	boost::unique_lock<boost::mutex> lock(m_mutex);

	boost::system_time t = boost::get_system_time() + boost::posix_time::milliseconds(timeout);
 
	if(m_queue.size() == 0) {
		if(!m_cond.timed_wait(lock,t)) {
			return NULL;
		}
	}
 
	// Retrieve the data from the queue
	T result=m_queue.front();
	m_queue.pop();
	return result;
}

template <typename T> int Queue<T>::Length() {return m_queue.size();}
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
