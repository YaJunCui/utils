//
// Edit by cyj 2016-12-16
//

#ifndef __CYJ_TIMER_HPP__
#define __CYJ_TIMER_HPP__

#include <chrono>

namespace cyj
{

class timer
{
public:
  timer() : m_begin(std::chrono::high_resolution_clock::now())
  {
  }

  void reset()                                                                  //重置计时器
  {
    m_begin = std::chrono::high_resolution_clock::now();
  }

  template <typename Duration = milliseconds>                                   //默认输出毫秒
  int64_t elapsed() const
  {
    return std::chrono::duration_cast<Duration>(std::chrono::high_resolution_clock::now() - m_begin).count();
  }

  int64_t elapsed_micro() const                                                 //微秒
  {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_begin).count();
  }

  int64_t elapsed_nano() const                                                  //纳秒
  {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_begin).count();
  }

  int64_t elapsed_seconds() const                                               //秒
  {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - m_begin).count();
  }

  int64_t elapsed_minutes() const                                               //分
  {
    return std::chrono::duration_cast<std::chrono::minutes>(std::chrono::high_resolution_clock::now() - m_begin).count();
  }

  int64_t elapsed_hours() const                                                 //时
  {
    return std::chrono::duration_cast<std::chrono::hours>(std::chrono::high_resolution_clock::now() - m_begin).count();
  }

private:
  std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
};

} // namespace cyj

#endif //__CYJ_TIMER_HPP__