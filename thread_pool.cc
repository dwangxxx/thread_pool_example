#include "thread_pool.h"

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

// 此函数运行一个循环，不断从任务队列中取任务出来执行
// 当无任务分配的时候，会阻塞，直到任务到达为止
void* ExecElement::Start(void* arg) {
  // 获取执行对象
  ExecElement* exec_element = (ExecElement*) arg;
  std::hash<std::thread::id> thread_id_hasher;
  
  while (true) {
    TaskElement* task_element = nullptr;
    {
      // 首先加锁，从任务队列中取任务
      std::unique_lock<std::mutex> unique_lock(exec_element->pool_->mutex_);
      // 条件变量使用，需要 while 循环，不能用 if，否则会出现虚假唤醒的情况
      while (exec_element->pool_->task_queue_.empty()) {
        // 如果当前线程不可用，直接退出
        if (exec_element->terminated_) {
          break;
        }
        // 等待任务到达
        exec_element->pool_->cv_.wait(unique_lock);
      }

      if (!exec_element->pool_->task_queue_.empty()) {
        task_element = exec_element->pool_->task_queue_.front();
        exec_element->pool_->task_queue_.pop_front();
      }
    }

    if (exec_element->terminated_) {
      break;
    }

    // 执行任务回调
    if (!task_element) {
      task_element->user_data_ += std::to_string(thread_id_hasher(std::this_thread::get_id()));
    }
  }

  std::cout << "Destroy thread " << thread_id_hasher(std::this_thread::get_id()) << std::endl;

  return nullptr;
}

void ExecElement::Terminate() {
  terminated_ = true;
}

ThreadPool::ThreadPool(int thread_count) : thread_count_(thread_count) {}

// 创建线程池
void ThreadPool::CreateThreadPool() {
  std::cout << "Start creating thread pool" << std::endl;

  for (int i = 0; i < thread_count_; ++i) {
    auto exec_element = new ExecElement;
    exec_element->pool_ = const_cast<ThreadPool*>(this);
    threads_.emplace_back(std::thread(ExecElement::Start, exec_element));
    std::cout << "Create thread " << i << " successfully." << std::endl;
    exec_queue_.push_back(exec_element);
  }

  std::cout << "Create thread pool successfully." << std::endl;
}

// 添加任务
void ThreadPool::PushTask(void(*task_callback)(void* arg), int i) {
  TaskElement* task_element = new TaskElement;
  task_element->SetFunc(task_callback);
  task_element->user_data_ = "Task " + std::to_string(i) + " run in thread";
  {
    std::unique_lock<std::mutex> lock(mutex_);
    task_queue_.push_back(task_element);
  }
  // 先解锁再通知其他等待条件变量的线程
  cv_.notify_one();
}

ThreadPool::~ThreadPool() {
  std::cout << "Stop exec queue" << std::endl;
  for (int i = 0; i < exec_queue_.size(); ++i) {
    exec_queue_[i]->Terminate();
  }

  // 清空任务队列中的元素，并唤醒其他的全部线程
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (int i = 0; i < task_queue_.size(); ++i) {
      delete task_queue_[i];
    }
  }
  cv_.notify_all();

  // 等待所有子线程安全退出
  for (int i = 0; i < exec_queue_.size(); ++i) {
    if (threads_[i].joinable()) {
      threads_[i].join();
    }
  }
  threads_.clear();

  // 所有线程均安全退出，清空执行队列
  for (int i = 0; i < exec_queue_.size(); ++i) {
    delete exec_queue_[i];
  }

  std::cout << "Destroy thread pool successfully!" << std::endl;
}

int main() {
  auto thread_pool = std::make_unique<ThreadPool>(10);
  thread_pool->CreateThreadPool();

  return 0;
}
