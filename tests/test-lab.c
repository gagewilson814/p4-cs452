#include "../src/lab.h"
#include "harness/unity.h"
#include <stdbool.h>

void setUp(void) {
  // nothing to do
}

void tearDown(void) {
  // nothing to do
}

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
  // the final enqueued element should still come out
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
  // Since queue is empty + shutdown, dequeue must return NULL
  TEST_ASSERT_NULL(dequeue(q));
  TEST_ASSERT_TRUE(is_empty(q));

  queue_destroy(q);
}

void test_wraparound_behavior(void) {
  // capacity 2 to force ring-buffer wrap
  queue_t q = queue_init(2);
  TEST_ASSERT_NOT_NULL(q);

  int a = 1, b = 2, c = 3;
  enqueue(q, &a);
  enqueue(q, &b);
  // now full: [a, b]

  // remove one => front moves, size=1
  TEST_ASSERT_EQUAL_PTR(&a, dequeue(q));

  // add another => goes into the slot vacated by 'a'
  enqueue(q, &c);

  // now order should be b, c
  TEST_ASSERT_EQUAL_PTR(&b, dequeue(q));
  TEST_ASSERT_EQUAL_PTR(&c, dequeue(q));

  TEST_ASSERT_TRUE(is_empty(q));
  queue_destroy(q);
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_create_destroy);
  RUN_TEST(test_queue_dequeue);
  RUN_TEST(test_queue_dequeue_multiple);
  RUN_TEST(test_queue_dequeue_shutdown);
  RUN_TEST(test_is_empty_initial);
  RUN_TEST(test_is_empty_after_enqueue_and_dequeue);
  RUN_TEST(test_shutdown_before_use);
  RUN_TEST(test_wraparound_behavior);
  return UNITY_END();
}
