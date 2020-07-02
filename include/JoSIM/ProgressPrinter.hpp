// Copyright (c) 2020 Johannes Delport
// This code is licensed under MIT license (see LICENSE for details)
#ifndef JOSIM_PROGRESSPRINTER_HPP
#define JOSIM_PROGRESSPRINTER_HPP

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <ratio>
#include <thread>

namespace JoSIM {

class TimeProgressPrinter {
  const float total_time_;

  public:
  using State = float;

  TimeProgressPrinter(State total_time) : total_time_(total_time) {
    // Empty
  }

  void draw(float current_time) const {
    std::cout << "\r" << static_cast<int>(current_time / total_time() * 100) 
              << "%" << std::flush;
  }

  void done() const {
    std::cout << "\r100%" << std::endl;
  }

  float total_time() const { return total_time_; }
}; // class TimeProgressPrinter

template <class T> class ThreadSafeVariable {
  T state_ = -1;
  std::mutex mutex_;

  public:
  ThreadSafeVariable(T initial) : state_(initial) {}

  ThreadSafeVariable(ThreadSafeVariable<T> &&other) { state_ = other.load(); }

  T load() {
    auto lock = std::lock_guard(mutex_);
    T out = state_;
    return out;
  }

  void store(T new_state) {
    auto lock = std::lock_guard(mutex_);
    state_ = new_state;
  }
}; // class ThreadSafeVariable

enum class BufferedWriterStateEnum {
  Starting,
  Waiting,
  Writing,
  Finished,
}; // class BufferedWriterStateEnum

class BufferedWriterState {
  std::atomic<BufferedWriterStateEnum> state_{
      BufferedWriterStateEnum::Starting};
  std::atomic<bool> should_finish_{false};

  public:
  void start_writing() { state_.store(BufferedWriterStateEnum::Writing); }

  void start_waiting() { state_.store(BufferedWriterStateEnum::Waiting); }

  void finish() { state_.store(BufferedWriterStateEnum::Finished); }

  void store(BufferedWriterStateEnum state) { state_.store(state); }

  BufferedWriterStateEnum load() { return state_.load(); }

  void wait_until(BufferedWriterStateEnum state) {
    using namespace std::chrono_literals;

    while (state_.load() != state) {
      std::this_thread::sleep_for(1ms);
    }
  }

  void wait_until_not(BufferedWriterStateEnum state) {
    using namespace std::chrono_literals;

    while (state_.load() == state) {
      std::this_thread::sleep_for(1ms);
    }
  }
}; // class BufferedWriterState

class BufferedWriterSignals {
  std::atomic<bool> next_{false};
  std::atomic<bool> should_exit_{false};

  public:
  bool has_next() { return next_.load(); }

  void process_next() { next_.store(false); }

  void signal_next() { next_.store(true); }

  bool should_exit() { return should_exit_.load(); }

  void signal_exit() { should_exit_.store(true); }
}; // BufferedWriterSignals

template <typename ProgressPrinter> class BufferedProgressPrinter {
  struct InternalMemory {
    // Functional state
    ProgressPrinter printer;
    ThreadSafeVariable<typename ProgressPrinter::State> progress;
    BufferedWriterState state;
    BufferedWriterSignals signal;

    // Threading state
    std::mutex output_mutex;
    std::condition_variable condition_variable;

    InternalMemory(
      ProgressPrinter printer_, typename ProgressPrinter::State initial) :
    printer(std::move(printer_)), progress(initial) {}

  };

  std::unique_ptr<InternalMemory> internal_;
  std::thread thread_;

  static void run(InternalMemory *storage) {
    std::unique_lock<std::mutex> output_lock(storage->output_mutex);
    storage->printer.draw(storage->progress.load());

    while (true) {
      // Wait on notify
      storage->state.start_waiting();
      storage->condition_variable.wait(output_lock);

      storage->state.start_writing();

      if (storage->signal.should_exit())
        break;

      if (storage->signal.has_next()) {
        storage->signal.process_next();
        storage->printer.draw(storage->progress.load());
      }
    }

    storage->printer.done();

    storage->state.finish();
  }

  public:
  using State = typename ProgressPrinter::State;

  BufferedProgressPrinter(ProgressPrinter printer, State progress)
      : internal_(std::make_unique<InternalMemory>(
            std::move(printer), progress)) {

    // Start thread
    thread_ = std::thread(&BufferedProgressPrinter::run, internal_.get());

    // Must compelte initialization
    internal_->state.wait_until_not(BufferedWriterStateEnum::Starting);
  }

  void update(State state) {
    internal_->progress.store(state);
    internal_->signal.signal_next();
    internal_->condition_variable.notify_one();
  }

  void done() {
    using namespace std::chrono_literals;

    internal_->signal.signal_exit();

    while(internal_->state.load() == BufferedWriterStateEnum::Waiting) {
      internal_->condition_variable.notify_one();
      std::this_thread::sleep_for(1ms);
    }

    thread_.join();
  }
}; // class BufferedPorgressPrinter

} // namespace JoSIM

#endif // JOSIM_PROGRESSPRINTER_HPP