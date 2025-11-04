# 🔐 AES File Encryption (OpenSSL)

This project implements AES-128 file encryption and decryption for `.TGA` image files using the **OpenSSL EVP API** in C++.

The implementation supports both **ECB** and **CBC** block cipher modes.  
It is based on a ProgTest assignment focusing on memory-efficient and stream-based encryption.

---

## ⚙️ Features

- AES-128 encryption and decryption
- ECB and CBC block cipher modes
- Streamed file processing (no full file loading into memory)
- Binary-safe header preservation for TGA files
- Automatic key and IV generation if not provided
- Validation of OpenSSL cipher parameters
- Cross-platform CMake build system

---

## 🧱 Project Structure
```
aes-file-encryption-openssl/
│
├── main.cpp              # Main source file with encrypt/decrypt implementations
├── CMakeLists.txt        # CMake build configuration
├── .gitignore            # Ignored build and IDE artifacts
│
├── testfiles/            # Test data for validation
│   ├── homer-simpson.TGA
│   ├── homer-simpson_enc_ecb.TGA
│   ├── homer-simpson_enc_cbc.TGA
│   ├── UCM8.TGA
│   ├── UCM8_enc_ecb.TGA
│   ├── UCM8_enc_cbc.TGA
│   ├── image_1.TGA
│   ├── image_2.TGA
│   ├── ref_*.TGA         # Reference encrypted/decrypted results
│   └── out_file.TGA      # Generated output for tests
│
└── build/                # (Ignored) local build directory
```

---

## 🔧 Build Instructions

### 1️⃣ Install dependencies

#### On macOS (via Homebrew)
```bash
brew install cmake openssl
```

#### On Debian/Ubuntu
```bash
sudo apt update
sudo apt install cmake libssl-dev build-essential
```

---

### 2️⃣ Configure and build

```bash
cmake -S . -B build
cmake --build build
```

The resulting binary will be located at:

```
./build/aes_file_encryption_openssl
```

---

### 3️⃣ Run the program

Simply run:
```bash
./build/aes_file_encryption_openssl
```

All tests in `main.cpp` will execute automatically, validating both ECB and CBC encryption modes against reference `.TGA` files.

---

## 🧠 Implementation Details

- Uses `EVP_EncryptInit_ex`, `EVP_EncryptUpdate`, `EVP_EncryptFinal_ex`
  and their decryption counterparts.
- `crypto_config` structure manages:
  - Cipher name (e.g. `"AES-128-CBC"`)
  - Secret key (`std::unique_ptr<uint8_t[]>`)
  - Initialization vector (IV)
  - Key and IV lengths
- Includes a helper function `check_config()` for automatic key/IV validation and generation.
- The first 18 bytes (TGA header) are **copied unencrypted**, per assignment rules.

---

## 🧪 Testing

The included reference files (e.g., `ref_*.TGA`) are used to verify correctness.  
`compare_files()` performs binary comparison to ensure exact match between the decrypted output and the original.

---


---

**Author:** Yelyzaveta Kononenko  
**Course:** Applied Cryptography / Secure Programming  
**Year:** 2025