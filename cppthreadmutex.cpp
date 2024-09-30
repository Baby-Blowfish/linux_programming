#include<iostream>
#include<thread>
#include<mutex>

using namespace std;

int value = 0;
mutex value_mutex;

void increase_value()
{
	value_mutex.lock();
	value++;
	cout<<"thread start"<<value<<endl;
	value_mutex.unlock();
}

int main()
{
	thread t[10];

	// 10 thread start
	for(auto i = 0; i < 10; ++i)
	{
		t[i] = thread(increase_value);
	}

	for(int i = 0; i < 10; ++i)
	{
		t[i].join();
	}
	
	return 0;

}
