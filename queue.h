#include "lruc.h"

#ifndef __queue_header__
#define __queue_header__

QNode *new_qnode();
void referenceQueue(lruc *cache, QNode *node);

void enQueue(lruc *cache, QNode *node);

void deQueue(lruc *cache);

#endif
