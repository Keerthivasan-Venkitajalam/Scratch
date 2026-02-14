# **MONTH 1 — MARKET DATA INFRASTRUCTURE**

**Mission:** Build a zero-copy C++ FeedHandler that ingests raw bytes from a socket and converts them into structured Tick objects with no heap allocation in the hot path.

**Primary Skills Installed This Month**

* Linux socket I/O
* Non-blocking networking
* TCP stream buffering
* Zero-copy parsing
* Custom atoi/atof
* Finite State Machine parsing
* Benchmarking & testing

---

## **WEEK 1 — THE PLUMBING (NETWORK & ENVIRONMENT)**

### **Day 1 — Project Skeleton & Toolchain**

**TODO**

* Install:

  * clang++ (latest)
  * cmake
  * gdb
  * valgrind
  * AddressSanitizer (via clang)

**Repo Structure**

```
feedhandler/
├── CMakeLists.txt
├── include/
│   ├── net/
│   ├── parser/
│   └── common/
├── src/
│   ├── net/
│   ├── parser/
│   └── main.cpp
├── tests/
├── benchmarks/
```

**CMake**

* C++20
* Debug + Release modes
* ASan optional flag

**Code**

* main.cpp prints “FeedHandler Boot OK”

**Done When**

* `cmake .. && make` builds
* Executable runs

---

### **Day 2 — Blocking TCP Client**

**TODO**

* Write `TcpClient` class:

  * socket()
  * connect()
  * send()
  * recv()

**Test**

* Run `nc -l 8080`
* Client connects and echoes data

**Files**

```
include/net/tcp_client.hpp
src/net/tcp_client.cpp
```

**Done When**

* You can send “hello” and receive echo

---

### **Day 3 — Non-Blocking Mode + select()**

**TODO**

* Set O_NONBLOCK using fcntl
* Implement select()-based event loop
* Print whenever socket becomes readable

**Files**

```
src/net/event_loop.cpp
```

**Done When**

* Program never blocks even if server sends nothing

---

### **Day 4 — Receive Buffer + TCP Stream Handling**

**TODO**

* Implement fixed buffer:

  ```
  alignas(64) char recv_buffer[8192];
  size_t write_pos, read_pos;
  ```
* recv() writes to buffer
* Parser reads from buffer
* Handle message split across reads

**Concept Implemented**

* TCP stream reassembly

**Done When**

* Partial messages across multiple recv() calls reconstruct correctly

---

### **Day 5 — Connect to Real WebSocket Feed**

**TODO**

* Use Boost.Beast OR stunnel
* Connect to Coinbase/Binance public feed
* Print raw JSON strings

**Done When**

* Console shows live market JSON

---

### **Day 6 — Algorithm Sprint**

**TODO**

* LeetCode 344
* LeetCode 151 (no split, in-place)

**Done When**

* Accepted solutions stored in `/algorithms/`

---

### **Day 7 — Protocol Theory + Tick Struct Design**

**TODO**

* Read Beej’s Guide client sections
* Study FIX tag=value format
* Design Tick struct:

```
struct Tick {
   std::string_view symbol;
   int64_t price;   // fixed point
   int32_t qty;
   char side;       // 'B' or 'S'
};
```

**Deliverable**

* `docs/tick_spec.md`

---

### **WEEK 1 DONE WHEN**

✔ Non-blocking TCP client works
✔ Raw byte buffer handles fragmentation
✔ Live feed printing
✔ Tick struct defined

---

## **WEEK 2 — THE PARSER (NAIVE → ZERO COPY)**

### **Day 8 — Naive FIX Parser**

**TODO**

* Using std::string + std::stringstream + getline('|')
* Parse dummy FIX message
* Fill Tick struct

**Benchmark**

* Parse 1M messages
* Measure time

**Done When**

* Benchmark recorded

---

### **Day 9 — string_view Parser**

**TODO**

* Rewrite parser to accept `std::string_view`
* No allocations
* Fields returned as views

**Done When**

* Parsing works with no std::string creation

---

### **Day 10 — fast_atoi**

**TODO**

```
inline int fast_atoi(const char* begin, const char* end)
```

