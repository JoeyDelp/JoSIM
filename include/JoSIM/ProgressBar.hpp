// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PROGRESSBAR_HPP
#define JOSIM_PROGRESSBAR_HPP

#include <atomic>
#include <mutex>
#include <iostream>

namespace JoSIM {

class ProgressBar {
public: 
  void set_progress(float value) {
    std::unique_lock lock{mutex_};  // CTAD (C++17)
    progress_ = value;
  }

  void set_bar_width(size_t width) {
    std::unique_lock lock{mutex_};
    bar_width_ = width;    
  }

  void fill_bar_progress_with(const std::string& chars) {
    std::unique_lock lock{mutex_};
    fill_ = chars;    
  }

  void fill_bar_remainder_with(const std::string& chars) {
    std::unique_lock lock{mutex_};
    remainder_ = chars;    
  }

  void set_status_text(const std::string& status) {
    std::unique_lock lock{mutex_};
    status_text_ = status;    
  }

  void update(float value, std::ostream &os = std::cout) {
    set_progress(value);
    write_progress(os);
  }

  void write_progress(std::ostream &os = std::cout) {
    std::unique_lock lock{mutex_};

    // No need to write once progress is 100%
    if (progress_ > 100.0f) return;

    // Move cursor to the first position on the same line and flush 
    os << "\r" << std::flush;

    // Start bar
    os << "[";

    const auto completed = static_cast<size_t>(
      progress_ * static_cast<float>(bar_width_) / 100.0);
    for (size_t i = 0; i < bar_width_; ++i) {
      if (i <= completed) 
        os << fill_;
      else 
        os << remainder_;
    }

    // End bar
    os << "]";

    // Write progress percentage
    os << " " << std::min(static_cast<size_t>(progress_), size_t(100)) << "%"; 

    // Write status text
    os << " " << status_text_;
  }

private:
  std::mutex mutex_;
  float progress_{0.0f};
  size_t bar_width_{60};
  std::string fill_{"#"}, remainder_{" "}, status_text_{""}; 
};

}  // namespace JoSIM

#endif // JOSIM_PROGRESSBAR_HPP