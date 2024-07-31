#ifndef MUMHORS_MUMHORS_H
#define MUMHORS_MUMHORS_H
#include "bitmap.h"

#define PKMATRIX_MORE_ROW_ALLOCATION_SUCCESS 0
#define PKMATRIX_NO_MORE_ROWS_TO_ALLOCATE 1

#define VERIFY_SIGNATURE_VALID 0
#define VERIFY_SIGNATURE_INVALID 1
#define VERIFY_FAILED_NO_MORE_ROW 2

#define SIGN_SUCCESS 0
#define SIGN_NO_MORE_ROW_FAILED 1

/// Struct for MUMHORS signature
typedef struct mumhors_signature {
    unsigned char *signature; /* Signature of the message signed by the signer */
    unsigned int ctr; /* Weak message counter */
} mumhors_signature_t;

/// Struct for MUMHORS signer
typedef struct mumhors_signer {
    unsigned char *seed; /* Seed to generate the private keys and signatures */
    int seed_len; /* Size of the seed in terms of bytes */
    int t; /* HORS t parameter */
    int k; /* HORS k parameter */
    int l; /* HORS l parameter */
    int rt; /* Bitmap threshold (maximum) rows to allocate */
    int r; /* Number of bitmap matrix rows */
    bitmap_t bm; /* Bitmap for managing the private key utilization */
    mumhors_signature_t signature; /* Signature of the message signed by the signer */
} mumhors_signer_t;

/// Public key node
typedef struct public_key {
    int available_pks; /* Number of available public keys */
    int number; /* Public key row number */
    unsigned char **pks; /* An array of public keys */
    struct public_key *next; /* Pointer to the next row of the matrix */
} public_key_t;

/// Public key matrix (linked list)
typedef struct public_key_matrix {
    public_key_t *head; /* Pointer to the first public key row in the matrix */
    public_key_t *tail; /* Pointer to the last public key row in the matrix */
} public_key_matrix_t;

/// Struct for MUMHORS verifier
typedef struct mumhors_verifier {
    int t; /* HORS t parameter */
    int k; /* HORS k parameter */
    int l; /* HORS l parameter */
    int r; /* Total number of rows in public key matrix (=MUMHORS parameter l)*/
    int c; /* Number of columns in public key matrix (=HORS parameter t)*/
    int rt; /* Maximum rows to consider the matrix at a time */
    int active_pks; /* Number of available public keys in the active rows */
    int windows_size; /* Size of the window (#PKs) required for each operation. Same as Bitmap window size */
    int nxt_row_number; /* Next row number for allocating new row */
    public_key_matrix_t pk_matrix; /* Matrix (linked list) of public keys */
} mumhors_verifier_t;


/// Public key generator of the MUMHORS. In MUMHORS the private keys are generated from seed on fly during signing.
/// Hence, there is no need to generate a list of private keys as this consumes storage and is not efficient.
/// \param pk_matrix Pointer to the public key matrix struct
/// \param seed Seed to generate the public keys
/// \param seed_len Size of the seed in terms of bytes
/// \param row Number of matrix rows
/// \param col Number of matrix columns
void mumhors_pk_gen(public_key_matrix_t *pk_matrix, const unsigned char *seed, int seed_len, int row, int col);

/// Initializes a new MUMHORS signer
/// \param signer Pointer to MUMHORS signer struct
/// \param seed Seed to generate the private keys and signatures
/// \param seed_len Size of the seed in terms of bytes
/// \param t HORS t parameter
/// \param k HORS k parameter
/// \param l HORS l parameter
/// \param rt Bitmap threshold(maximum) rows to allocate
/// \param r Number of bitmap matrix rows
void mumhors_init_signer(mumhors_signer_t *signer, unsigned char *seed, int seed_len,
                         int t, int k, int l, int rt, int r);

/// Deletes the MUMHORS signer struct
/// \param signer Pointer to MUMHORS signer struct
void mumhors_delete_signer(mumhors_signer_t *signer);

/// Initializes a new MUMHORS verifier
/// \param verifier Pointer to MUMHORS verifier struct
/// \param pk_matrix Matrix (linked list) of public keys
/// \param t HORS t parameter
/// \param k HORS k parameter
/// \param l HORS l parameter
/// \param r Number of rows in the public key matrix
/// \param c Number of columns in the public key matrix
/// \param rt Maximum number of rows to consider in its window
/// \param window_size Size of the window required for each operation
void mumhors_init_verifier(mumhors_verifier_t *verifier, public_key_matrix_t pk_matrix, int t, int k, int l,
                           int r, int c, int rt, int window_size);

/// Deletes the MUMHORS verifier struct
/// \param verifier Pointer to MUMHORS verifier struct
void mumhors_delete_verifier(const mumhors_verifier_t *verifier);

/// Sign the message
/// \param signer Pointer to MUMHORS signer struct
/// \param message Pointer to the message to be signed
/// \param message_len Length of the message to be signed
/// \return SIGN_SUCCESS or SIGN_NO_MORE_ROW_FAILED
int mumhors_sign_message(mumhors_signer_t *signer, const unsigned char *message, int message_len);


/// Verifies the signature on the given message
/// \param verifier Pointer to MUMHORS verifier struct
/// \param signature Pointer to the signature
/// \param message Pointer to the message
/// \param message_len Message's length
/// \return
int mumhors_verify_signature(mumhors_verifier_t *verifier, const mumhors_signature_t *signature,
                             const unsigned char *message, int message_len);
#endif
