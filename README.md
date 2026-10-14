# Easy-Read Treasury of Scripture Knowledge & KJV Bible CLI Tools

This repository contains a suite of Command Line Interface (CLI) applications designed to output raw Bible text in multiple highly accessible formats.

Each application operates as a standalone tool with the complete King James Version (KJV) Bible and Treasury of Scripture Knowledge (TSK) datasets fully embedded. Bundling the data directly within the binaries completely eliminates external dependencies, making the tools lightweight, portable, and incredibly fast. Every utility is engineered to deliver direct, instantaneous verse access.

## 📜 Motivation

It is of utmost importance for ministry leaders and teachers to present the Word of God with absolute accuracy and clarity. Scripture cannot contradict itself; when we encounter apparent ambiguity, it is almost always a result of incomplete study or incorrect human interpretation.

The **Treasury of Scripture Knowledge (TSK)** is an invaluable asset for cross-referencing verses within their true scriptural contexts, allowing the Bible to interpret itself through the harmony of similar references. 

However, manually paging through thousands of cross-references in a physical book or complex software can be incredibly onerous. These CLI tools solve this by granting students instantaneous access to relevant references within the exact same context—allowing you to deep-dive into study without losing your train of thought. The cross-referenced verses are displayed immediately on-screen in their correct sequential order.

Furthermore, integrating these raw outputs with modern Large Language Models (LLMs) can greatly assist students in analyzing original grammatical structures, exploring syntax, and unpacking difficult phrases for any given verse.

Use the Word of God responsibly, grounded in truth, and standing on the shoulders of the faithful Bible scholars who have gone before us over many ages.

Read more here: [delicium.co.za](https://delicium.co.za/blog/)

## Available CLI Tools

### Core Bible & Analysis Tools
* **`tskjv`**: Prints the requested verse from the KJV Bible followed immediately by all corresponding cross-references from the TSK.
* **`tskjvg`**: An LLM-powered grammar assistant for KJV text. Uses a local LLM to break down the grammar and phrasing of a verse.
  * *Note: Requires an active Ollama server with at least one LLM installed (preferably `qwen3:0.6b`).*

### Web & Capsule Server Extensions (CGI / FastCGI)
* **`serve_tskjv_html.cgi` & `serve_tskjv_html.fcgi`**: Web server applications that generate dynamic HTML. They can be deployed behind any CGI/FastCGI-enabled HTTP server.
* **`serve_tskjv_gemtext.cgi`**: A CGI server application (capsule) tailored for Gemini protocol servers.

### Emacs Org-Mode Integration
* **`c-org-expand`**: Expands an Org-mode table by fetching and inserting the full Bible verses for every reference found.
* **`c-org-collapse`**: Performs the inverse operation—removes the expanded Bible verses from the table, leaving only the clean reference structure intact.

---

## Usage

### `tskjv`
```text
Usage: tskjv --command=value

Commands & Descriptions:
  --v     Fetch verses. Examples:
            "joh 1:1"
            "joh 1:1,12"
            "joh 1:1-12"
            "joh 1:1;joh 1:2-11;joh 1:12"
  --r     Fetch verse along with TSK references. Examples:
            "joh 1:1"
            "joh 1:1,12"
            "joh 1:1-12"
  --b     List all Bible books. Options:
            true or 1
```

### `tskjvg`
```text
Usage: tskjvg --command=value

Commands & Descriptions:
  --v             Target verse. Example: "joh 1:1"
  --model         Override the default LLM model defined in config.txt
  --temperature   Override the temperature setting defined in config.txt
```

#### Example Output:
```text
Grammar for:
joh 1:1 In the beginning was the Word, and the Word was with God, and the Word was God.
Processing... ⠏
  Model: qwen3:0.6b
  role:  assistant
  content start:
------------------------------------
Here's a proper breakdown of the phrases and grammar structure of the KJV verse:

### 1. Structure
The verse is a compound sentence with three clauses connected by conjunctions ("and"). Each clause is a noun phrase:

- "In the beginning was the Word"
  - Subject: "the Word" (possessive pronoun)
  - Verb: "was" (past tense)
  - Preposition: "in the beginning"

- "and the Word was with God"
  - Subject: "the Word" (possessive pronoun)
  - Verb: "was" (past tense)
  - Preposition: "with God"

- "and the Word was God"
  - Subject: "the Word" (possessive pronoun)
  - Verb: "was" (past tense)
  - Preposition: "with God"

### 2. Grammar and Conjunctions
- Conjunctions:
  - "and" links the three clauses (used to connect the noun phrases).
  - "and" again links the second and third clauses.

### 3. Key Elements
- Subject-Verb Agreement: All subjects ("the Word") and verbs ("was") are in the past tense.
- Possessive Pronouns: "the Word" is used as a possessive pronoun in all clauses.

### 4. Final Notes
The structure is clear and follows standard English grammar rules, emphasizing the Word as the central subject throughout the verse.
------------------------------------
  content end
  Duration: 16 sec
Processing... Done!
```

### `serve_tskjv_html.cgi` & `serve_tskjv_html.fcgi`
Deploy using Nginx, Caddy, or Apache by following your server's routing instructions for CGI/FastCGI applications (typically handled via `fcgiwrap`).

* Live HTML Demo: [Easy-Read TSK](https://delicium.co.za/ertsk)

### `serve_tskjv_gemtext.cgi`
Deploy using a Gemini server that supports CGI scripts (such as `gmnisrv`) by adding it to the server's configuration file.

* Live Gemini Demo: gemini://delicium.co.za
*(Accessible via Gemini clients like Lagrange, Amfora, or gmni)*

### `c-org-expand`
**Use Case:** Automatically expand a Bullinger structural outline defined in an Org-mode file by injecting the physical Bible text.

```bash
cat ./test/test.org | c-org-expand
# OR
c-org-expand ./test/test.org
```
* Generated Page Example: [Easy Bullinger Structure for Ps 104](https://delicium.co.za/erbs/ps/ps_104.html)

### `c-org-collapse`
**Use Case:** Reverse the expansion process to remove the Bible text blocks and compress the document back down to its skeletal Bullinger structure.

```bash
cat ./test/test_collapse.org | c-org-collapse
# OR
c-org-collapse ./test/test_collapse.org
```

---

## Build Instructions

### Requirements
* A **Clang** or **GNU C** compiler with support for the **C23** standard.
* **GNU Make**.
* **`libcurl`** (Required only for `tskjvg` to communicate with the LLM API).
* An active **Ollama** server instance (To run the grammar tool testing suite).

### Building the Project
Run one of the following commands from the root directory:

```bash
# Build for production (Optimized)
make MODE=release

# Build for testing with AddressSanitizer (ASan)
make MODE=asan

# Build with debug symbols enabled (Default)
make MODE=debug
```

---

## ⚖️ License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details. 

*If you use this software or bundle it into your own projects, please retain the copyright notice and include a link back to this GitHub repository.*
