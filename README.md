# thread_pool_example

简单的线程池代码，具体原理为经典的两队列一中枢结构，包括：

- 一个任务队列，包括所有需要执行的任务
- 一个执行队列，包括所有的线程执行体
- 一个管理中枢，负责创建和管理任务队列和执行队列，将任务放到任务队列中，以及执行队列的销毁。
