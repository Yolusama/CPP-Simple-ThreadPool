#pragma once

#include<thread>
#include<mutex>
#include<functional>
#include<queue>
#include<condition_variable>
#include<Windows.h>
#include "Function.hpp"

namespace std {
    //基于C++11的基本线程操作实现的线程池
    class ThreadPool
    {
    private:
        std::vector<std::thread> threads;
        std::queue<std::function<void()>>tasks;
        std::mutex mutex;
        std::condition_variable condition;
        bool shutdown;
        static const int MaxCount;
        static ThreadPool* pool;
        ThreadPool() {}
    public:
        static void Init() {
            pool = new ThreadPool;
            pool->shutdown = false;
            for (int i = 0; i < MaxCount; i++) {
                pool->threads.emplace_back([] {
                    while (true) {
                        std::unique_lock<std::mutex> lock(pool->mutex);
                        pool->condition.wait(lock, [] { return pool->shutdown || !pool->tasks.empty(); });
                        if (pool->shutdown && pool->tasks.empty()) {
                            return;
                        }
                        std::function<void()> task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        lock.unlock();
                        task();
                    }
                    });
            }
        }
        static void Add(std::function<void()>&& func)
        {
            {
                std::unique_lock<std::mutex> lock(pool->mutex);
                pool->tasks.push(func);
            }
            pool->condition.notify_one();
        }
        static const int MaxThreadCount(){
            return MaxCount;
        }
        static void Destroy() {
            {
                std::unique_lock<std::mutex> lock(pool->mutex);
                pool->shutdown = true;
            }
            pool->condition.notify_all();
            for (int i = 0; i < pool->threads.size(); i++) {
                pool->threads[i].join();
            }
            delete pool;
        }
    };
    ThreadPool* ThreadPool::pool;
    const int ThreadPool::MaxCount = std::thread::hardware_concurrency();
}
namespace MStd {
    //使用Windows.h核心线程功能实现的线程池
    class ThreadPool
    {
    private:
       CRITICAL_SECTION lock;
       std::vector<HANDLE> threads;
       std::queue<Function<void>> tasks;
       bool shutdown;
       HANDLE signal;
       static ThreadPool* pool;
       static const int MaxCount;
    private:
       ThreadPool() {}
       static Function<void> GetTask()
       {
           Function<void> task;
           EnterCriticalSection(&pool->lock);
           if (pool->tasks.size() > 0)
           {
               task = pool->tasks.front();
               pool->tasks.pop();
           }
           LeaveCriticalSection(&pool->lock);
           return task;
       }
       static DWORD WINAPI ThreadPoolThread(LPVOID arg) {
           ThreadPool* pool = (ThreadPool*)arg;
           while (!pool->shutdown) {
               Function<void> task = GetTask();
               if (task.func() != NULL) {
                   task();
               }
           }
           return 0;
       }
    public:
        static void Init()
        {
            pool = new ThreadPool;
            InitializeCriticalSection(&pool->lock);
            pool->signal = CreateEventW(NULL, FALSE, FALSE, NULL);
            pool->shutdown = false;
            for (int i = 0; i < MaxCount; i++) {
                DWORD threadId;
                HANDLE threadHandle = CreateThread(NULL, 0, ThreadPoolThread, pool, 0, &threadId);
                if (threadHandle != NULL) {
                    pool->threads.push_back(threadHandle);
                }
            }
        }
        static void Add(Function<void>&& task)
        {
            EnterCriticalSection(&pool->lock);
            while (pool->tasks.size() == MaxCount) {
                WaitForSingleObject(pool->signal, INFINITE);
            }
            pool->tasks.push(task);
            SetEvent(pool->signal);
            LeaveCriticalSection(&pool->lock);
        }
        static const int MaxThreadCount() {
            return MaxCount;
        }
        static void Destroy()
        {
            pool->shutdown = true;
            SetEvent(pool->signal);
            WaitForMultipleObjects(pool->threads.size(), pool->threads.data(), TRUE, INFINITE);
            for (int i = 0; i < pool->threads.size(); i++) {
                CloseHandle(pool->threads[i]);
            }
            DeleteCriticalSection(&pool->lock);
            CloseHandle(pool->signal);
            delete pool;
        }
    };
    ThreadPool* ThreadPool::pool;
    const int ThreadPool::MaxCount = std::thread::hardware_concurrency();
}