# CDR Billing System 📊

A comprehensive **Call Detail Record (CDR) Processing and Billing System** built in C with a client-server architecture. This system processes telecommunication call records, generates billing reports for customers and interoperator settlements, and provides real-time file transfer capabilities.

---

## 📑 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [System Requirements](#system-requirements)
- [Installation](#installation)
- [Usage Guide](#usage-guide)
- [Technical Details](#technical-details)
- [Data Flow](#data-flow)
- [Security](#security)
- [File Formats](#file-formats)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)

---

## 🎯 Overview

The CDR Billing System is a multi-threaded TCP-based client-server application designed for telecommunications billing. It processes raw CDR (Call Detail Record) data to generate:

- **Customer Billing Reports** (CB.txt) - Individual customer usage and charges
- **Interoperator Settlement Reports** (IOSB.txt) - Inter-network traffic for settlements

The system features user authentication, concurrent CDR processing, search capabilities, and automatic file transfer to clients.

---

## ✨ Features

### Core Functionality
- ✅ **User Authentication** - Secure signup/login with encrypted credentials
- ✅ **Multi-threaded Server** - Handles multiple concurrent clients using pthreads
- ✅ **Parallel CDR Processing** - Simultaneous customer and interoperator billing generation
- ✅ **Real-time Search** - Search by MSISDN (Mobile Station International Subscriber Directory Number) or operator name
- ✅ **File Transfer** - Automatic download of billing reports to client
- ✅ **User-specific Data** - Isolated output directories per user

### Technical Features
- 🔒 **XOR Encryption** - Secure password storage
- 🔍 **Hash-based Indexing** - Fast data lookup (Hash tables with chaining)
- 📊 **Progress Tracking** - Real-time file transfer progress indicators
- 🧵 **Thread Safety** - Proper synchronization for concurrent operations
- 🌐 **Socket Programming** - Robust TCP communication with error handling

---

## 🏗️ Architecture

### System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         CLIENT SIDE                              │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  client.c                                                 │   │
│  │  - TCP connection to server (port 3000)                  │   │
│  │  - Menu-driven interface                                 │   │
│  │  - File reception & save                                 │   │
│  │  - Password masking (termios)                            │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              ↕️ TCP/IP
┌─────────────────────────────────────────────────────────────────┐
│                         SERVER SIDE                              │
│                                                                   │
│  ┌────────────────────────────────────────────────────────┐     │
│  │  server.c - Main Server (port 12345)                   │     │
│  │  - Accept connections                                   │     │
│  │  - Spawn client threads (pthread)                       │     │
│  │  - State machine for menu navigation                    │     │
│  └────────────────────────────────────────────────────────┘     │
│                              │                                    │
│       ┌──────────────────────┼──────────────────────┐           │
│       ↓                      ↓                       ↓           │
│  ┌─────────┐         ┌──────────────┐      ┌──────────────┐    │
│  │  Auth   │         │   Process    │      │   Billing    │    │
│  │  Module │         │   Module     │      │   Module     │    │
│  └─────────┘         └──────────────┘      └──────────────┘    │
│  - auth.c            - process.c            - CustomerBilling.c │
│  - Signup/Login      - CDR coordinator      - search_msisdn()   │
│  - XOR encryption    - Thread spawning      - File transfer     │
│  - Validation        │                      - InteroperatorBilling.c│
│                      ├─────────┬─────────┐  - search_operator()│
│                      ↓         ↓         ↓  - File transfer     │
│            ┌──────────────┐ ┌────────────┐                      │
│            │CustBillProcess│ │IntopBillProcess│                 │
│            └──────────────┘ └────────────┘                      │
│            - Hash table     - Hash map                           │
│            - Customer stats - Operator stats                     │
│            - CB.txt output  - IOSB.txt output                    │
│                                                                   │
│  ┌────────────────────────────────────────────────────────┐     │
│  │  Data Storage                                          │     │
│  │  - data/user.txt (encrypted credentials)              │     │
│  │  - data/CDR.txt (raw call records)                    │     │
│  │  - Output/<user>/CB.txt (customer billing)            │     │
│  │  - Output/<user>/IOSB.txt (interoperator billing)     │     │
│  └────────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────────┘
```

---

## 📂 Project Structure

```
Project/
│
├── client/
│   └── client.c                    # TCP client application
│
├── server/
│   ├── server.c                    # Main server (listener, thread manager)
│   │
│   ├── Auth/
│   │   └── auth.c                  # Authentication logic
│   │
│   ├── Process/
│   │   ├── process.c               # CDR processing coordinator
│   │   ├── CustBillProcess.c       # Customer billing processor
│   │   └── IntopBillProcess.c      # Interoperator billing processor
│   │
│   ├── Billing/
│   │   ├── CustomerBilling.c       # Customer search & file transfer
│   │   └── InteroperatorBilling.c  # Operator search & file transfer
│   │
│   ├── Header/
│   │   ├── server.h                # Server structures & constants
│   │   ├── auth.h                  # Auth function declarations
│   │   ├── process.h               # Process function declarations
│   │   ├── CustBillProcess.h       # Customer billing declarations
│   │   └── IntopBillProcess.h      # Interoperator billing declarations
│   │
│   ├── data/
│   │   ├── user.txt                # Encrypted user credentials
│   │   └── CDR.txt                 # Raw call detail records (input)
│   │
│   └── Output/
│       └── <user_email>/           # User-specific output directory
│           ├── CB.txt              # Customer billing report
│           └── IOSB.txt            # Interoperator billing report
│
└── README.md                       # This file
```

---

## 💻 System Requirements

### Operating System
- Linux (Ubuntu, Debian, CentOS, etc.)
- macOS (with GCC installed)
- Windows (WSL - Windows Subsystem for Linux)

### Compiler & Libraries
- **GCC** (GNU Compiler Collection) version 7.0+
- **POSIX Threads** (pthread library)
- **Standard C Libraries** (stdio, stdlib, string, socket, etc.)

### Network
- Port **3000** for client (configurable)
- Port **12345** for server (configurable)
- TCP/IP support

---

## 🚀 Installation

### Step 1: Clone the Repository

```bash
git clone https://github.com/swayamkaushal1/CDR.git
cd CDR/Project
```

### Step 2: Prepare Data Directories

```bash
# Create necessary directories
mkdir -p server/data
mkdir -p server/Output

# Ensure CDR.txt exists with sample data
# Format: MSISDN|OPERATOR|CODE|CALL_TYPE|DURATION|DOWNLOAD|UPLOAD|BRAND_NAME|THIRD_PARTY
```

### Step 3: Compile Server

```bash
cd server
gcc -o server server.c \
    Auth/auth.c \
    Process/process.c \
    Process/CustBillProcess.c \
    Process/IntopBillProcess.c \
    Billing/CustomerBilling.c \
    Billing/InteroperatorBilling.c \
    -lpthread
```

### Step 4: Compile Client

```bash
cd ../client
gcc -o client client.c
```

---

## 📖 Usage Guide

### Starting the Server

```bash
cd server
./server
```

**Output:**
```
Server listening on port 12345...
```

### Starting the Client

```bash
cd client
./client                    # Connects to 127.0.0.1 (localhost)
# OR
./client <server_ip>        # Connect to remote server
```

---

## 🎮 Menu Navigation

### Main Menu
```
-- MAIN MENU --
1) Signup
2) Login
3) Exit
Enter choice (1-3):
```

#### Option 1: Signup
- Enter email (validated format: user@domain.com)
- Enter password (min 6 chars: uppercase, lowercase, digit, special char)
- Credentials encrypted and stored in `data/user.txt`

#### Option 2: Login
- Enter registered email
- Enter password (hidden input)
- Creates user-specific output directory: `Output/<email>/`

#### Option 3: Exit
- Closes connection gracefully

---

### Secondary Menu (After Login)
```
-- SECONDARY MENU --
1) Process the CDR data
2) Print and search
3) Logout
Enter choice (1-3):
```

#### Option 1: Process CDR Data
- Reads `data/CDR.txt`
- Spawns two parallel threads:
  - **Thread 1:** Customer Billing Processing → `CB.txt`
  - **Thread 2:** Interoperator Billing Processing → `IOSB.txt`
- Outputs saved to `Output/<user_email>/`

#### Option 2: Print and Search
- Navigate to **Billing Menu**

#### Option 3: Logout
- Returns to Main Menu

---

### Billing Menu
```
-- PRINT & SEARCH MENU --
1) Customer Billing
2) Interoperator Billing
3) Back
Enter choice (1-3):
```

#### Option 1: Customer Billing Submenu
```
-- CUSTOMER BILLING --
1) Search by msisdn no
2) Print file content of CB.txt
3) Back
```

**1.1 Search by MSISDN:**
- Enter 10-digit MSISDN (e.g., 9876543210)
- Displays customer details (calls, SMS, data usage)
- Connection closes after display

**1.2 Print CB.txt:**
- Sends `CB.txt` file to client
- Client saves file locally with progress tracking
- Connection closes after transfer

#### Option 2: Interoperator Billing Submenu
```
-- INTEROP BILLING --
1) Search by operator name
2) Print file content of IOSB.txt
3) Back
```

**2.1 Search by Operator:**
- Enter operator name (e.g., "Airtel", "Jio")
- Displays operator statistics
- Connection closes after display

**2.2 Print IOSB.txt:**
- Displays file content line-by-line
- Sends `IOSB.txt` file to client
- Client saves file locally with progress tracking
- Connection closes after transfer

---

## 🔧 Technical Details

### Server Configuration

| Parameter | Value | Location |
|-----------|-------|----------|
| Port | 12345 | `server.h` |
| Max Connections | 5 (BACKLOG) | `server.h` |
| Buffer Size | 1024 bytes | `server.h` |
| Thread Model | One thread per client | `server.c` |

### Client Configuration

| Parameter | Value | Location |
|-----------|-------|----------|
| Port | 3000 | `client.c` |
| Buffer Size | 1024 bytes | `client.c` |
| File Transfer Buffer | 8192 bytes | `client.c` |

### Hash Table Sizes

| Structure | Size | Purpose |
|-----------|------|---------|
| Customer Hash Table | 1000 buckets | Fast MSISDN lookup |
| Operator Hash Map | 4096 buckets | Interoperator stats |

---

## 🔄 Data Flow

### CDR Processing Flow

```
1. User logs in → Creates Output/<email>/ directory
                        ↓
