#ifndef MUHORS_MUHORS_H
#define MUHORS_MUHORS_H
#include <muhors/bitmap.h>

/// Struct for MUHORS signer
typedef struct muhors_signer{
    unsigned char * seed;   /* Seed to generate the private keys and signatures */
    int t;                  /* HORS t parameter */
    int k;                  /* HORS k parameter */
    int ir;                 /* Bitmap initial rows to allocate */
    int rt;                 /* Bitmap threshold(maximum) rows to allocate */
    int l;                  /* Number of bitmap matrix rows */
    bitmap_t bm;    /* Bitmap for managing the private key utilization */
}muhors_signer_t;


/// Public key node
typedef struct public_key{
    int number;             /* Public key row number */
    unsigned char ** pks;   /* An array of public keys */
    int available_pks;      /* Number of available public keys */
    struct public_key *next;     /* Pointer to the next row of the matrix */
}public_key_t;

/// Public key matrix (linked list)
typedef struct public_key_matrix{
    public_key_t *head;     /* Pointer to the first public key row in the matrix */
    public_key_t *tail;     /* Pointer to the last public key row in the matrix */
}public_key_matrix_t;


/// Struct for MUHORS verifier
typedef struct muhors_verifier{
    int r;      /* Number of rows in public key matrix (=MUHORS parameter l)*/
    int c;      /* Number of columns in public key matrix (=HORS parameter t)*/
    int rt;     /* Maximum rows to consider in its window */
    int active_pks;    /* Number of available public keys in the active rows */
    int windows_size;   /* Size of the window (#PKs) required for each operation */
    int next_row_number; /* Next number for the rows */
//    int active_rows;    /* Number of available rows */
    public_key_matrix_t pk_matrix;      /* Matrix (linked list) of public keys */
}muhors_verifier_t;


/// Initializes a new MUHORS signer
/// \param signer Pointer to MUHORS signer struct
/// \param seed Seed to generate the private keys and signatures
/// \param t HORS t parameter
/// \param k HORS k parameter
/// \param ir Bitmap initial rows to allocate
/// \param rt Bitmap threshold(maximum) rows to allocate
/// \param l Number of bitmap matrix rows
void muhors_init_signer(muhors_signer_t *signer, unsigned char *seed,
                       int t, int k, int ir, int rt, int l);

/// Deletes the MUHORS signer struct
/// \param signer Pointer to MUHORS signer struct
void muhors_delete_signer(muhors_signer_t *signer);

/// Initializes a new MUHORS verifier
/// \param verifier Pointer to MUHORS verifier struct
/// \param r Number of rows in the public key matrix
/// \param c Number of columns in the public key matrix
/// \param rt Maximum number of rows to consider in its window
/// \param window_size Size of the window required for each operation
void muhors_init_verifier(muhors_verifier_t *verifier, int r, int c, int rt, int window_size);


/// Deletes the MUHORS verifier struct
/// \param signer Pointer to MUHORS verifier struct
void muhors_delete_verifier(muhors_verifier_t *verifier);


void muhors_verify(muhors_verifier_t *verifier, int * indices, int num_indices);



//DEBUG
void pk_display(muhors_verifier_t *verifier);
void muhors_invalidate_public_pks(muhors_verifier_t *verifier, int * indices, int num_indices);
#endif
