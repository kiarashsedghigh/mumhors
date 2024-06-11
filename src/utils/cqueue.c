#include <muhors/cqueue.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

void cq_init(cqueue_t * cq, int size){
    cq->size=size;
    cq->front=-1;
    cq->rear=-1;
    cq->list = malloc(sizeof (void *) * size);
    assert(cq->list!=NULL);
}

void cq_delete(cqueue_t * cq){
    //TODO delete content
    free(cq->list);
}

void cq_enqueue(cqueue_t * cq, void * e){
    if ((cq->front == 0 && cq->rear == cq->size-1) || ((cq->rear+1) % cq->size == cq->front)){
        printf("CQueue is full \n");
        exit(-1);
    }else if (cq->front == -1){
        cq->front = cq->rear = 0;
        cq->list[cq->rear] = e;
    }else if (cq->rear == cq->size-1 && cq->front != 0){
        cq->rear = 0;
        cq->list[cq->rear] = e;
    }else{
        cq->rear++;
        cq->list[cq->rear] = e;
    }
}

void * cq_dequeue(cqueue_t * cq){
    if (cq->front == -1){
        printf("Queue is Empty\n");
        exit(-1);
    }
    void * data = cq->list[cq->front];
    cq->list[cq->front] = NULL;
    if (cq->front == cq->rear){
        cq->front = -1;
        cq->rear = -1;
    }
    else if (cq->front == cq->size-1)
        cq->front = 0;
    else
        cq->front++;

    return data;
}

void * cq_iter_next(cqueue_t * cq){
    static int next_index;
    if (!cq) {
        next_index = 0;
        return NULL;
    }

    if (next_index > cq->rear){
        printf("CQ ITER: Reached rear end\n");
        return NULL;
    }

    if (cq->front == -1){
        printf("CQ ITER: Queue is empty\n");
        return NULL;
    }
    if (cq->rear >= cq->front || next_index < cq->size - cq->front){
        next_index++;
        return cq->list[cq->front + next_index - 1];
    }
    next_index++;
    return cq->list[next_index-1-(cq->size-cq->front)];
}

/// Assuming int
void * cq_display(cqueue_t * cq) {
    if (cq->front == -1){
        printf("\nQueue is Empty");
        exit(-1);
    }
    if (cq->rear >= cq->front){
        for (int i = cq->front; i <= cq->rear; i++)
            printf("%d ", *(int *)cq->list[i]);
    }
    else{
        for (int i = cq->front; i < cq->size ; i++)
            printf("%d ", *(int *)cq->list[i]);

        for (int i = 0; i <= cq->rear; i++)
            printf("%d ", *(int *)cq->list[i]);
    }
}