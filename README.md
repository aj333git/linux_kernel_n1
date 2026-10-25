# Linux Kernel Module: Pointer Types, Alignment, and Endianness

## 1. Overview

This project explores how pointer types affect memory access, pointer arithmetic, and data interpretation in C, using a runnable Linux kernel module.

The focus is on understanding what happens when a byte-oriented pointer is interpreted as a pointer to a 16-bit integer, and why correct memory access requires more than simply casting one pointer type to another.

### Concepts covered

- Memory access granularity
- Pointer arithmetic
- Pointer type conversion
- Memory alignment
- Endianness
- Buffer boundaries
- Safe handling of potentially unaligned data using `get_unaligned()`

The central idea is that a pointer type determines how pointer arithmetic is scaled and how the compiler interprets memory accesses. Alignment, byte order, and buffer validity determine whether those accesses are appropriate.

---

## 2. Understanding Pointer Arithmetic

Consider two pointers referencing the same buffer:

```c
const unsigned char *byte_ptr = buffer;
const u16 *word_ptr = (const u16 *)buffer;
```

Although both pointers may contain the same numerical address, they have different types.

```c
byte_ptr + 1;  /* Advances by 1 byte */
word_ptr + 1;  /* Advances by sizeof(u16) bytes */
```

On Linux systems where `u16` is a 16-bit type:

```text
                         BUFFER
              +------+------+------+------+------+
              | B[0] | B[1] | B[2] | B[3] | B[4] |
              +------+------+------+------+------+
                 ^      ^
                 |      |
                 |      +-- byte_ptr + 1
                 |
                 +--------- byte_ptr

              +------+------+------+------+------+
              | B[0] | B[1] | B[2] | B[3] | B[4] |
              +------+------+------+------+------+
                 ^             ^
                 |             |
                 |             +-- word_ptr + 1
                 |
                 +---------------- word_ptr
```

The address calculations are:

```text
byte_ptr + 1
    = address(byte_ptr) + 1 * sizeof(unsigned char)
    = address(byte_ptr) + 1

word_ptr + 1
    = address(word_ptr) + 1 * sizeof(u16)
    = address(word_ptr) + 2
```

### Important distinction

Pointer arithmetic advances in units of the pointed-to type, not in units of the pointer itself.

Changing a pointer's type changes how arithmetic is calculated. It does not change the underlying bytes stored in memory.

---

## 3. Memory Access Granularity

A byte-oriented pointer is useful when examining memory one byte at a time.

```c
const unsigned char *byte_ptr = buffer;
```

A pointer to `u16` represents access to 16-bit values:

```c
const u16 *word_ptr = (const u16 *)buffer;
```

Conceptually:

```text
Byte-oriented access:

buffer
  |
  v
+------+------+------+------+------+
| 0x34 | 0x12 | 0x78 | 0x56 | 0xAB |
+------+------+------+------+------+
   |      |      |      |
   +------+------+------+
          |
          v
  Individual byte inspection


16-bit-oriented access:

buffer
  |
  v
+-------------+-------------+------+
|    0x1234   |    0x5678   | 0xAB |
+-------------+-------------+------+
```

The second diagram illustrates grouping two bytes into a 16-bit value on a little-endian system. The actual interpretation depends on the machine's byte order.

The type of pointer influences how the compiler generates memory accesses, but it does not guarantee that the target address satisfies the requirements of that access.

---

## 4. Alignment: Casting Does Not Guarantee Alignment

A cast changes the type through which an address is accessed. It does not automatically align the address.

```c
const unsigned char *byte_ptr = buffer;
const u16 *word_ptr = (const u16 *)byte_ptr;
```

Suppose the buffer begins at an address that is not suitably aligned for a 16-bit access.

```text
Memory addresses:

Address       Byte
-------       ----
0x1000        0x34
0x1001        0x12
0x1002        0x78
0x1003        0x56

Aligned candidate:
              0x1000

Potentially unaligned candidate:
              0x1001
```

Whether a particular address is suitably aligned depends on the target architecture and the access requirements.

Depending on the architecture and access, misaligned memory access may:

- Work transparently but incur additional cost.
- Require special handling.
- Trigger an alignment fault.
- Cause problems when interacting with hardware or particular memory regions.

Do not assume that an address is correctly aligned merely because it was converted to a `u16 *`.

### Practical rule

Use a byte-oriented pointer for byte-level inspection. Use a typed pointer only when alignment, object representation, and access requirements are satisfied.

---

## 5. Endianness: The Same Bytes, Different Values

Endianness describes the order in which the bytes of a multi-byte value are represented in memory.

Consider these two bytes:

```text
Address       Byte
-------       ----
0x1000        0x34
0x1001        0x12
```

Interpreting them as a 16-bit integer produces different results depending on byte order.

### Little-endian

The least significant byte appears first:

```text
Memory order:
+--------+--------+
| 0x34   | 0x12   |
+--------+--------+

16-bit value:
0x1234
```

### Big-endian

The most significant byte appears first:

```text
Memory order:
+--------+--------+
| 0x34   | 0x12   |
+--------+--------+

16-bit value:
0x3412
```

This is why interpreting a byte buffer as a multi-byte integer can produce different values across systems.

