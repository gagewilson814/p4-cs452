#include "lab.h"
#include <pthread.h>

struct queue {
  int capacity; // max number of items
  int size;     // current number of items
  int front;    // index of next dequeue
  int rear;     // index of next enqueue

  bool shutdown; // set by queue_shutdown()

  pthread_mutex_t lock;     // protects all fields
  pthread_cond_t not_full;  // signaled when size < capacity
  pthread_cond_t not_empty; // signaled when size > 0
  void *items[];
};

queue_t queue_init(int capacity) {
  // one block for struct + capacity pointers
  queue_t q = malloc(sizeof(*q) + capacity * sizeof(void *));

  if (!q) {
    return NULL;
  }

  q->capacity = capacity;
  q->size = 0;
  q->front = 0;
  q->rear = 0;
  q->shutdown = false;

  // synchronization primitives initialization
  if (pthread_mutex_init(&q->lock, NULL) != 0) {
    free(q);
    return NULL;
  }
  if (pthread_cond_init(&q->not_full, NULL) != 0) {
    pthread_mutex_destroy(&q->lock);
    free(q);
    return NULL;
  }
  if (pthread_cond_init(&q->not_empty, NULL) != 0) {
    pthread_cond_destroy(&q->not_full);
    pthread_mutex_destroy(&q->lock);
    free(q);
    return NULL;
  }

  return q;
}

void queue_destroy(queue_t q) {
  if (!q)
    return;

  pthread_mutex_lock(&q->lock);
  // Optionally ensure no threads remain blocked
  pthread_mutex_unlock(&q->lock);

  pthread_cond_destroy(&q->not_empty);
  pthread_cond_destroy(&q->not_full);
  pthread_mutex_destroy(&q->lock);

  free(q);
}

void enqueue(queue_t q, void *data) {
  pthread_mutex_lock(&q->lock);

  // wait for space or shutdown
  while (q->size == q->capacity && !q->shutdown) {
    pthread_cond_wait(&q->not_full, &q->lock);
  }

  if (q->shutdown) {
    // no further enqueue allowed
    pthread_mutex_unlock(&q->lock);
    return;
  }

  // insert item
  q->items[q->rear] = data;
  q->rear++;

  // wrap around if necessary
  if (q->rear == q->capacity) {
    q->rear = 0;
  }

  q->size++;

  // wake up one dequeuer
  pthread_cond_signal(&q->not_empty);
  pthread_mutex_unlock(&q->lock);
}

void *dequeue(queue_t q) {
  pthread_mutex_lock(&q->lock);

  // wait for item or shutdown
  while (q->size == 0 && !q->shutdown) {
    pthread_cond_wait(&q->not_empty, &q->lock);
  }

  // if empty and shutting down, return NULL
  if (q->size == 0 && q->shutdown) {
    pthread_mutex_unlock(&q->lock);
    return NULL;
  }

  // remove item
  void *item = q->items[q->front];
  q->front++;

  // wrap around if necessary
  if (q->front == q->capacity) {
    q->front = 0;
  }

  q->size--;

  // wake up one enqueuer
  pthread_cond_signal(&q->not_full);
  pthread_mutex_unlock(&q->lock);

  return item;
}

void queue_shutdown(queue_t q) {
  pthread_mutex_lock(&q->lock);
  q->shutdown = true;
  // wake all waiting threads
  pthread_cond_broadcast(&q->not_empty);
  pthread_cond_broadcast(&q->not_full);
  pthread_mutex_unlock(&q->lock);
}

bool is_empty(queue_t q) {
  pthread_mutex_lock(&q->lock);
  bool empty = (q->size == 0);
  pthread_mutex_unlock(&q->lock);
  return empty;
}

bool is_shutdown(queue_t q) {
  pthread_mutex_lock(&q->lock);
  bool sd = q->shutdown;
  pthread_mutex_unlock(&q->lock);
  return sd;
}
