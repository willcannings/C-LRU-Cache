#include "queue.h"
#include <stdlib.h>


extern void lruc_delete_internal(lruc *cache, void *key, uint32_t key_length);

// create a new qnode when set not find
QNode *new_qnode() {
    QNode *n = (QNode *) calloc(sizeof(QNode), 1);
    return n;
}

int isQueueEmpty(dqueue *queue) {
    return queue->rear == NULL;
}

void referenceQueue(lruc *cache, QNode *node) {
    dqueue *queue = cache->queue;

    node->prev->next = node->next;
    if (node->next) {
        node->next->prev = node->prev;
    }

    // If the requested page is rear, then change rear
    // as this node will be moved to front
    if (node == queue->rear) {
        queue->rear = node->prev;
        queue->rear->next = NULL;
    }

    // Put the requested page before current front
    node->next = queue->front;
    node->prev = NULL;

    // Change prev of current front
    node->next->prev = node;

    // Change front to the requested page
    queue->front = node;
}

void enQueue(lruc *cache, QNode *node) {
    // Create a new node with given page number,
    // And add the new node to the front of queue

    dqueue *queue = cache->queue;
    node->next = queue->front;

    // If queue is empty, change both front and rear pointers
    if (isQueueEmpty(queue)){
        queue->rear = queue->front = node;
    } else {
        queue->front->prev = node;
        queue->front = node;
    }

    queue->filled_count++;
}

void deQueue(lruc *cache)
{
    dqueue *queue = cache->queue;
    if (isQueueEmpty(queue))
        return;

    // If this is the only node in list, then change front
    if (queue->front == queue->rear) {
        queue->front = NULL;
    }

    // Change rear and remove the previous rear
    QNode* temp = queue->rear;
    queue->rear = queue->rear->prev;

    if (queue->rear) {
        queue->rear->next = NULL;
    }

    lruc_item *item = temp->item;
    if (item) {
        lruc_delete_internal(cache, item->key, item->key_length);
    } else {
        free(temp);
    }

    // temp is free already in the lruc_delete_internal
    // decrement the number of full frames by 1
    queue->filled_count--;
}