When processing network protocols, do not assume that the host's native byte order matches the protocol's byte order. Use the appropriate conversion functions or protocol-specific helpers.

For example, user-space networking code commonly uses:

```c
uint16_t host_value = ntohs(network_value);
```

Kernel code has its own types and conversion helpers, such as `__be16`, `be16_to_cpu()`, and `cpu_to_be16()`.

---

## 6. Buffer Boundaries: A 16-Bit Read Requires Two Bytes

A 16-bit read requires two accessible bytes starting at the selected address.

```text
Buffer:

+------+------+------+------+
| B[0] | B[1] | B[2] | B[3] |
+------+------+------+------+
   ^      ^
   |      |
   +------+  Valid two-byte region

A 16-bit read beginning at B[3]:

+------+------+------+------+
| B[0] | B[1] | B[2] | B[3] |
+------+------+------+------+
                         ^
                         |
                    Only one byte
                    remains in buffer
```

If a buffer contains four bytes, a two-byte read may begin at offsets 0, 1, or 2, assuming the address is otherwise suitable for the access.

A read beginning at offset 3 would extend beyond the four-byte buffer.

Out-of-bounds accesses can lead to undefined behavior in C, memory corruption, or kernel faults.

### General rule

Before reading a value of size `N`, verify that at least `N` bytes remain accessible from the selected position.

For an offset `offset` into a buffer of length `length`, a safe bounds condition is:

```c
if (offset <= length && length - offset >= sizeof(u16)) {
    /* Two bytes remain available. */
}
```

The buffer itself must be valid, and any additional alignment or data-format requirements must still be satisfied.

---

## 7. Handling Potentially Unaligned Data in the Linux Kernel

The Linux kernel provides helpers for accessing values that may not be naturally aligned.

One such helper is:

```c
#include <asm/unaligned.h>

u16 value = get_unaligned((const u16 *)(buffer + offset));
```

This is an illustrative pattern, not a complete bounds-checked implementation. Verify that the buffer contains at least `sizeof(u16)` bytes beginning at `offset` before performing the access.

`get_unaligned()` addresses alignment-related access concerns. It does not independently validate the buffer length or convert a value from network byte order to host byte order.

For protocol parsing, the correct sequence is conceptually:

```text
Receive or obtain buffer
          |
          v
Validate buffer length
          |
          v
Validate field offset
          |
          v
Read using an appropriate access method
          |
          v
Convert byte order if required
          |
          v
Interpret the resulting value
```

This separation is important in kernel code, where invalid memory accesses can affect system stability.

---

## 8. Build the Kernel Module

This repository uses a Linux kernel module named `ptr_demo`.

### Prerequisites

- Linux system with kernel headers installed for the running kernel.
- A compatible compiler and build toolchain.
- The module source file and its Makefile.
- MOK signing key and certificate, if required by the system's Secure Boot configuration.

### Step 1: Build

Run the build command in the directory containing the Makefile:

```bash
make
```

The build should produce:

```text
ptr_demo.ko
```

If the build fails, inspect the compiler output and confirm that the installed kernel headers match the running kernel.

### Step 2: Sign the module

If your system requires a module signature, sign the generated module using the kernel's `sign-file` utility:

```bash
sudo /usr/src/linux-headers-$(uname -r)/scripts/sign-file \
    sha256 \
    ~/kernel_keys/MOK.key \
    ~/kernel_keys/MOK.crt \
    ptr_demo.ko
```

This command uses the private key and certificate at the specified paths. Adjust those paths if your signing keys are stored elsewhere.

Signing the module does not automatically make the certificate trusted by the running kernel. The certificate must be enrolled or otherwise trusted according to your system's Secure Boot and module-signing configuration.

### Step 3: Load the module

```bash
sudo insmod ptr_demo.ko
```

Check the kernel log:

```bash
sudo dmesg | tail -20
```

Inspect the output generated by the module.

### Step 4: Unload the module

```bash
sudo rmmod ptr_demo
```

Check the kernel log again:

```bash
sudo dmesg | tail -5
```

The module should unload successfully if it has no outstanding references or other conditions preventing removal.

---

## 9. Complete Development Workflow

```text
+-------------------------+
| Edit module source      |
+------------+------------+
             |
             v
+-------------------------+
| make                    |
+------------+------------+
             |
             v
+-------------------------+
| Sign ptr_demo.ko        |
+------------+------------+
             |
             v
+-------------------------+
| insmod ptr_demo.ko      |
+------------+------------+
             |
             v
+-------------------------+
| Inspect dmesg            |
+------------+------------+
             |
             v
+-------------------------+
| rmmod ptr_demo          |
+------------+------------+
             |
             v
+-------------------------+
| Inspect unload messages |
+-------------------------+
```

This workflow allows the module to be rebuilt, loaded, observed, and unloaded during experimentation.

Kernel modules execute with kernel privileges. Test experimental modules on a system where a crash or reboot would not cause data loss.

---

## 10. Key Takeaways

1. Pointer arithmetic advances in units of the pointed-to type.
2. Casting a pointer changes its type, not the alignment of its address.
3. A multi-byte value's interpretation depends on endianness.
4. A valid memory access must remain within the accessible buffer.
5. `get_unaligned