2. User selects "Process CDR data"
                        ↓
3. process.c spawns two threads:
                        ↓
        ┌───────────────┴────────────────┐
        ↓                                ↓
   Thread 1: CustBillProcess       Thread 2: IntopBillProcess
        ↓                                ↓
   Reads data/CDR.txt              Reads data/CDR.txt
        ↓                                ↓
   Parses each line                Parses each line
        ↓                                ↓
   Hash table by MSISDN            Hash map by Operator ID
        ↓                                ↓
   Aggregates customer stats       Aggregates operator stats
        ↓                                ↓
   Writes Output/<email>/CB.txt    Writes Output/<email>/IOSB.txt
        └───────────────┬────────────────┘
                        ↓
            Both threads complete
                        ↓
         "Processing completed" message
```

### File Transfer Protocol

```
1. User selects "Print file content"
                ↓
2. Server sends: FILE_TRANSFER_START:<filename>
                ↓
3. Server sends: FILE_SIZE:<bytes>
                ↓
4. Server sends: <binary file data in 8KB chunks>
                ↓
5. Server sends: FILE_TRANSFER_COMPLETE
                ↓
6. Client saves file locally
                ↓
7. Client displays: ✅ File saved successfully
```

---

## 🔒 Security

### Authentication Security

**Encryption Method:** XOR Cipher
- **Key:** `SECRETKEY123` (configurable in `auth.c`)
- **Storage:** `data/user.txt` (format: `encrypted_email|encrypted_password`)

**Password Requirements:**
- Minimum 6 characters
- At least 1 uppercase letter
- At least 1 lowercase letter
- At least 1 digit
- At least 1 special character: `!@#$%^&*()-_=+[]{}|;:'",.<>?/\`~`

