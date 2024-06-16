#ifndef MUHORS_BITMAP_H
#define MUHORS_BITMAP_H

#define BITMAP_MORE_ROW_ALLOCATION_SUCCESS 0
#define BITMAP_NO_MORE_ROWS_TO_ALLOCATE 1
#define BITMAP_UNSET_BITS_SUCCESS 0
#define BITMAP_UNSET_BITS_FAILED 1


#ifdef JOURNAL
/// A group of journaling information which show the performance of the bitmap
typedef struct bitmap_journaling{
    int cnt_alloc_more_rows;        /* Number of calls to the row allocation procedure */
    int cnt_cleanup_call;           /* Number of calls to the cleanup procedure */
    int cnt_count_unset;            /* Number of calls to the unset procedure */
    int cnt_discarded_bits;         /* Counting the number of discard bits */
}bitmap_journaling_t;
#endif

/// Row data structure to represent each row with its meta parameters and data
typedef struct row {
    int number;                 /* Row number */
    int set_bits;               /* Number of set bits (1s) in the row */
    unsigned char *data;        /* The pointer to the bytes of the row */
    struct row *next;           /* Pointer to the next row in the list */
}row_t;

/// Bitmap matrix containing the rows of the bitmap matrix
typedef struct bitmap_row_matrix {
    row_t *head;        /* Pointer to the first row of the matrix */
    row_t *tail;        /* Pointer to the last row of the matrix. Tail pointer is used for fast insertion. */
} bitmap_row_matrix_t;

/// Bitmap structure
typedef struct bitmap {
    int r;                      /* Total number of rows */
    int cB;                     /* Total number of columns in terms of Bytes */
    int rt;                     /* Threshold on number of active rows */
    int nxt_row_number;         /* Row number of the next usable row */
    int active_rows;            /* Number of active rows */
    int set_bits;               /* Number of set bits in the active rows */
    int window_size;            /* Size of the window for the operations */
    bitmap_row_matrix_t bitmap_matrix;      /* The matrix of rows (linked list) containing the rows */
#ifdef JOURNAL
    bitmap_journaling_t bitmap_report;      /* Report of the bitmap execution performance */
#endif
} bitmap_t;

/// Initializing the bitmap structure
/// \param rows Number of rows
/// \param cols Number of columns in terms of (bits)
/// \param row_threshold Threshold on number of rows (maximum number of rows at a time)
/// \param window_size Window size for operations
void bitmap_init(bitmap_t *bm, int rows, int cols, int row_threshold, int window_size);

/// Deleting the bitmap structure
/// \param bm Pointer to the bitmap structure
void bitmap_delete(bitmap_t *bm);

/// Unset bit index in the specified window
/// \param bm Pointer to the bitmap structure
/// \param indices Array of indices to be unset
/// \return BITMAP_UNSET_BITS_SUCCESS or BITMAP_UNSET_BITS_FAILED
int bitmap_unset_index_in_window(bitmap_t *bm, int *indices, int num_index);

#ifdef JOURNAL
/// Presents a report of the bitmap performance
/// \param bm Pointer to the bitmap structure
void bitmap_report(bitmap_t *bm);
#endif


// Debug purpose
void bitmap_display(bitmap_t *bm);

#endif