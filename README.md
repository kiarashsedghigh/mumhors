# MUM-HORS
Maximum Utilization Multiple HORS (MUM-HORS) is a new implementation of 
HORS signature in which the utilization of private keys and
public keys, is maximum. This is critical in situations where to 
reduce the signature size, we use small `k` but large `t` values
for HORS. In such circumstances, the waste of keys, `t-k` is significant.
Hence, a new approach to mitigate this waste is critical.

# Installation
To install first:
```
git clone https://github.com/kiarashsedghigh/muhors.git
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