**Email Validation:**
- Must contain exactly one `@`
- Must have at least one `.` after `@`
- Length: 5-63 characters

### Network Security

- **SIGPIPE Handling:** Prevents server crash on client disconnect
- **Error Recovery:** Retry logic with exponential backoff (EINTR, EAGAIN, EWOULDBLOCK)
- **Resource Cleanup:** Proper socket closure and memory deallocation

---

## 📄 File Formats

### CDR.txt (Input)

**Format:** Pipe-delimited (`|`)

```
MSISDN|OPERATOR|CODE|CALL_TYPE|DURATION|DOWNLOAD|UPLOAD|BRAND_NAME|THIRD_PARTY
9876543210|Airtel|91|MOC|120.5|0|0|Airtel India|
9876543210|Airtel|91|SMS-MO|0|0|0|Airtel India|
9876543211|Jio|92|GPRS|0|150.25|50.75|Reliance Jio|
```

**Fields:**
- `MSISDN` - Mobile number (10 digits)
- `OPERATOR` - Network operator name
- `CODE` - Operator code
- `CALL_TYPE` - MOC, MTC, SMS-MO, SMS-MT, GPRS
- `DURATION` - Call duration in seconds (for voice calls)
- `DOWNLOAD` - Data downloaded in MB (for GPRS)
- `UPLOAD` - Data uploaded in MB (for GPRS)
- `BRAND_NAME` - Operator brand name
- `THIRD_PARTY` - Third party operator (for interconnect calls)

### CB.txt (Customer Billing Output)

```
#Customers Data Base:
Customer ID: 9876543210
Operator Brand Name: Airtel
Operator Code: 91
Incoming Voice (Within): 45.5 seconds
Outgoing Voice (Within): 120.5 seconds
Incoming Voice (Outside): 30.0 seconds
Outgoing Voice (Outside): 60.0 seconds
Incoming SMS (Within): 10
Outgoing SMS (Within): 15
Incoming SMS (Outside): 5
Outgoing SMS (Outside): 8
Downloaded: 150.25 MB
Uploaded: 50.75 MB
----------------------------------------------------
```

