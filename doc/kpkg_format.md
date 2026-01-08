# KyroOS Package Format (.kpkg)

## 1. Overview

The KyroOS package format, `.kpkg`, is a simple, uncompressed archive format designed for distributing statically linked binaries and their associated metadata. Its simplicity facilitates easy creation and parsing by a userspace package manager.

## 2. Structure

A `.kpkg` file is a concatenation of a metadata block and a data block (the ELF executable).

```
+-----------------------------------+
|      Header (16 bytes)            |
+-----------------------------------+
|      Metadata (JSON format)       |
|      (size from header)           |
+-----------------------------------+
|      ELF Executable Data          |
|      (size from header)           |
+-----------------------------------+
```

### 2.1. Header

The file begins with a 16-byte fixed-size header.

| Offset | Size (bytes) | Description                                |
|--------|--------------|--------------------------------------------|
| 0      | 4            | Magic number: `0x4B504B47` (`KPKG`)        |
| 4      | 4            | Metadata size (in bytes), 32-bit little-endian. |
| 8      | 8            | ELF data size (in bytes), 64-bit little-endian.   |

### 2.2. Metadata Block

Immediately following the header is a UTF-8 encoded JSON text block. The size of this block is specified in the header.

The JSON object contains the following key-value pairs:
- `name` (string, required): The name of the package.
- `version` (string, required): The package version (e.g., "1.0.0").
- `arch` (string, required): The target architecture (e.g., "x86_64").
- `description` (string, optional): A brief description of the package.
- `dependencies` (array of strings, optional): A list of other packages this package depends on.

**Example `pkg.json`:**
```json
{
  "name": "my-app",
  "version": "1.2.0",
  "arch": "x86_64",
  "description": "A cool application for KyroOS.",
  "dependencies": [
    "libc-kyro-1.0.0"
  ]
}
```

### 2.3. Data Block

Immediately following the metadata block is the raw binary data of the statically linked ELF executable. Its size is specified in the header.

## 3. Tooling

The userspace package manager (`kpm`) will be responsible for:
- Creating `.kpkg` files by combining a metadata file and an ELF executable.
- Parsing `.kpkg` files to extract metadata and the ELF file.
- Installing packages by placing the ELF executable in a designated system directory (e.g., `/bin`).
- Managing dependencies.
- Fetching packages from official and local repositories (requires networking and filesystem syscalls).
