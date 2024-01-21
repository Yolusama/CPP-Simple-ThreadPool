#include<iostream>
#include "ThreadPool.hpp"


int main() {
	std::cout << std::ThreadPool::MaxThreadCount() << '\n';
	std::ThreadPool::Init();
	std::ThreadPool::Add([]() {std::cout << "hello,world!\n"; });
	std::ThreadPool::Add([]() {std::cout << "Hello,world!\n"; });
	std::ThreadPool::Add([]() {std::cout << "Hello World!\n"; });
	std::ThreadPool::Destroy();
	MStd::ThreadPool::Init();
	MStd::ThreadPool::Add([]() {std::cout << "hello,world!\n"; });
	MStd::ThreadPool::Add([]() {std::cout << "Hello,world!\n"; });
	MStd::ThreadPool::Add([]() {std::cout << "Hello World!\n"; });
	MStd::ThreadPool::Destroy();
	return 0;
}