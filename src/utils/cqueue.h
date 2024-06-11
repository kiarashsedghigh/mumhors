#ifndef MUHORS_CQUEUE_H
#define MUHORS_CQUEUE_H


typedef struct cqueue{
    int front;
    int rear;
    void ** list;
    int size;
}cqueue_t;


void cq_init(cqueue_t * cq, int size);

void cq_delete(cqueue_t * cq);

void cq_enqueue(cqueue_t * cq, void * e);

void * cq_dequeue(cqueue_t * cq);

void * cq_display(cqueue_t * cq);

void * cq_iter_next(cqueue_t * cq);

#endif
