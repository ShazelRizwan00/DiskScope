# Academic Integrity and AI Usage Disclosure

## Course Information

- **Course Title:** Data Structures and Algorithms
- **Project Title:** DiskScope: Disk Space Analyzer with Custom Max Binary Heap
- **Submission Date:** `[Insert Date: YYYY-MM-DD]`
- **Student Name(s) and ID(s):**
  1. `[Student Name 1]` — `[Student ID 1]`
  2. `[Student Name 2]` — `[Student ID 2]`
  3. `[Student Name 3]` — `[Student ID 3]`

---

## 1. Scope of Artificial Intelligence Assistance

In accordance with academic integrity guidelines, the development team discloses the use of generative AI assistance tools (such as Claude / ChatGPT) during the conception, implementation, and verification of DiskScope. AI assistance was utilized strictly as an engineering accelerator and peer reviewer across the following phases:

1. **Architecture Draft and Interface Review:**
   - AI tools assisted in drafting initial modular header contracts and proposing clean architectural boundaries between headless core services (`diskscope_core`) and graphical components (`diskscope_ui`).
   - Assisted in peer-reviewing interface definitions, identifying edge cases such as memory safety during tree rebuilds, const correctness of recording observers, and junction loop hazards.
2. **Code Scaffolding and Boilerplate Implementation:**
   - Assisted in generating boilerplate structures for Windows Win32 API interaction (`AllocatedSize.cpp`, `FileAttributes.cpp`).
   - Drafted initial structural scaffolds for ImGui rendering callbacks, theme styling, and event snapshot recording in `HeapEvent.h`.
3. **Test Harness Generation:**
   - Assisted in structuring unit test assertions in `test_scanner.cpp`, `test_heap.cpp`, and `test_analysis.cpp`.
   - Formulated adversarial boundary test vectors (empty heaps, single-element heaps, duplicate size ties, ACL permission failures, and cancellation checkpoints).
4. **Documentation Drafting and Formatting:**
   - Aided in drafting initial Markdown structural outlines for assignment documentation, LaTeX mathematical equations, and complexity tables.
   - Formatted manual dry trace tables and presentation slide layouts.

---

## 2. Independent Student Verification and Authorship

The development team explicitly affirms that:
- Every line of code, data structure invariant, and algorithm implementation was thoroughly read, inspected, and understood by the submitting students.
- The custom max binary heap (`MaxHeap.cpp`, `MaxHeap.h`) was reviewed to guarantee that no standard library heap algorithms (`std::priority_queue`, `std::make_heap`, `std::push_heap`, `std::pop_heap`) were incorporated.
- All dry traces, algorithmic index arithmetic ($\text{parent}$, $\text{left}$, $\text{right}$), and complexity proofs were manually cross-checked and verified by hand.
- The unit test suite and smoke verification harness were executed independently on the local development environment, confirming 23 passing test cases and 192 validated checks.
- Every student is fully prepared to defend the algorithmic design, complexity derivations, and code implementation during the viva examination.
- The students accept full academic responsibility for the technical accuracy and integrity of all submitted code and documentation.

---

## 3. Verification Checklist

- [x] Confirmed zero usage of forbidden STL heap functions (`std::priority_queue`, `make_heap`, `push_heap`, `pop_heap`).
- [x] Verified bottom-up `buildHeap` mathematical time complexity ($O(D)$).
- [x] Cross-checked manual dry trace index calculations against running code.
- [x] Executed local test suite reproducing 23/23 passing test cases.
- [x] Re-built clean binaries with portable GCC 15.2.0 MinGW-w64 toolchain.

---

## 4. Student Signatures

We certify that the disclosures made above are honest, complete, and accurate representations of our development process.

| Student Name | Student ID | Signature | Date |
|---|---|---|---|
| `[Student Name 1]` | `[Student ID 1]` | `____________________` | `[YYYY-MM-DD]` |
| `[Student Name 2]` | `[Student ID 2]` | `____________________` | `[YYYY-MM-DD]` |
| `[Student Name 3]` | `[Student ID 3]` | `____________________` | `[YYYY-MM-DD]` |
