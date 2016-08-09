#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<list>
#include<cstdio>
#include<exception>
#include<pthread.h>
#include "locker.h"

template<typename Type>
class threadpool
{
public:
   threadpool(int thread_number = 8, int max_requests = 10000);
   ~threadpool();
   bool append(Type* request);
protected:
  static void * worker(void *arg);
  void run();
private:
  int m_thread_number;
  int m_max_requests;
  pthread_t* m_threads;
  list<Type*> m_workqueue;
  locker m_queuelocker;
  sem m_queuestat;
  bool m_stop;
};

template<typename Type>
threadpool<Type>::threadpool(int thread_number, int max_requests):m_thread_number(thread_number),m_max_requests(max_requests),m_threads(NULL), m_stop(false)
{
   if(thread_number <= 0 || max_requests <= 0)
   {
     throw exception();
   }
   try
   {
     m_threads = new pthread_t[m_thread_number];
   }
   catch(exception &e)
   {
     cout<<"threadpool create fail!"<<endl;
     throw(e);
   }
   
   for(int i=0; i<m_thread_number;++i)
   {
      cout<<"create the "<<i<<"th thread"<<endl;
      if(pthread_create(m_threads+i, NULL, worker, this) != 0) 
      {
         delete [] m_threads;
         throw exception();
      }
      if(pthread_detach(m_threads[i]))
      {
         delete [] m_threads;
         throw exception();
      }
   }
}

template<typename Type>
threadpool<Type>::~threadpool()
{
   delete [] m_threads;
   m_threads  = NULL;
   m_stop = true;
}

template<typename Type>
bool threadpool<Type>::append(Type *request)
{
  m_queuelocker.lock();
 if(m_workqueue.size() >= m_max_requests)
  {
     m_queuelocker.unlock();
     return false;
  }
  m_workqueue.push_back(request);
  m_queuelocker.unlock();
  m_queuestat.post();
  return true;
}

template<typename Type>
void *threadpool<Type>::worker(void *arg)
{
   threadpool * pool = static_cast<threadpool*>(arg);
   pool->run();
   return pool;
}

template<typename Type>
void threadpool<Type>::run()
{
  while(!m_stop)
  {
     m_queuestat.wait();
     m_queuelocker.lock();
     if(m_workqueue.empty())
     {
        m_queuelocker.unlock();
        continue;
     }
     Type *request = m_workqueue.front();  
     m_workqueue.pop_front();
     m_queuelocker.unlock();
     if(!request)
     {
        continue;
     }
     request->process();
  }
}
#endif