* No exceptions
* Handles sign
* Stops at non-digit

**Tests**

* Unit tests for valid/invalid

---

### **Day 11 — fast_atof (fixed-point)**

**TODO**

* Parse "123.4567" → int64 scaled
* No float/double

**Tests**

* Decimal cases
* No decimal case
* Leading zeros

---

### **Day 12 — Repeating Group Logic**

**TODO**

* Parse multiple repeating tags into multiple Tick objects
* Store output into vector<Tick>

---

### **Day 13 — Algorithm Sprint**

* LeetCode 8 (atoi)
* LeetCode 65 (valid number FSM)

---

### **Day 14 — Reading**

* “Writing a Zero-Copy Parser”
* Lexical analysis basics

---

### **WEEK 2 DONE WHEN**

✔ string_view parser works
✔ fast_atoi/atof tested
✔ 1M message benchmark improved vs naive

---

## **WEEK 3 — FSM PARSER**

### **Day 15 — FSM State Diagram**

States:

```
WAIT_TAG → READ_TAG → WAIT_VALUE → READ_VALUE → DELIM
```

**Deliverable**

* docs/fsm_diagram.md

---

### **Day 16 — FSM Implementation**

**TODO**

* Single while loop
* switch(state)
* Parse char-by-char

**Constraint**

* Can stop mid-message and resume

---

### **Day 17 — Tag Switch Optimization**

**TODO**

* Convert tag chars → int tag_id
* switch(tag_id) for field assignment

---

### **Day 18 — Integrate FSM with Buffer**

**TODO**

* Parser reads from receive buffer
* Saves state if buffer ends mid-field

---

### **Day 19 — Unit Tests**

Using GTest:

1. Full message
2. Fragmented message
3. Corrupt message

---

### **Day 20 — Algorithm Sprint** ✓

* LeetCode 10 - Regular Expression Matching (DP solution)
* Reverse words optimized (true O(1) space in-place)

---

### **Day 21 — Branch Prediction Awareness**

**TODO**

* Mark delimiter branch likely
* Profile basic perf counters

---

### **WEEK 3 DONE WHEN**

✔ FSM parses streaming input
✔ Handles fragmented TCP
✔ GTest passes all cases

---

## **WEEK 4 — OPTIMIZATION & BENCHMARKING**

### **Day 22 — Object Pool**

**TODO**

* Preallocate vector<Tick> pool
* Parser writes into pool slots

---

### **Day 23 — Flyweight Pattern**

**TODO**

* Tick stores only string_views into buffer
* Ensure buffer lifetime valid

---

### **Day 24 — Google Benchmark**

**Benchmarks**

* naive parser
* string_view parser
* FSM parser

**Goal**

* FSM ≥ 5× faster

---

### **Day 25 — Garbage Recovery**

**TODO**

* If parsing fails → scan for "8=FIX"
* Resume parsing

---

### **Day 26 — Thread Split**

**TODO**

* Network thread reads socket
* Parser thread consumes buffer
* std::mutex queue (for now)

---

### **Day 27 — Algorithm Sprint**

* LeetCode 3
* CF string problem

---

### **Day 28 — Final Assembly**

**TODO**

* Mock TCP server sends simulated FIX
* FeedHandler parses
* Prints:

```
Bid: X / Ask: Y
```

---

## **MONTH 1 FINAL DEFINITION OF DONE**

✔ Non-blocking TCP client
✔ Receive ring buffer
✔ Zero-copy FSM parser
✔ Custom atoi / atof
✔ Fragmentation-safe stream parsing
✔ GTest suite passing
✔ Google Benchmark report
✔ Console demo printing live ticks
✔ No heap allocation inside parser hot path

---

## **Performance Target**

* ≥ 1 million messages/sec single core
* 0 allocations during parse (verified by valgrind/ASan)

---

## **Suggested Daily Time Split**

* 90 min coding
* 30 min reading or algorithms

---

If you want, I can next generate:

* **Exact CMakeLists.txt**
* **TcpClient class code**
* **FSM parser skeleton**
* **Google Benchmark template**

Say the word and we proceed Day 1 implementation immediately.
