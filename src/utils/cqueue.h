#ifndef MUMHORS_CQUEUE_H
#define MUMHORS_CQUEUE_H

/// Circular queue structure
typedef struct cqueue {
    int size;       /* Total capacity of the circular queue */
    int front;      /* Front pointer */
    int rear;       /* Rear pointer */
    void **list;   /* Pointer to the list of elements in the queue */
} cqueue_t;

/// Initializing the circular queue
/// \param cq Pointer to the circular queue struct
/// \param size Size of the circular queue
void cq_init(cqueue_t *cq, int size);

/// Deleting the circular queue
/// \param cq Pointer to the circular queue struct
void cq_delete(cqueue_t *cq);

/// Enqueueing a new element to the circular queue
/// \param cq Pointer to the circular queue struct
/// \param e An element to be pushed to the circular queue
void cq_enqueue(cqueue_t *cq, void *e);

/// Dequeuing a new element from the circular queue
/// \param cq Pointer to the circular queue struct
void *cq_dequeue(cqueue_t *cq);

/// Circular queue iterator
/// \param cq Pointer to the circular queue struct
/// \return An element of the queue
void *cq_iter_next(cqueue_t *cq);

/// Displaying the circular queue content
/// \param cq Pointer to the circular queue struct
void cq_display(cqueue_t *cq);

void *cq_get_row_by_index(cqueue_t *cq, int index);

void cq_remove_row_by_index(cqueue_t *cq, int index);

#endif
