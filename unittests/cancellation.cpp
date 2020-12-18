#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <taskflow/taskflow.hpp>

// EmptyFuture
TEST_CASE("EmptyFuture" * doctest::timeout(300)) {
  tf::Future<void> fu;
  REQUIRE(fu.cancel() == false);
}

// Future
TEST_CASE("Future" * doctest::timeout(300)) {

  tf::Taskflow taskflow;
  tf::Executor executor;

  std::atomic<int> counter{0};
  
  for(int i=0; i<100; i++) {
    taskflow.emplace([&](){
      counter.fetch_add(1, std::memory_order_relaxed);
    });
  }

  auto fu = executor.run(taskflow);

  fu.get();

  REQUIRE(counter == 100);
}

// Cancel
TEST_CASE("Cancel" * doctest::timeout(300)) {
  
  tf::Taskflow taskflow;
  tf::Executor executor;

  std::atomic<int> counter{0};
  
  // artificially long (possible larger than 300 seconds)
  for(int i=0; i<10000; i++) {
    taskflow.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
  }
  
  // a new round
  counter = 0;
  auto fu = executor.run(taskflow);
  REQUIRE(fu.cancel() == true);
  fu.get();
  REQUIRE(counter < 10000);
  
  // a new round
  counter = 0;
  fu = executor.run_n(taskflow, 100);
  REQUIRE(fu.cancel() == true);
  fu.get();
  REQUIRE(counter < 10000);
}

// multiple cnacels
TEST_CASE("MultipleCancels" * doctest::timeout(300)) {

  tf::Taskflow taskflow1, taskflow2, taskflow3, taskflow4;
  tf::Executor executor;
  
  std::atomic<int> counter{0};
  
  // artificially long (possible larger than 300 seconds)
  for(int i=0; i<10000; i++) {
    taskflow1.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
    taskflow2.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
    taskflow3.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
    taskflow4.emplace([&](){
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      counter.fetch_add(1, std::memory_order_relaxed);
    });
  }
  
  // a new round
  counter = 0;
  auto fu1 = executor.run(taskflow1);
  auto fu2 = executor.run(taskflow2);
  auto fu3 = executor.run(taskflow3);
  auto fu4 = executor.run(taskflow4);
  REQUIRE(fu1.cancel() == true);
  REQUIRE(fu2.cancel() == true);
  REQUIRE(fu3.cancel() == true);
  REQUIRE(fu4.cancel() == true);
  executor.wait_for_all();
  REQUIRE(counter < 10000);
  REQUIRE(fu1.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
  REQUIRE(fu2.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
  REQUIRE(fu3.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
  REQUIRE(fu4.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready);
}

// cancel subflow
TEST_CASE("CancelSubflow" * doctest::timeout(300)) {

  tf::Taskflow taskflow;
  tf::Executor executor;

  std::atomic<int> counter{0};
  
  // artificially long (possible larger than 300 seconds)
  for(int i=0; i<100; i++) {
    taskflow.emplace([&, i](tf::Subflow& sf){
      for(int j=0; j<100; j++) {
        sf.emplace([&](){
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          counter.fetch_add(1, std::memory_order_relaxed);
        });
      }
      if(i % 2) {
        sf.join();
      }
      else {
        sf.detach();
      }
    });
  }
  
  // a new round
  counter = 0;
  auto fu = executor.run(taskflow);
  REQUIRE(fu.cancel() == true);
  fu.get();
  REQUIRE(counter < 10000);
  
  // a new round
  counter = 0;
  auto fu1 = executor.run(taskflow);
  auto fu2 = executor.run(taskflow);
  auto fu3 = executor.run(taskflow);
  REQUIRE(fu1.cancel() == true);
  REQUIRE(fu2.cancel() == true);
  REQUIRE(fu3.cancel() == true);
  fu1.get();
  fu2.get();
  fu3.get();
  REQUIRE(counter < 10000);
}