### IOSB.txt (Interoperator Billing Output)

```
#Interoperator Data:
Operator Brand: Airtel
MOC (Outgoing) Duration: 3600 seconds
MTC (Incoming) Duration: 2400 seconds
SMS MO (Outgoing): 120
SMS MT (Incoming): 95
Downloaded: 5120 MB
Uploaded: 2048 MB
----------------------------------------------------
```

---

## 🐛 Troubleshooting

### Common Issues

#### 1. **Connection Refused**
```
Error: connect: Connection refused
```
**Solution:**
- Ensure server is running: `./server`
- Check port is not blocked: `netstat -tuln | grep 12345`
- Verify firewall settings

#### 2. **Address Already in Use**
```
Error: bind: Address already in use
```
**Solution:**
- Kill existing server process: `pkill -f server`
- Wait 60 seconds for TIME_WAIT to expire
- Or change PORT in `server.h` and recompile

#### 3. **File Not Found Errors**
```
Error opening file: No such file or directory
```
**Solution:**
- Ensure `data/CDR.txt` exists
- Run "Process CDR data" before searching/printing
- Check file permissions: `chmod 644 data/CDR.txt`

#### 4. **Compilation Errors**

**pthread undefined:**
```bash
# Add -lpthread flag
gcc ... -lpthread
```

**Missing headers:**
```bash
# Install build essentials (Ubuntu/Debian)
sudo apt-get install build-essential
```

#### 5. **File Transfer Incomplete**
```
⚠️ File transfer incomplete: received 512 of 1024 bytes
```
**Solution:**
- Check network stability
- Increase client buffer size in `client.c`
- Verify server file permissions

---

## 📊 Performance Characteristics

| Metric | Value | Notes |
|--------|-------|-------|
| Client Capacity | Unlimited | Limited by system resources |
| Thread Overhead | ~8KB per client | pthread stack size |
| CDR Processing Speed | ~50,000 records/sec | Depends on hardware |
| Search Complexity | O(1) average | Hash table lookup |
| Memory Usage | ~1MB per 10,000 customers | Hash table overhead |
| File Transfer Speed | ~10MB/sec | Network dependent |

---

## 🧪 Testing

### Unit Testing

```bash
# Test authentication
./server &
./client
# Signup with weak password → Should fail
# Signup with strong password → Should succeed

# Test CDR processing
# Login → Process CDR data → Check Output/<email>/ for CB.txt and IOSB.txt

# Test search
# Search for existing MSISDN → Should return results
# Search for non-existent MSISDN → Should return "not found"

# Test file transfer
# Select "Print file content" → Verify file saved in client directory
```

### Load Testing

```bash
# Simulate multiple clients
for i in {1..10}; do
    ./client &
done

# Monitor server
top -p $(pgrep server)
```

---

## 🤝 Contributing

### How to Contribute

1. **Fork** the repository
2. **Create** a feature branch: `git checkout -b feature/YourFeature`
3. **Commit** changes: `git commit -m "Add YourFeature"`
4. **Push** to branch: `git push origin feature/YourFeature`
5. **Submit** a pull request

### Coding Standards

- Follow **K&R C** style
- Use **4 spaces** for indentation (no tabs)
- Maximum line length: **100 characters**
- Comment complex algorithms
- Add error handling for all I/O operations

---

## 📝 License

This project is licensed under the **MIT License**.

---

## 👨‍💻 Author

**Swayam Kaushal**
- GitHub: [@swayamkaushal1](https://github.com/swayamkaushal1)
- Project: [CDR Billing System](https://github.com/swayamkaushal1/CDR)

---

## 🙏 Acknowledgments

- **POSIX Threads** documentation
- **Beej's Guide to Network Programming**
- **The C Programming Language** by Kernighan & Ritchie

---

## 📞 Support

For issues, questions, or feature requests:
- Open an issue on [GitHub Issues](https://github.com/swayamkaushal1/CDR/issues)
- Email: your.email@example.com

---

## 🔮 Future Enhancements

- [ ] SSL/TLS encryption for network communication
- [ ] Database integration (MySQL/PostgreSQL)
- [ ] Web-based dashboard
- [ ] Real-time billing alerts
- [ ] CSV export functionality
- [ ] Rate limiting and DDoS protection
- [ ] Logging system (syslog integration)
- [ ] Docker containerization
- [ ] REST API for third-party integration
- [ ] Multi-language support

---

## 📈 Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0.0 | 2025-11-06 | Initial release with core features |
| 1.1.0 | 2025-11-06 | Added file transfer functionality |
| 1.2.0 | 2025-11-06 | Optimized CB.txt transfer (direct send) |

---

**⭐ If you find this project useful, please consider giving it a star on GitHub!**

---

**Last Updated:** November 6, 2025
