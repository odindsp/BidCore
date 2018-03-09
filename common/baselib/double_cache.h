/* 作者：陈晓明
* 时间：2017-7-28
* 双缓冲封装
* 采用模板方式，使用时模板参数类为缓冲数据对象，需要实现 bool Update();方法
*/
#ifndef DOUBLECACH_H_
#define DOUBLECACH_H_
#include <semaphore.h>
#include <iostream>
#include <unistd.h>
#include "thread.h"
using namespace std;


template < class T >
class DoubleCache : public Thread
{
public:
	T* GetData();
	void ReleaseData(const T*);

	bool Init(int interval_, bool *run_flag, void *par);
	void Uninit();

protected:
	virtual void run();
	void WaitSwap(sem_t &sem_);

private:
	sem_t _semidarr[2];
	T _arr[2];

    sem_t *_extsemid;
	int _cur;

	bool *_run_flag;
	int _interval;
	pthread_mutex_t _mutex;
};


//////////////////////实现////////////////////////
template < class T >
T* DoubleCache<T>::GetData()
{
	T* ret_ = NULL;
	pthread_mutex_lock(&_mutex);
    sem_post(_extsemid);
	ret_ = &_arr[_cur % 2];
	pthread_mutex_unlock(&_mutex);
	return ret_;
}

template < class T >
void DoubleCache<T>::ReleaseData(const T* val)
{
	int sel = 0;
	pthread_mutex_lock(&_mutex);
	if (&_arr[1] == val){
		sel = 1;
	}
	sem_wait(&_semidarr[sel]);
	pthread_mutex_unlock(&_mutex);
}

template < class T >
bool DoubleCache<T>::Init(int interval_, bool *run_flag, void *par)
{
	if (run_flag == NULL){
		return false;
	}

	if (!_arr[0].Init(par) || !_arr[1].Init(par)){
		return false;
	}

	if (interval_ <= 0) interval_ = 1;
	_interval = interval_;
	_run_flag = run_flag;
	pthread_mutex_init(&_mutex, 0);
	sem_init(&_semidarr[0], 0, 0);
	sem_init(&_semidarr[1], 0, 0);

	if (!_arr[0].Update())
	{
		//sem_destroy(&_semidarr[0]);
		//sem_destroy(&_semidarr[1]);
		//return false;
	}

    _extsemid = &_semidarr[0];
	_cur = 0;

	start();

	return true;
}

template < class T >
void DoubleCache<T>::Uninit()
{
	join();
	sem_destroy(&_semidarr[0]);
	sem_destroy(&_semidarr[1]);
	pthread_mutex_destroy(&_mutex);
	_arr[0].Uninit();
	_arr[1].Uninit();
}

template < class T >
void DoubleCache<T>::run()
{
	int next = 0;
	while (_run_flag && *_run_flag)
	{
		SleepInFlag(_interval, _run_flag);
		if (!*_run_flag)
		{
			cout << "DoubleCache find exit flag!\n";
			break;
		}

		next = _cur + 1;
		WaitSwap(_semidarr[next % 2]); // 等待上次被标记为闲置的缓冲区

		if (!_arr[next % 2].Update()){
			continue;
		}

		pthread_mutex_lock(&_mutex);
        _extsemid = &_semidarr[next % 2]; // 指向新更新的
		_cur++;
		pthread_mutex_unlock(&_mutex);
	}
}

template < class T >
void DoubleCache<T>::WaitSwap(sem_t &sem_)
{
	int val = -1;
	while (1)
	{
		if (sem_getvalue(&sem_, &val) == 0)
		{
			if (val == 0){
				return;
			}
		}
		else{
			return;
		}
        usleep(10000);
	}
}


#endif
