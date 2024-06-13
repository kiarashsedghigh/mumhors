#ifndef MUHORS_BITMAP_H
#define MUHORS_BITMAP_H

#include <muhors/cqueue.h>

typedef unsigned char *vec_t;
typedef struct row *row_t;

/// Row data structure to represent each row with its number and data
struct row {
    int row_number;     /* Row number */
    vec_t data;         /* The pointer to the actual data of row */
};

/// Bitmap structure
typedef struct bitmap {
    /* Hyperparameters of bitmap structure */
    int r;        /* Total number of rows */
    int cB;       /* Total number of columns in terms of Bytes */
    int ir;       /* Initial number of rows to be allocated */
    int rt;        /* Threshold on number of active rows */
    int next_row_number;  /* Row number of the next usable row */
    int active_rows;    /* Number of active rows */
    int num_ones_in_active_rows; /* Number of set bits in the active rows */

    cqueue_t bitmap_matrix;  /* The matrix of rows (circular queue) containing the rows */
} bitmap_t;

/// Initializing the bitmap structure
/// \param rows Number of rows
/// \param cols Number of columns in terms of (bits)
/// \param init_rows Initial number of rows
/// \param row_threshold Threshold on number of rows
void bitmap_init(bitmap_t *bm, int rows, int cols, int init_rows, int row_threshold);

/// Deleting the bitmap structure
/// \param bm Pointer to the bitmap structure
void bitmap_delete(bitmap_t *bm);

/// Unset an index in the bitmap
/// \param bm Pointer to the bitmap structure
/// \param index Index to be unset
void bitmap_unset(bitmap_t *bm, int index);

/// Displaying the bitmap
/// \param bm Pointer to the bitmap structure
void bitmap_display(bitmap_t *bm);

/// Allocate a new row and adds it to the bitmap matrix
/// \param bm Pointer to the bitmap structure
void bitmap_allocate_new_row(bitmap_t *bm);

/// Remove a row from the bitmap matrix based on its index
/// \param bm Pointer to the bitmap structure
/// \param index The index of the row
void bitmap_remove_row(bitmap_t *bm, int index);

/// Unset bit index in the specified window
/// \param bm Pointer to the bitmap structure
/// \param index Bit index to be unset
/// \param windows_size The windows size defines the number of first 1s in the matrix
void bitmap_unset_index_in_window(bitmap_t *bm, int index, int windows_size);

#endif