# SSR to Mihomo Config Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a C++ command-line tool that downloads an SSR subscription, decodes `ssr://` nodes, and writes a mihomo-compatible `config.yaml`.

**Architecture:** Split the program into fetch, decode, parse, and generate stages. Keep SSR parsing isolated behind focused functions so future protocol parsers can be added without rewriting the CLI or configuration generator.

**Tech Stack:** C++17, CMake, libcurl, lightweight built-in YAML emitter, CTest

---

### Task 1: Project Skeleton And Red Tests

**Files:**
- Create: `CMakeLists.txt`
- Create: `include/ssrdecode/types.h`
- Create: `include/ssrdecode/encoding.h`
- Create: `include/ssrdecode/subscription.h`
- Create: `include/ssrdecode/ssr.h`
- Create: `include/ssrdecode/mihomo.h`
- Create: `include/ssrdecode/fetcher.h`
- Create: `tests/test_ssrdecode.cpp`

- [ ] **Step 1: Write the failing tests**
- [ ] **Step 2: Run the test target and confirm the project fails before implementation exists**

### Task 2: Implement Decode And Parse Flow

**Files:**
- Create: `src/encoding.cpp`
- Create: `src/subscription.cpp`
- Create: `src/ssr.cpp`

- [ ] **Step 1: Implement loose Base64/Base64URL decode, URL decode, and line splitting**
- [ ] **Step 2: Implement SSR URI parsing and node naming**
- [ ] **Step 3: Run tests and confirm parsing passes**

### Task 3: Implement Mihomo Output And CLI

**Files:**
- Create: `src/mihomo.cpp`
- Create: `src/fetcher.cpp`
- Create: `src/main.cpp`

- [ ] **Step 1: Implement a safe YAML emitter for mihomo config generation**
- [ ] **Step 2: Implement libcurl fetch and command-line entry point**
- [ ] **Step 3: Build the binary and run tests again**
