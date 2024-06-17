#ifndef MUMHORS_MUMHORS_H
#define MUMHORS_MUMHORS_H
#include <mumhors/bitmap.h>

#define PKMATRIX_MORE_ROW_ALLOCATION_SUCCESS 0
#define PKMATRIX_NO_MORE_ROWS_TO_ALLOCATE 1

#define VERIFY_SUCCESS 0
#define VERIFY_NO_MORE_ROW_FAILED 1


/// Struct for MUMHORS signer
typedef struct mumhors_signer {
    unsigned char *seed;   /* Seed to generate the private keys and signatures */
    int t;                  /* HORS t parameter */
    int k;                  /* HORS k parameter */
    int rt;                 /* Bitmap threshold (maximum) rows to allocate */
    int r;                  /* Number of bitmap matrix rows */
    bitmap_t bm;            /* Bitmap for managing the private key utilization */
} mumhors_signer_t;

/// Public key node
typedef struct public_key {
    int available_pks;          /* Number of available public keys */
    int number;                 /* Public key row number */
    unsigned char **pks;       /* An array of public keys */
    struct public_key *next;    /* Pointer to the next row of the matrix */
} public_key_t;

/// Public key matrix (linked list)
typedef struct public_key_matrix {
    public_key_t *head;         /* Pointer to the first public key row in the matrix */
    public_key_t *tail;         /* Pointer to the last public key row in the matrix */
} public_key_matrix_t;

/// Struct for MUMHORS verifier
typedef struct mumhors_verifier {
    int r;                      /* Total number of rows in public key matrix (=MUMHORS parameter l)*/
    int c;                      /* Number of columns in public key matrix (=HORS parameter t)*/
    int rt;                     /* Maximum rows to consider the matrix at a time */
    int active_pks;             /* Number of available public keys in the active rows */
    int windows_size;           /* Size of the window (#PKs) required for each operation. Same as Bitmap window size */
    int nxt_row_number;         /* Next row number for allocating new row */
    public_key_matrix_t pk_matrix;      /* Matrix (linked list) of public keys */
} mumhors_verifier_t;


/// Public key generator of the MUMHORS. In MUMHORS the private keys are generated from seed on fly during signing.
/// Hence, there is no need to generate a list of private keys as this consumes storage and is not efficient.
/// \param pk_matrix Pointer to the public key matrix struct
/// \param seed Seed to generate the public keys
/// \param row Number of matrix rows
/// \param col Number of matrix columns
void mumhors_pk_gen(public_key_matrix_t *pk_matrix, unsigned char *seed, int row, int col);


/// Initializes a new MUMHORS signer
/// \param signer Pointer to MUMHORS signer struct
/// \param seed Seed to generate the private keys and signatures
/// \param t HORS t parameter
/// \param k HORS k parameter
/// \param ir Bitmap initial rows to allocate
/// \param rt Bitmap threshold(maximum) rows to allocate
/// \param r Number of bitmap matrix rows
void mumhors_init_signer(mumhors_signer_t *signer, unsigned char *seed,
                         int t, int k, int ir, int rt, int r);

/// Deletes the MUMHORS signer struct
/// \param signer Pointer to MUMHORS signer struct
void mumhors_delete_signer(mumhors_signer_t *signer);

/// Initializes a new MUMHORS verifier
/// \param verifier Pointer to MUMHORS verifier struct
/// \param pk_matrix Matrix (linked list) of public keys
/// \param r Number of rows in the public key matrix
/// \param c Number of columns in the public key matrix
/// \param rt Maximum number of rows to consider in its window
/// \param window_size Size of the window required for each operation
void mumhors_init_verifier(mumhors_verifier_t *verifier, public_key_matrix_t pk_matrix, int r, int c,
                           int rt, int window_size);

/// Deletes the MUMHORS verifier struct
/// \param signer Pointer to MUMHORS verifier struct
void mumhors_delete_verifier(mumhors_verifier_t *verifier);


int mumhors_verify(mumhors_verifier_t *verifier, int *indices, int num_indices);


//DEBUG
void pk_display(mumhors_verifier_t *verifier);

void mumhors_invalidate_public_pks(mumhors_verifier_t *verifier, int *indices, int num_indices);

#endif
