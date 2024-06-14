#ifndef MUHORS_BITMAP_H
#define MUHORS_BITMAP_H

#include <muhors/cqueue.h>

typedef unsigned char *vec_t;
typedef struct row *row_t;
typedef struct bit_object bitobj_t;

/// A linked lists of bit_object to hold the bits resulted from compressing bitmap matrix
typedef struct bitmap_rowcompressor_object{
    int total_size;     /* Total number of allowed nodes to store */
    int current_nodes;  /* Current number of nodes in  the list */
    bitobj_t * head;
    bitobj_t * tail;
}bitmap_rowcom_t;

/// A struct representing a bit with its row and column indices
struct bit_object{
    int row;    /* Bit row index */
    int col;    /* Bit column index */
    bitobj_t * next;  /* Pointer to the next bit object */
};

/// Row data structure to represent each row with its number and data
struct row {
    int row_number;     /* Row number */
    vec_t data;         /* The pointer to the actual data of row */
    row_t next;         /* Pointer to the next row in the list */
};

/// Bitmap matrix containing the rows of the bitmap matrix
typedef struct bitmap_matrix{
    row_t head;
    row_t tail;
}bitmap_matrix_t;


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

//    cqueue_t bitmap_matrix;  /* The matrix of rows (circular queue) containing the rows */
    bitmap_matrix_t bitmap_matrix; /* The matrix of rows (linked list) containing the rows */

    bitmap_rowcom_t compressed_rows;  /* A linked list of bits from compressed rows */
} bitmap_t;

/// Initializing the bitmap structure
/// \param rows Number of rows
/// \param cols Number of columns in terms of (bits)
/// \param init_rows Initial number of rows
/// \param row_threshold Threshold on number of rows
void bitmap_init(bitmap_t *bm, int rows, int cols, int init_rows, int row_threshold, int row_compressed_list_length);

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

/// Allocate more new rows
/// \param bm Pointer to the bitmap structure
void bitmap_allocate_more_row(bitmap_t *bm);

/// Remove a row from the bitmap matrix based on its index
/// \param bm Pointer to the bitmap structure
/// \param index The index of the row
void bitmap_remove_row(bitmap_t *bm, int index);

/// Unset bit index in the specified window
/// \param bm Pointer to the bitmap structure
/// \param indices Array of indices to be unset
/// \param windows_size The windows size defines the number of first 1s in the matrix
void bitmap_unset_index_in_window(bitmap_t *bm, int * indices, int num_index, int windows_size);

// For debugging still here
void bitmap_rowcompressor_addnode(bitmap_rowcom_t * bmrcom, int row, int col);
void bitmap_rowcompressor_display(bitmap_rowcom_t * bmrcom);
#endif