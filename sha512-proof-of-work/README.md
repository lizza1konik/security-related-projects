# 🧩 SHA-512 Proof-of-Work Generator

A small C++ project demonstrating **hash-based proof-of-work** generation using the **OpenSSL** cryptographic library.  
The program randomly generates byte strings and searches for one whose **SHA-512 hash starts with a specified number of leading zero bits** — similar to how mining puzzles work in blockchain systems.

---

## 🚀 Features
- Generates random byte sequences using OpenSSL’s `RAND_bytes`
- Computes SHA-512 hashes via OpenSSL EVP API
- Checks bit-level constraints (leading zero bits)
- Converts raw bytes to hexadecimal representation
- Demonstrates basic usage of OpenSSL for secure cryptographic operations

---

## 🧠 Technologies
- **Language:** C++22
- **Libraries:** OpenSSL 3.x (`libssl`, `libcrypto`)
- **Build system:** CMake
- **Platform:** macOS / Linux (tested on Apple Silicon)

---

## 🛠️ Build Instructions

### 1️⃣ Install dependencies
On macOS (via Homebrew):
```bash
brew install cmake openssl
```

On Debian/Ubuntu:
```bash
sudo apt update
sudo apt install cmake libssl-dev build-essential
```

---

### 2️⃣ Configure and build
```bash
cmake -S . -B build -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
cmake --build build
```

This will:
- Configure the build environment (create the `build/` folder)
- Locate OpenSSL headers and libraries
- Compile and link the program into `build/sha512_proof_of_work`

---

### 3️⃣ Run the program
```bash
./build/sha512_proof_of_work
```

If you see a permissions error, run:
```bash
chmod +x ./build/sha512_proof_of_work
```

---

## 🧩 Example Output
```
Trying to find a hash strating with 0 zeros
Hash found: 804a41cbb957ec5d5838c5098cf17540636ffc993ad96cf37cf62ac42fe162fb04077f69b8d4a8153892a6e9a51fa47d1570e17382a8500c11e3f4109296da15
String is: 8dde011433338b0334385b45830a852620c943aa89fd25c6c60e06c24dfcca47cf8eaa78125ead3dfbf79e208fcb5c868a89ad3c6fe3bc858fa013d6ef2f134b

Trying to find a hash strating with 1 zero bit
Hash found: 3f00a50341e892fd2640814970db7d86d6203a4dbfe1a49d9dc184c32808ad5163fabeae29c4e1eb7587a0827dfc05867f51d4bc00d2483864817ca94ae8faaa
String is: ed4e80173c79ef972609181dee7635661bcef7569f0a05eb58805503d35a78f6d430e9c2011be36fe0a2c80969a77bebe03a2b680d7aa2ec460aa256e5373129

Trying to find a hash strating with 2 zero bits
Hash found: 0e20f628dc8ef8d072a5319768079da88cac1c21a857a3bba831ff92081562f9d9f2c0450e280ed4fff3d8fa41617d0a95e52dd11defbe59e31abd2e3790e711
String is: a1ef7180c9b5eeed55bfc581af26de070cc281ad3c0a48663b18257063f24c2e6d010c84ece315db12f50833b58f2ed09de03c9cb605d5514cff64ca4c167d74

Trying to find a hash strating with 3 zero bits
Hash found: 06b9a6768f95dd72141a47cc59832d1107f5b2e76c059334a0d0d47b1252f5135e6788c4d5c37a4a2e0f3e0ed600c4da037f60b6ed1a908d9c1b9e0ccb28f0b8
String is: 6f7312ea8606ab07d385b3096c8010b344cf496f0fc6f48a383a503e0cc88f78400e5020b382f7f94cfe611953a75769682d742f2f8b0130ad11d81b9bbb3dc7

```

*(The exact output will vary each time since random input is used.)*

---

## 📁 Project Structure
```
sha512-proof-of-work/
├── .gitignore          # Ignore build files and IDE-specific metadata
├── CMakeLists.txt      # CMake build configuration
├── main.cpp            # Main program source code
└── README.md           # Project documentation
```

---

## 🧠 How It Works
1. Generates a random byte sequence with OpenSSL’s `RAND_bytes`
2. Computes its SHA-512 hash using the EVP API
3. Compares the hash against a bitmask of leading zero bits
4. Repeats until a matching hash is found
5. Prints the random input and its valid hash
