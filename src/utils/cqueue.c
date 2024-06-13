#include <muhors/cqueue.h>
#include <muhors/debug.h>
#include <stdlib.h>
#include <stdio.h>

void cq_init(cqueue_t *cq, int size) {
    cq->size = size;
    cq->front = -1;
    cq->rear = -1;
    cq->list = malloc(sizeof(void *) * size);
}

void cq_delete(cqueue_t *cq) {
    free(cq->list);
}

void cq_enqueue(cqueue_t *cq, void *e) {
    if ((cq->front == 0 && cq->rear == cq->size - 1) || ((cq->rear + 1) % cq->size == cq->front)) {
        debug("CQueue is full \n", DEBUG_ERR);
        exit(-1);
    } else if (cq->front == -1) {
        cq->front = cq->rear = 0;
        cq->list[cq->rear] = e;
    } else if (cq->rear == cq->size - 1 && cq->front != 0) {
        cq->rear = 0;
        cq->list[cq->rear] = e;
    } else {
        cq->rear++;
        cq->list[cq->rear] = e;
    }
}

void *cq_dequeue(cqueue_t *cq) {
    if (cq->front == -1) {
        debug("Queue is Empty\n", DEBUG_ERR);
        exit(-1);
    }
    void *data = cq->list[cq->front];
    if (cq->front == cq->rear) {
        cq->front = -1;
        cq->rear = -1;
    } else if (cq->front == cq->size - 1)
        cq->front = 0;
    else
        cq->front++;
    return data;
}

void *cq_iter_next(cqueue_t *cq) {
    static int next_index;

    if (!cq) {
        next_index = 0;
        return NULL;
    }

    if (cq->front + next_index > cq->rear) {
        debug("CQ ITER: Reached rear end\n", DEBUG_WARNING);
        return NULL;
    }

    if (cq->front == -1) {
        debug("CQ ITER: Queue is empty\n", DEBUG_WARNING);
        return NULL;
    }
    next_index++;

    if (cq->rear >= cq->front || next_index < cq->size - cq->front)
        return cq->list[cq->front + next_index - 1];

    return cq->list[next_index - 1 - (cq->size - cq->front)];
}


void cq_display(cqueue_t *cq) {
    if (cq->front == -1) {
        debug("\nQueue is Empty", DEBUG_ERR);
        exit(-1);
    }
    if (cq->rear >= cq->front) {
        for (int i = cq->front; i <= cq->rear; i++)
            printf("%d ", *(int *) cq->list[i]);
    } else {
        for (int i = cq->front; i < cq->size; i++)
            printf("%d ", *(int *) cq->list[i]);

        for (int i = 0; i <= cq->rear; i++)
            printf("%d ", *(int *) cq->list[i]);
    }
}

void *cq_get_row_by_index(cqueue_t *cq, int index) {
    if (cq->front == -1) {
        debug("\nQueue is Empty", DEBUG_WARNING);
        exit(-1);
    }
    if (cq->rear >= cq->front) {
        if (cq->front + index > cq->rear) {
            debug("Index out of range \n", DEBUG_ERR);
            exit(-1);
        }
        return cq->list[cq->front + index];
    } else {
        if (cq->size - cq->front + cq->rear < index) {
            debug("Index out of range \n", DEBUG_ERR);
            exit(-1);
        }

        if (index < cq->size - cq->front)
            return cq->list[cq->front + index];

        return cq->list[index - (cq->size - cq->front)];
    }
}

void cq_remove_row_by_index(cqueue_t *cq, int index) {

    if (cq->front == -1) {
        debug("\nQueue is Empty", DEBUG_ERR);
        exit(-1);
    }
    if (cq->front <= cq->rear) {

        if (cq->front + index > cq->rear) {
            printf("Index out of range \n");
            exit(-1);
        }

        /* Check which direction is better for shifting */
        if (index < (cq->rear - cq->front) / 2) {
            // Shift all from front to index
            for (int i = cq->front + index - 1; index!=0 && i >= cq->front; i--)
                cq->list[i + 1] = cq->list[i];

            cq->front = (cq->front + 1) % cq->size;

        } else {
            // Shift all from rear to index
            for (int i = cq->front + index + 1; (index!= cq->rear - cq->front) && i <= cq->rear; i++)
                cq->list[i - 1] = cq->list[i];
            cq->rear = (cq->rear - 1) % cq->size;
        }
    } else {
        if (cq->size - cq->front + cq->rear < index) {
            printf("Index out of range \n");
            exit(-1);
        }
        // TODO need optimization based on the size
        if (cq->front + index < cq->size) {
            // Shift all from front to index
            for (int i = cq->front + index - 1; i >= cq->front; i--)
                cq->list[i + 1] = cq->list[i];
            cq->front = (cq->front + 1) % cq->size;
        } else {
            index -= (cq->size - cq->front);

            // Shift all from front to index
            for (int i = index + 1; i <= cq->rear; i++)
                cq->list[i - 1] = cq->list[i];
            cq->rear = (cq->rear - 1) % cq->size;
        }

    }
    if (cq->front == cq->rear) {
        cq->front = -1;
        cq->rear = -1;
    }
}

