#pragma once

typedef std::function<void()> Lambda;
typedef std::list<Lambda> LambdaList;

class Finally {
private:
  Lambda function;

public:
  Finally(Lambda function)
    : function(function) {
  }

  virtual ~Finally() {
    function();
  }
};

class TaskQueueWrapper {
  typedef std::mutex Mutex;
  typedef std::unique_lock<Mutex> Locker;
  typedef std::queue<Lambda> TaskQueue;

  TaskQueue queue;
  Mutex mutex;

public:
  void drainTaskQueue() {
    TaskQueue copy;
    {
      Locker lock(mutex);
      std::swap(copy, queue);
    }
    while (!copy.empty()) {
      copy.front()();
      copy.pop();
    }
  }

  void queueTask(Lambda task) {
    Locker locker(mutex);
    queue.push(task);
  }

};
