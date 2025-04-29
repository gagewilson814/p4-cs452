#include "../src/lab.h"
#include "harness/unity.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

// –– Globals and helpers for the new multi-thread tests ––
static queue_t q_for_thread;
static void *thread_out;

#define NUM_PRODUCERS 4
#define NUM_CONSUMERS 4
#define ITEMS_PER_PRODUCER 1000

static queue_t q_for_mt;
static int consumed[NUM_PRODUCERS * ITEMS_PER_PRODUCER];
static int consumed_count;

// Single-consumer helper
static void *thread_consumer_single(void *arg) {
  (void)arg;
  thread_out = dequeue(q_for_thread);
  return NULL;
}

// Multi-producer helper
static void *producer_func(void *arg) {
  int id = *(int *)arg;
  for (int i = 0; i < ITEMS_PER_PRODUCER; ++i) {
    int *item = malloc(sizeof(int));
    *item = id * ITEMS_PER_PRODUCER + i;
    enqueue(q_for_mt, item);
  }
  return NULL;
}

// Multi-consumer helper
static void *consumer_func_mt(void *arg) {
  (void)arg;
  for (;;) {
    int *item = dequeue(q_for_mt);
    if (item == NULL)
      break;
    int idx = __sync_fetch_and_add(&consumed_count, 1);
    consumed[idx] = *item;
    free(item);
  }
  return NULL;
}

// –– Existing single-threaded tests ––
void setUp(void) {}
void tearDown(void) {}

void test_create_destroy(void) {
  queue_t q = queue_init(10);
  TEST_ASSERT_NOT_NULL(q);
  queue_destroy(q);
}

void test_queue_dequeue(void) {
  queue_t q = queue_init(10);
  TEST_ASSERT_NOT_NULL(q);
  int data = 1;
  enqueue(q, &data);
  TEST_ASSERT_EQUAL_PTR(&data, dequeue(q));
  queue_destroy(q);
}

void test_queue_dequeue_multiple(void) {
  queue_t q = queue_init(10);
  TEST_ASSERT_NOT_NULL(q);
  int data1 = 1, data2 = 2, data3 = 3;
  enqueue(q, &data1);
  enqueue(q, &data2);
  enqueue(q, &data3);
  TEST_ASSERT_EQUAL_PTR(&data1, dequeue(q));
  TEST_ASSERT_EQUAL_PTR(&data2, dequeue(q));
  TEST_ASSERT_EQUAL_PTR(&data3, dequeue(q));
  queue_destroy(q);
}

void test_queue_dequeue_shutdown(void) {
  queue_t q = queue_init(10);
  TEST_ASSERT_NOT_NULL(q);
  int data1 = 1, data2 = 2, data3 = 3;
  enqueue(q, &data1);
  enqueue(q, &data2);
  enqueue(q, &data3);
  TEST_ASSERT_EQUAL_PTR(&data1, dequeue(q));
  TEST_ASSERT_EQUAL_PTR(&data2, dequeue(q));

  queue_shutdown(q);
  TEST_ASSERT_EQUAL_PTR(&data3, dequeue(q));
  TEST_ASSERT_TRUE(is_shutdown(q));
  TEST_ASSERT_TRUE(is_empty(q));
  queue_destroy(q);
}

void test_is_empty_initial(void) {
  queue_t q = queue_init(5);
  TEST_ASSERT_NOT_NULL(q);
  TEST_ASSERT_TRUE(is_empty(q));
  TEST_ASSERT_FALSE(is_shutdown(q));
  queue_destroy(q);
}

void test_is_empty_after_enqueue_and_dequeue(void) {
  queue_t q = queue_init(5);
  TEST_ASSERT_NOT_NULL(q);

  int val = 42;
  enqueue(q, &val);
  TEST_ASSERT_FALSE(is_empty(q));
  TEST_ASSERT_FALSE(is_shutdown(q));

  int *out = dequeue(q);
  TEST_ASSERT_EQUAL_PTR(&val, out);
  TEST_ASSERT_TRUE(is_empty(q));

  queue_destroy(q);
}

