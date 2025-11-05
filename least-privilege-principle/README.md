# 🧩 Principle of Least Privilege — Network Time Application

This project demonstrates the **Principle of Least Privilege (PoLP)** by splitting a time management application into two isolated processes — a non-privileged main app (`main.cpp`) and a privileged helper (`set_time.cpp`).

---

## ⚙️ System Architecture

**Components:**
- **SH (Network Clock)** — runs as a standard user, displays and formats the current time, listens on a TCP port, and handles all non-privileged user actions.
- **NC (Set Time)** — runs with the `CAP_SYS_TIME` capability and performs only one privileged operation: setting the system time.

**Security goals:**
- Avoid running network code as root.
- Use capability-based privilege separation (`libcap`).
- Drop all unnecessary privileges immediately after startup.
- Protect against input-based vulnerabilities (format string, buffer overflow).

---

## 🧠 Date & Time Format Specification

The application allows users to specify a **custom format** for date and time display using `$`-prefixed markers.

### ✅ Allowed characters

| Type | Description | Example |
|------|--------------|----------|
| Format markers | `$D` – day, `$M` – month, `$Y` – year, `$h` – hour, `$m` – minute, `$s` – second | `$D:$M:$Y` |
| Separators | Space, colon, dash, slash | `- : /` |
| Others | Any printable character not blacklisted | `"Date: $D.$M.$Y"` |

### 🚫 Blacklist

The following characters are forbidden for security reasons:  
`['%', '\']`

These symbols are excluded to prevent possible interpretation by shell or formatting subsystems.

### 🧩 Examples

| Valid | Invalid | Reason |
|--------|----------|--------|
| `$D:$M:$Y` | `%D:%M:%Y` | `%` is blacklisted |
| `Date: $D.$M.$Y, time: $h:$m:$s` | `$D$M$Y` | Missing separators |
| `Rok: $Y` | `$S:$m:` | `$S` is not a valid marker |

### 🔒 Security Rationale

The format parser works as a pure text interpreter — it never evaluates user input as code.  
Only a minimal blacklist is applied to balance **security and flexibility**.

A helper variable `lastWasMarker` ensures that two format markers cannot appear consecutively without a separator (e.g. `$D$M$Y` → rejected).  
This enforces readability and consistency in user-defined formats.

---

## 🏗️ Architecture and Privilege Separation

### 🕒 Application SH

- Runs as a standard user (no elevated privileges).
- Passes time-change requests to `set_time` using `fork()` + `exec()`.
- Converts formatted input into a numeric `time_t` value before passing it to NC.
- Manages local CLI and network threads (TCP listener).

### 🔐 Application NC

- Minimal process that only validates and executes `settimeofday()`.
- All input validation and logic are handled in SH.
- Runs with `CAP_SYS_TIME`, dropped immediately after use.
- No user interaction or networking capabilities.

### 🔧 Multi-threading

- **Local thread:** handles CLI input and privileged requests.
- **Network thread:** processes remote requests safely (read-only).

---

## 🧱 Capability Management

1. **Grant capability**

```bash
sudo setcap cap_sys_time+pe /usr/local/sbin/set_time
```

2. **Drop all unnecessary capabilities at startup**

```cpp
give_up_capabilities(SYS_TIME, 1);
set_effective_cap(SYS_TIME, 1, false);
```

3. **Temporarily enable when needed**

```cpp
set_effective_cap(SYS_TIME, 1, true);
settimeofday(...);
give_up_capabilities(nullptr, 0);
```

---

## 🗂️ Configuration File and ACLs

**Location:** `/etc/network_time.conf`  
Contains one line:

```text
PORT=5555
```

**Access control:**
- Readable by the SH process.
- Not writable by standard users.
- `/usr/local/sbin/set_time` located in a secure, non-user-writeable path.

---

## 🧠 DEP (Data Execution Prevention)

### Hardware support

ARMv8 architecture (AArch64) implements **Execute Never (XN)** memory protection bit.

> “This feature allows memory pages to be marked as non-executable.”  
> — [ARM Documentation](https://developer.arm.com/documentation/den0013/d/The-Memory-Management-Unit/Memory-attributes/Execute-Never)

### Linux kernel support

DEP is **enabled by default** on modern Linux distributions.

> “If the CPU has this feature it is enabled by default, unless overridden by `noexec=off`.”  
> — [RedHat Security Guide](https://access.redhat.com/solutions/2936741)

No additional configuration is required on Kali Linux ARM64.

---

## ⚙️ Installation and Execution

### 1️⃣ Compilation

```bash
sudo apt install libcap-dev
g++ -Wall -pedantic -o main src/main.cpp -lcap
g++ -o set_time src/set_time.cpp -lcap
```

### 2️⃣ Capability Setup

```bash
sudo setcap cap_sys_time+pe /usr/local/sbin/set_time
```

### 3️⃣ Configuration

Edit `/etc/network_time.conf`:

```text
PORT=5555
```

### 4️⃣ Run

```bash
./main
```

---

## 🧩 Example Workflow

| Step | Action | Privilege |
|------|---------|-----------|
| 1 | User starts `main` | Standard user |
| 2 | TCP client requests formatted time | Standard user |
| 3 | Local CLI user enters `set` | Triggers NC |
| 4 | `set_time` runs with `CAP_SYS_TIME` | Elevated only temporarily |
| 5 | Privileges dropped, control returned | User-level process again |

---

## 🧭 Summary

This project showcases **practical application of the Principle of Least Privilege**:
- Secure process isolation (SH / NC)
- Dynamic capability management
- DEP and input sanitization
- Safe configuration and ACL control

It demonstrates that **security and functionality can coexist** without running network code as root.

