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
cmake ../
make
```
To install the binary and header files
```
sudo make install
```

# Running
To run the program:
```
muhors T K L IR RT TESTS 
```
where `T` and `K` are HORS parameters, `IR` denotes the number
of initial rows to be allocated, `RT` denotes row threshold (maximum number of rows), and
`TESTS` denotes number of test cases.
