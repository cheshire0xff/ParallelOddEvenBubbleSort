#ifndef INC_TIMER_H__
#define INC_TIMER_H__

#include <chrono>

class Timer {
   public:
    void start() { startTime = std::chrono::system_clock::now(); }
    template <typename T>
    T elapsedTime() const {
        return std::chrono::duration_cast<T>(std::chrono::system_clock::now() -
                                             startTime);
    }

   private:
    std::chrono::time_point<std::chrono::system_clock> startTime;
};

#endif // INC_TIMER_H__