void test_shutdown_before_use(void) {
  queue_t q = queue_init(3);
  TEST_ASSERT_NOT_NULL(q);

  queue_shutdown(q);
  TEST_ASSERT_TRUE(is_shutdown(q));
  TEST_ASSERT_NULL(dequeue(q));
  TEST_ASSERT_TRUE(is_empty(q));

  queue_destroy(q);
}

void test_wraparound_behavior(void) {
  queue_t q = queue_init(2);
  TEST_ASSERT_NOT_NULL(q);

  int a = 1, b = 2, c = 3;
  enqueue(q, &a);
  enqueue(q, &b);
  TEST_ASSERT_EQUAL_PTR(&a, dequeue(q));
  enqueue(q, &c);
  TEST_ASSERT_EQUAL_PTR(&b, dequeue(q));
  TEST_ASSERT_EQUAL_PTR(&c, dequeue(q));

  TEST_ASSERT_TRUE(is_empty(q));
  queue_destroy(q);
}

// –– New multithreaded tests ––

void test_consumer_blocks_then_receives(void) {
  q_for_thread = queue_init(1);
  thread_out = NULL;
  pthread_t tid;

  // consumer will block
  pthread_create(&tid, NULL, thread_consumer_single, NULL);
  usleep(100000); // give it time to block
  int data = 99;
  enqueue(q_for_thread, &data);
  pthread_join(tid, NULL);

  TEST_ASSERT_EQUAL_PTR(&data, thread_out);
  queue_destroy(q_for_thread);
}

void test_shutdown_unblocks_blocked_dequeue(void) {
  q_for_thread = queue_init(1);
  thread_out = (void *)1; // sentinel non-NULL
  pthread_t tid;

  // consumer will block
  pthread_create(&tid, NULL, thread_consumer_single, NULL);
  usleep(100000); // ensure it's blocked
  queue_shutdown(q_for_thread);
  pthread_join(tid, NULL);

  TEST_ASSERT_NULL(thread_out);
  queue_destroy(q_for_thread);
}

void test_multiple_producers_consumers(void) {
  q_for_mt = queue_init(50);
  consumed_count = 0;
  pthread_t producers[NUM_PRODUCERS];
  pthread_t consumers[NUM_CONSUMERS];
  int ids[NUM_PRODUCERS];

  // start consumers
  for (int i = 0; i < NUM_CONSUMERS; ++i) {
    pthread_create(&consumers[i], NULL, consumer_func_mt, NULL);
  }

  // start producers
  for (int i = 0; i < NUM_PRODUCERS; ++i) {
    ids[i] = i;
    pthread_create(&producers[i], NULL, producer_func, &ids[i]);
  }

  // wait for all producers
  for (int i = 0; i < NUM_PRODUCERS; ++i) {
    pthread_join(producers[i], NULL);
  }

  // signal consumers to finish
  queue_shutdown(q_for_mt);

  // wait for all consumers
  for (int i = 0; i < NUM_CONSUMERS; ++i) {
    pthread_join(consumers[i], NULL);
  }

  // verify total items consumed
  TEST_ASSERT_EQUAL_INT(NUM_PRODUCERS * ITEMS_PER_PRODUCER, consumed_count);
  queue_destroy(q_for_mt);
}

int main(void) {
  UNITY_BEGIN();

  // single-threaded
  RUN_TEST(test_create_destroy);
  RUN_TEST(test_queue_dequeue);
  RUN_TEST(test_queue_dequeue_multiple);
  RUN_TEST(test_queue_dequeue_shutdown);
  RUN_TEST(test_is_empty_initial);
  RUN_TEST(test_is_empty_after_enqueue_and_dequeue);
  RUN_TEST(test_shutdown_before_use);
  RUN_TEST(test_wraparound_behavior);

  // multi-threaded
  RUN_TEST(test_consumer_blocks_then_receives);
  RUN_TEST(test_shutdown_unblocks_blocked_dequeue);
  RUN_TEST(test_multiple_producers_consumers);

  return UNITY_END();
}
