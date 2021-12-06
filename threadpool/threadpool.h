#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <queue>
#include <cstdio>
#include <exception>
#include <pthread.h>

#include "../locker/locker.h"


//#include "../CGImysql/sql_connection_pool.h"

#include "../redis/redis.h"



template <typename T>
class threadpool{
private:
	int m_thread_number;    //线程数
	int m_max_requests;     //最大请求数
	pthread_t* m_threads;   //线程池
	std::list<T*> m_workqueue; //请求队列
	locker m_queuelocker;      //锁
	sem m_queuestat;           //任务数
	bool m_stop;               //是否结束线程

	//connection_pool* m_connPool;
	redis_pool* m_connPool;   



	static void* worker(void* arg){
		threadpool *pool = (threadpool*) arg;
		pool->run();
		return pool;
	}

	void run(){
		while(!m_stop){
			m_queuestat.wait();
			m_queuelocker.lock();
			if(m_workqueue.empty()){
				m_queuelocker.unlock();
				continue;
			}
			T* request = m_workqueue.front();
			m_workqueue.pop_front();
			m_queuelocker.unlock();
			if(!request) continue;

			connectionRAII rediscon(&request->r_conn, m_connPool);

			request->process();
		}
	}


public:
	threadpool(redis_pool *connPool, int thread_number = 8, int max_request = 10000){
		m_thread_number = thread_number;
		m_max_requests = max_request;
		m_stop = false;
		m_threads = NULL;
		m_connPool = connPool;
		if(thread_number<=0 || max_request<=0){
			throw std::exception();
		}
		m_threads = new pthread_t[m_thread_number];
		if(!m_threads){
			throw std::exception();
		}
		for(int i=0;i<thread_number;i++){
			if(pthread_create(m_threads+i,NULL,worker,this)!=0){
				delete [] m_threads;
				throw std::exception();
			}
			if(pthread_detach(m_threads[i])){
				delete [] m_threads;
				throw std::exception();
			}
		}
	}

	~threadpool(){
		delete [] m_threads;
		m_stop = true;
	}

	bool append(T* request){
		m_queuelocker.lock();
		if(m_workqueue.size()>m_max_requests){
			m_queuelocker.unlock();
			return false;
		}
		m_workqueue.push_back(request);
		m_queuelocker.unlock();
		m_queuestat.post();
		return true;
	}
};

#endif