#ifndef THREAD_H_
#define THREAD_H_ 
#include <pthread.h>  

using namespace std;

class Thread
{
public:
	enum { THREAD_STATUS_NEW, THREAD_STATUS_RUNNING, THREAD_STATUS_EXIT };
	//构造函数  
	Thread();

	//开始执行线程  
	bool start();
	//获取线程ID  
	pthread_t getThreadID();
	//获取线程状态  
	int getState();
	//等待线程退出或者超时  
	void join(unsigned long millisTime = 0);
	virtual void term();

	bool SleepInFlag(int Second, bool *flag);

protected:
	//当前线程的线程ID  
	pthread_t _tid;
	//线程的状态  
	int _threadStatus;
	//获取执行方法的指针  
	static void * thread_proxy_func(void * args);
	//内部执行方法  
	void run1();
	//线程的运行实体  
	virtual void run() = 0;
};

#endif
