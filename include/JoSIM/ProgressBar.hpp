// Copyright (c) 2021 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PROGRESSBAR_HPP
#define JOSIM_PROGRESSBAR_HPP

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

namespace JoSIM {

class ProgressBar {
 private:
  std::atomic<bool> complete_ = false;
  std::atomic<float> c_progress_{0.0f};
  std::atomic<float> progress_{0.0f};
  std::atomic<float> total_{1.0f};
  std::atomic<size_t> bar_width_{60};
  std::string fill_{"#"}, remainder_{" "}, status_text_{""};
  std::vector<std::thread> threads_;

 public:
  void set_progress(float value) { progress_ = value; }

  void set_total(float value) { total_ = value; }

  void set_currentprogress(float value) { c_progress_ = value; }

  void set_bar_width(size_t width) { bar_width_ = width; }

  void fill_bar_progress_with(const std::string& chars) { fill_ = chars; }

  void fill_bar_remainder_with(const std::string& chars) { remainder_ = chars; }

  void set_status_text(const std::string& status) { status_text_ = status; }

  void update(float value, std::ostream& os = std::cout) {
    set_progress(value);
  }

  void write_progress(float perc, std::ostream& os = std::cout) {
    os << "\r" << std::flush;
    os << "[";
    const auto completed =
        static_cast<size_t>(perc) * static_cast<size_t>(bar_width_) / 100.0;
    for (size_t i = 0; i < bar_width_; ++i) {
      if (i <= completed)
        os << fill_;
      else
        os << remainder_;
    }
    os << "]";
    os << " " << std::min(static_cast<size_t>(perc), size_t(100)) << "%";
    os << " " << status_text_;
  }

  void thread_print() {
    while (!complete_) {
      float perc = progress_ / total_ * 100;
      if (perc > c_progress_) {
        write_progress(perc);
        set_currentprogress(perc);
      }
    }
    write_progress(100.0f);
  }

  void create_thread() {
    threads_.push_back(std::thread(&ProgressBar::thread_print, this));
  }

  void complete(std::ostream& os = std::cout) {
    complete_ = true;
    if (!threads_.empty()) threads_.back().join();
  }

  ~ProgressBar() {
    complete_ = true;
    if (!threads_.empty())
      if (threads_.back().joinable()) threads_.back().join();
  }
};

}  // namespace JoSIM

#endif  // JOSIM_PROGRESSBAR_HPP