#pragma once

namespace oria {
  std::string readFile(const std::string & filename);
}



class RateCounter {
  std::vector<float> times;

public:
  void reset() {
    times.clear();
  }

  size_t count() const {
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
    times.push_back(Platform::elapsedSeconds());
  }

  float getRate() const {
    if (elapsed() == 0.0f) {
      return NAN;
    }
    return (float)count() / elapsed();
  }
};
