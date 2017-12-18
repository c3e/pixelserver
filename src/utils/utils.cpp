#include <sys/time.h>
#include <mutex>
#include <iostream>


time_t tstart;
std::mutex logm;

void init_log_(){
	tstart = time(0);
}

template<typename U>
void log__ (int a, U u){
	std::cout << u << std::endl;
	logm.lock();

}

template<typename S, typename... Args>
void log__(int a, S s, Args... args){
	if ( a == 0){
		logm.lock();
		std::cout << "[" << difftime(time(0), tstart) << "]: ";
	}
	std::cout << s;
	log__(a+1,args...);
}

template<typename S,typename... Args>
void log_( S s, Args... args){
	log__(0,s,args...);
}

template <class T>
void log___(std::initializer_list<T> list){
	logm.lock();
	std::cout << "[" << difftime(time(0), tstart) << "]: ";
	for ( auto elem : list)
		std::cout << elem;
	std::cout << std::endl;
	logm.unlock();
}
