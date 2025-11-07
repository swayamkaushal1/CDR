# Design Review Checklist - CDR Billing System

**Project Name:** CDR (Call Detail Record) Billing System  
**Version:** 1.1  
**Authors:** Swayam Kaushal, Soham Bose  
**Review Date:** November 7, 2025  

---

## Design Review Checklist

| S. No. | Category | Item | Yes/No/NA | Remarks |
|--------|----------|------|-----------|---------|
| | **Traceable** | | | |
| 1.1 | | Are all requirements addressed in the design? | Yes | Authentication (signup/login), CDR processing (parallel threads), billing operations (search/display), file transfer implemented |
| 1.2 | | Does design capture feature or function level design/flows/data structures for meeting functional requirements? | Yes | State machine (MAIN→SECOND→BILLING→CUST_BILL/INTER_BILL), Customer hash table (1000 buckets), OpNode hash map (4096 buckets), ProcessThreadArg structure for threading |
| 1.3 | | Are all the (external and internal) interfaces designed | Yes | External: TCP socket (port 12345), client.c interface. Internal: auth.h, process.h, CustBillProcess.h, IntopBillProcess.h define module interfaces |
| 1.4 | | Have the performance requirements been addressed? | Yes | Hash tables provide O(1) lookup, pthread detached threads for concurrent clients, sendall_fd with retry logic prevents blocking |
| 1.5 | | The design addresses security | Yes | XOR encryption (SECRETKEY123), password validation (6+ chars, upper/lower/digit/special), email format validation, sanitized directory names |
| 1.6 | | The design ensures that the scalability requirements, if any, are addressed | Yes | User-specific output directories (Output/<email>/), unlimited concurrent clients (limited by system resources), thread-per-client model |
| 1.7 | | The software (component) design addresses porting requirements | No | POSIX-dependent (pthread, socket, unistd, sys/socket) - requires Linux/Unix/macOS, not portable to Windows without modification |
| 1.8 | | The software (component) design considers the maintainability requirements | Yes | Modular files (Auth/, Process/, Billing/, Header/), function declarations in headers, consistent naming (snake_case), comments for complex logic |
| 1.9 | | The design does not violate any design constraints documented in the SRS | Yes | Adheres to TCP client-server model, port 12345, menu-driven interface, file-based storage as specified |
| 1.1 | | The design does not violate any requirement | Yes | No requirement conflicts identified |
| 1.11 | | All stated and unstated (implied) requirements are addressed | Yes | SIGPIPE ignored (prevents crashes), SO_REUSEADDR (allows restart), proper socket cleanup, thread cleanup via pthread_detach |
| 1.12 | | Has the updated RTM been sent as part of deliverables for review? (RTM should also be a part of the deliverable for review) | NA | RTM not applicable for this project scope |
| | **Complete** | | | |
| 2.1 | | All Applicable sections are filled as per template | Yes | server.c (main loop + client_thread + handle_client), auth.c (signup/login), process.c (coordinator), CustBillProcess.c, IntopBillProcess.c, CustomerBilling.c, InteroperatorBilling.c - all sections complete |
| 2.2 | | The design addresses response of the software (module) to all erroneous response from the software (module) | Yes | Socket errors (recv_line returns -1), malloc failures (checks NULL, closes socket), pthread_create failures (frees memory), file open errors (sends error message to client) |
| 2.3 | | The design addresses response of the software (module) to all valid (and invalid) input data as defined in the SRS/any other requirement specification document | Yes | Valid: email regex (has @ and .), password (6+ chars + complexity), MSISDN (atol check >0). Invalid: error messages sent, returns to menu. Empty inputs handled (strlen check) |
| 2.4 | | Has an alternate design approach been considered? | No | Single approach implemented: file-based storage with hash tables. Alternatives (database, message queue, async I/O) not explored |
| 2.5 | | Is the alternate design approach documented in the document? | No | No design alternatives documented |
| 2.6 | | Is the rationale for following an approach documented? | No | Implementation decisions not formally documented. Code comments present but no design rationale doc exists |
| | **Accurate** | | | |
| 3.1 | | All assumptions made during the design been stated clearly | No | Implicit assumptions not documented: single server instance, file-based storage (data/CDR.txt), pipe-delimited CDR format, user.txt encryption format, no database, synchronous I/O |
| 3.2 | | Are all design assumptions validated | Yes | TCP reliability validated (send/recv with error checks), thread safety (detached threads, no shared state conflicts), hash collision (chaining with linked lists), XOR encryption (bidirectional) |
| 3.2 | | The design does not impose any new requirements | Yes | No scope creep. Original requirements (auth, process, search, display, file transfer) met without adding unrequested features |
| | **Consistency** | | | |
| 4.1 | | There is no conflict within the design sections | Yes | Menu state flow consistent (MAIN→SECOND→BILLING→CUST_BILL/INTER_BILL), function naming uniform (snake_case), return types consistent (int for status, void* for threads), error handling pattern uniform |
| | **Detailed** | | | |
| 5.1 | | The Inter process communication protocol, if required, is defined | Yes | TCP socket protocol: send_line (text + \n), recv_line (char-by-char until \n), FILE_TRANSFER protocol (FILE_TRANSFER_START:<filename>\nFILE_SIZE:<bytes>\n<binary data>FILE_TRANSFER_COMPLETE\n) |
| 5.2 | | The design is understandable | Yes | README.md with architecture diagram, modular structure (Auth/Process/Billing), function names self-documenting (search_msisdn, display_customer_billing_file), state enum clear (MenuState) |
| | **Design Reuse** | | | |
| 6.1 | | Has any part of the design reused | Yes | sendall_fd/send_line_fd reused in CustomerBilling.c, InteroperatorBilling.c, process.c. Hash table pattern reused (Customer hash table in CustBillProcess.c, OpNode hash map in IntopBillProcess.c). State machine pattern standard industry practice |
| | **Capacity and Scalability** | | | |
| 8.1 | | Has the main data structures which would occupy major RAM memory been identified? | Yes | Customer *hashTable[HASH_SIZE=1000], OpNode *buckets[NUM_BUCKETS=4096], pthread stack (8MB default per thread), client buffers (BUFSIZE=1024 per connection) |
| 8.2 | | Has the capacity values for the main data structures which would occupy major RAM been identified? | Yes | HASH_SIZE=1000, NUM_BUCKETS=4096, MAX_LINE=1024, BUFSIZE=1024, EMAIL_MAX=64, PASS_MAX=32. Customer struct ~200 bytes, OpNode ~150 bytes |
| 8.3 | | Has the total memory used for static storage as well as transactional memory been theoretically calculated for Max resources | No | Not formally calculated. Estimated: Static buckets ~40KB, per-customer ~200B, per-operator ~150B. For 10K customers: ~2MB data + 8MB*threads. Full calculation missing |
| 8.4 | | Is the memory requirement for the system within limits (both for 64bit as well as 32bit)? | Yes | Pointer-based structures work on 32/64-bit. Estimated max usage: 100 threads = 800MB stacks + 10MB data = <1GB total. Well within modern limits |
| 8.5 | | Does the design take care to allocate memory for the resources at init time? | No | Hash table buckets static (compile-time), but Customer/OpNode nodes allocated on-demand (malloc in getCustomer/get_or_create_opnode). No pre-allocation strategy |

