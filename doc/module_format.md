# KyroOS Signed Kernel Module Format (.ko)

## 1. Overview

KyroOS kernel modules (`.ko`) are standard ELF64 relocatable object files with a custom signature block appended to the end. This allows for a standard build process while ensuring the kernel can verify the module's integrity and authenticity before loading it.

## 2. Structure

The structure of a signed `.ko` file is as follows:

```
+-----------------------------------+
|      Standard ELF64 Object        |
|      (Relocatable, .o file)       |
+-----------------------------------+
|      KyroOS Signature Footer      |
|      (136 bytes)                  |
+-----------------------------------+
```

### 2.1. ELF Object

The main body of the file is a standard ELF64 object file produced by the compiler (`gcc -c ...`). It contains the module's code, data, and relocation information. The kernel module loader is responsible for parsing this ELF data and linking it into the running kernel.

### 2.2. Signature Footer

A fixed-size 136-byte footer is appended to the end of the ELF file.

| Offset (from end of file) | Size (bytes) | Description                                |
|---------------------------|--------------|--------------------------------------------|
| -136                      | 128          | Signature (e.g., RSA-2048) of the SHA-256 hash of the ELF object. |
| -8                        | 8            | Magic number: `0x4B524F534D4F444C` (`KROSMODL`) |

## 3. Verification Process

When the kernel is asked to load a module:
1. It first looks for the 8-byte magic number at the end of the file. If not present, the module is rejected.
2. It reads the 128-byte signature from the footer.
3. It calculates the SHA-256 hash of the entire file *excluding* the 136-byte footer (i.e., the ELF object portion).
4. It uses a public key embedded within the kernel to verify the signature against the calculated hash.
5. If the signature is valid, the kernel proceeds to load the ELF object. Otherwise, the module is rejected with a security error.

## 4. Tooling

A userspace tool will be responsible for signing modules. This tool will:
1. Take a compiled `.ko` file (a standard ELF object) as input.
2. Calculate its SHA-256 hash.
3. Use a private key to sign the hash.
4. Append the 128-byte signature and the 8-byte magic number to the end of the `.ko` file.
