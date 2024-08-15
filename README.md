# MUM-HORS
<p>
<a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT License-%23ffb243?style=flat-square"></a>
<a href="CMAKE"><img src="https://img.shields.io/badge/cmake-3.22%2B-blue.svg">
</p>


Maximum Utilization Multiple HORS (MUM-HORS) is a new implementation of 
HORS signature in which the utilization of private keys and
public keys, is maximum. This project introduces a novel bitmap data structure designed to maximize key usage within the HORS (Hash to Obtain Random Subset) digital signature scheme. The bitmap is a critical component that efficiently manages key allocation and usage, ensuring optimal performance and security. The data structure is provided in two distinct flavors:

1. **Linked List**
2. **Circular Queue**

Each implementation `(src/utils/bitmap.c)` is tailored to different use cases, offering flexibility depending on the system's requirements.

### Bitmap Variants

#### 1. Linked List Bitmap
The linked list variant of the bitmap is designed for scenarios where dynamic resizing and flexibility are crucial. Each bit represents a key in the HORS signature scheme, and the linked list structure allows for efficient key addition, deletion, and management without the overhead of shifting elements. This makes it ideal for environments where keys are frequently added or removed.

- **Key Features:**
    - Dynamic size adjustment.

#### 2. Circular Queue Bitmap
The circular queue variant is optimized for scenarios where a fixed number of keys are managed
- **Key Features:**
    - Fixed-size structure for predictable memory usage.



## Requirements
Before you begin, ensure you have met the following requirements:
- **Libtomcrypt**: v1.18.2 or higher
- **Openssl**
- **CMake**: 3.22 or higher 

# Building
To install first:
```
git clone https://github.com/kiarashsedghigh/mumhors.git
```
To build the binary:
```
mkdir build
cd buid
```
with bitmap as linked list:
```
cmake -S .. -B . -DCMAKE_C_FLAGS="-DBITMAP_LIST"
make
```
with bitmap as array:
```
cmake -S .. -B . -DCMAKE_C_FLAGS="-DBITMAP_ARRAY -DROW_THRESHOLD=RT -DBIT_VECTOR=VECTOR_SIZE"
make
```
where `VECTOR_SIZE` is in bits.

Add `-DJOURNAL` to get performance report of the bitmap.

# Running
To run the program:
```
$ ./muhors T K L R RT TESTS SEED_FILE 
```
where `T`, `K`, `L` are HORS parameters, `R` denotes the total number
of rows to be allocated, `RT` denotes row threshold (maximum number of rows),
`TESTS` denotes number of test cases, and `SEED_FILE` is the path to the seed file. Create a 
seed file manually if no exists.

# Example
## Build
```
cmake -S .. -B . -DCMAKE_C_FLAGS="-DJOURNAL -DBITMAP_ARRAY -DROW_THRESHOLD=11 -DBIT_VECTOR=1024"
make
```

## Run 
To sign 2<sup>20</sup> messages with 128-bit security, we can set parameters 
as `t=1024`, `k=25`, `l=256`, `r=25601`, `rt=11`, `tests=1048576`:
```
$ ./mumhors 1024 25 256 25601 11 1048576 ./seed_file
```
![Running_kg_test](https://github.com/kiarashsedghigh/mumhors/blob/main/figures/run_kg_test.png)

# ⚠️ Parameter setting 
Make sure when using array-based version, the parameters when building the project, 
matches the parameters passed in command line. In other words, `VECTOR_SIZE` should match
`T` and `RT` in `-DROW_THRESHOLD` matches `RT` when running the program.
