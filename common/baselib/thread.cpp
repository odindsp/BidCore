#include <unistd.h>
#include "thread.h"


void Thread::run1()
{
	_tid = pthread_self();
	_threadStatus = THREAD_STATUS_RUNNING;
	run();
	_threadStatus = THREAD_STATUS_EXIT;
	pthread_exit(NULL);
}

Thread::Thread()
{
	_tid = 0;
	_threadStatus = THREAD_STATUS_NEW;
}

bool Thread::start()
{
	return pthread_create(&_tid, NULL, thread_proxy_func, this) == 0;
}

pthread_t Thread::getThreadID()
{
	return _tid;
}

int Thread::getState()
{
	return _threadStatus;
}

void * Thread::thread_proxy_func(void * args)
{
	Thread * pThread = static_cast<Thread *>(args);

	pThread->run1();

	return NULL;
}

void Thread::join(unsigned long millisecond)
{
	if (_tid == 0){
		return;
	}

	if (millisecond == 0)
	{
		pthread_join(_tid, NULL);
		_tid = 0;
	}
	else
	{
		unsigned long k = 0;
		while (_threadStatus != THREAD_STATUS_EXIT && k <= millisecond)
		{
			usleep(1000);
			k++;
		}
	}
}

void Thread::term()
{
	if (_threadStatus == THREAD_STATUS_RUNNING)
	{
		pthread_cancel(_tid);
	}
}

bool Thread::SleepInFlag(int Second, bool *flag)
{
	if (!flag)
	{
		sleep(Second);
		return true;
	}

	int k = 0;
	while (*flag && _threadStatus != THREAD_STATUS_EXIT && k <= Second * 10)
	{
		usleep(100 * 1000);
		k++;
	}

	return *flag;
}
