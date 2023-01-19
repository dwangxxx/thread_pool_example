#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class ThreadPool;

/* 
  任务队列元素定义：
    包括一个 callbak 回调函数，该回调函数就是需要执行的任务
    以及一个 user_data 元素，为函数的参数，包含客户端传过来的一些信息，比如 socketfd 等     
*/

class TaskElement {
 public:
  TaskElement() = default;
  ~TaskElement() = default;

  void (*task_callback_)(void* arg);
  std::string user_data_;
  void SetFunc(void (*task_callback)(void* arg)) {
    task_callback_ = task_callback;
  }
};

/*
  执行队列元素：
    tid_: 当前执行队列线程的线程 id
    usable_: 当前线程是否还有效，如果无效，管理器会将其销毁
    Start: 启动入口，不断循环从任务队列中取任务执行
*/

class ExecElement {
 public:
  ExecElement() = default;
  ~ExecElement() = default;

  std::thread thread_;
  bool terminated_ = false;
  ThreadPool* pool_;
  static void* Start(void* arg);
  void Terminate();
};

class ThreadPool {
 public:
  ThreadPool(int thread_count);
  ~ThreadPool();

  // 创建线程池
  void CreateThreadPool();
  // 加入任务
  void PushTask(void(*task_callback)(void* arg), int i);

  // 任务队列和执行队列
  std::deque<TaskElement*> task_queue_;
  std::deque<ExecElement*> exec_queue_;

  // 用于任务队列的多线程访问
  std::condition_variable cv_;
  std::mutex mutex_;

  // 线程池大小
  int thread_count_;
};