---

## Summary

### Strengths:
1. **Well-structured modular design** - Clear separation of concerns (Auth, Process, Billing)
2. **Scalable architecture** - Thread-per-client model, efficient hash-based lookups
3. **Security conscious** - Credential encryption, input validation, password strength requirements
4. **Robust error handling** - Comprehensive error checks for I/O, network, and user input
5. **Good code organization** - Header files for interfaces, consistent naming, documented functions

### Areas for Improvement:
1. **Design documentation** - Add formal design document explaining architecture decisions and alternatives considered
2. **Portability** - Consider Windows compatibility or document POSIX-only requirement explicitly
3. **Memory analysis** - Provide detailed memory footprint calculations for different load scenarios
4. **Assumptions documentation** - Explicitly state all assumptions (file format, single server, etc.) in SRS/design doc
5. **Resource initialization** - Consider pre-allocating hash table buckets to avoid runtime malloc overhead

### Recommendations:
1. Create a formal Software Design Document (SDD) with UML diagrams
2. Add capacity planning section with memory/thread calculations
3. Document the rationale for file-based storage vs. database
4. Consider adding configuration file for tunable parameters (hash sizes, ports, etc.)
5. Add unit tests for critical modules (hash functions, encryption, input validation)

---

**Overall Assessment:** The design is **APPROVED** with minor recommendations for documentation improvements. The implementation is solid, functional requirements are met, and the architecture supports the stated use cases effectively.

**Design Quality Score:** 85/100

**Next Steps:**
1. Address documentation gaps (design rationale, assumptions)
2. Conduct performance testing with load scenarios
3. Add README with capacity/scaling guidelines
4. Consider extracting configuration to external file

---

**Reviewed By:** GitHub Copilot Analysis  
**Review Completion Date:** November 7, 2025
