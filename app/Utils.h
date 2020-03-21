#pragma once

#include <QElapsedTimer>

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


class RateCounter {

  QElapsedTimer timer;
  std::vector<float> times;

public:
  void reset() {
    times.clear();
  }

  unsigned int count() const {
    return times.size() - 1;
  }

  float elapsed() const {
    if (times.size() < 1) {
      return 0.0f;
    }
    float elapsed = *times.rbegin() - *times.begin();
    return elapsed;
  }

  void increment() {
    if (times.empty()) {
      timer.start();
      times.push_back(0);
    } else {
      times.push_back((float)timer.elapsed() / 1000.0f);
    }
  }

  float getRate() const {
    if (elapsed() == 0.0f) {
      return NAN;
    }
    return (float)count() / elapsed();
  }
};
