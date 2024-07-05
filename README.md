# MUM-HORS
<p>
<a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT License-%23ffb243?style=flat-square"></a>
</p>

Maximum Utilization Multiple HORS (MUM-HORS) is a new implementation of 
HORS signature in which the utilization of private keys and
public keys, is maximum. This is critical in situations where to 
reduce the signature size, we use small `k` but large `t` values
for HORS. In such circumstances, the waste of keys, `t-k` is significant.
Hence, a new approach to mitigate this waste is critical.

## Requirements
Before you begin, ensure you have met the following requirements:
- **Libtomcrypt**: v1.18.2 or higher
- **CMake**: 3.22 or higher ![CMake](https://img.shields.io/badge/cmake-3.22%2B-blue.svg)


# Installation
To install first:
```
git clone https://github.com/kiarashsedghigh/mumhors.git
```
To build the binary:
```
mkdir build
cd buid
cmake ../ .
cmake --build . --target test
```

# Running
To run the program:
```
test/muhors T K L R RT TESTS SEED_FILE 
```
where `T` and `K` are HORS parameters, `R` denotes the totla number
of rows to be allocated, `RT` denotes row threshold (maximum number of rows),
`TESTS` denotes number of test cases, and `SEED_FILE` is the path to the seed file.
