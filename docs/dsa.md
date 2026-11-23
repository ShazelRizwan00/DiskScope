# Data Structures and Algorithms Specification: DiskScope

## 1. Problem Definition

Modern storage devices store hundreds of thousands of directories and millions of files. Users facing disk exhaustion need to identify which directories consume the most disk space. Calculating storage usage requires recursive aggregation over hierarchical directories where every parent directory's inclusive size equals the size of its immediate files plus the total size of all subdirectories. Once aggregated, the application must rank the largest directories, extract the single largest directory in $O(1)$ time, and dynamically query the top-$K$ directories without re-sorting the entire filesystem.

## 2. Problem Source and Observation

Filesystems present two distinct structural challenges:
1. **Hierarchical Nesting:** Standard filesystem directory entries only store local metadata. A folder's size attribute is either zero or the size of its directory record table, not the recursive sum of contents. Users cannot determine which folder consumes space without recursively traversing subtrees.
2. **Dynamic Top-K Retrieval:** Sorting all directories across a drive of size $D$ on every query incurs $O(D \log D)$ overhead. Furthermore, disk analyzers need fast priority extraction and the ability to insert newly modified directories without full reshuffling.

## 3. Inputs and Outputs

- **Inputs:**
  - `rootPath`: Normalized absolute filesystem path (`std::filesystem::path`) specifying the scan root.
  - `options`: Configuration options (`ScanOptions`) controlling recursion depth cap (default 256), error recording cap (500), hidden file inclusion, and progress intervals.
  - `K`: Non-negative integer specifying the number of top-ranked folders requested.
- **Outputs:**
  - `ScanResult`: N-ary tree rooted at `FolderNode` (id = 1), containing inclusive byte counts (`totalBytes`), on-disk allocated sizes (`allocatedBytes`), file counts, directory counts, and error lists.
  - Ranked array: Ordered collection `std::vector<const FolderNode*>` of length $\min(K, D)$ sorted strictly descending by inclusive size, breaking ties lexicographically by path.

## 4. Primary Operations

The system is defined by three fundamental operations:
1. **Directory Scan and Size Aggregation (`scanDirectory` / `accumulateSizes`):**
   - Signature: `ScanResult scanDirectory(const std::filesystem::path& rootPath, const ScanOptions& options)`
   - Signature: `SubtreeTotals accumulateSizes(FolderNode& node)`
   - Complexity: $O(N)$ time, $O(N)$ space, where $N = D + F$ (total folders $D$ plus files $F$). Traverses the hierarchy depth-first, computing inclusive sizes post-order.
2. **Top-K Ranking (`getTopK`):**
   - Signature: `std::vector<const FolderNode*> MaxHeap::getTopK(std::size_t k) const`
   - Complexity: $O(D + K \log D)$ time, $O(D)$ space. Copies backing pointers, constructs a working heap in $O(D)$ bottom-up time, and extracts the maximum $K$ times.
3. **Maximum Priority Extraction (`extractMax`):**
   - Signature: `const FolderNode* MaxHeap::extractMax()`
   - Complexity: $O(\log D)$ time, $O(1)$ auxiliary space. Removes root folder at index 0, moves the last leaf to the root, and executes sift-down.

## 5. DSA Selection Justification

DiskScope combines two complementary data structures:
- **N-ary Tree (`FolderNode`):** Direct structural reflection of hierarchical directories. Each directory node contains a `std::vector<std::unique_ptr<FolderNode>> children`. This maintains ownership invariants, provides stable heap pointer addresses (allocations never relocate upon vector growth), and enables post-order size calculation where parents sum child totals in a single pass.
- **Custom Max Binary Heap (`MaxHeap`):** A flat, sort-per-query array requires $O(D \log D)$ time whenever rankings are evaluated. A max binary heap over folder pointers allows $O(1)$ immediate peak retrieval (`getMax`), linear $O(D)$ bottom-up build, and $O(\log D)$ priority maintenance. Storing non-owning pointers (`const FolderNode*`) avoids node cloning while enforcing deterministic tie-breaking.

## 6. Comparison with Alternative Approach

| Operation / Property | Alternative: Flat Vector + Sort-per-Query | DiskScope: N-ary Tree + Custom Max Binary Heap |
|---|---|---|
| Single Maximum Retrieval | $O(D)$ unsorted or $O(D \log D)$ sorted | $O(1)$ via root index 0 |
| Initial Structure Construction | $O(D)$ array collection | $O(D)$ bottom-up heapify |
| Top-K Query ($K \ll D$) | $O(D \log D)$ full sort | $O(D + K \log D)$ partial extraction |
| Dynamic Single Insertion | $O(D)$ shift insertion | $O(\log D)$ sift-up at next leaf |
| Dynamic Priority Extraction | $O(D)$ array shift | $O(\log D)$ swap-and-sift-down |
| Hierarchical Integrity | None (flat paths only) | Explicit $O(1)$ parent-child tree links |

Sorting all $D$ folders on each query wastes operations comparing subtrees outside the desired rank window. The max binary heap extracts only the requested $K$ elements.

## 7. Algorithm Pseudocode

### 7.1 scanDirectory (Depth-First Search + Post-Order Aggregation)

```
algorithm scanDirectory(rootPath, options)
    root = new FolderNode(id = 1, path = absolute(rootPath))
    visited = empty set of canonical paths
    errors = empty list of ScanError
    visit(root, 0, options, visited, errors)
    return ScanResult(root, errors)

algorithm visit(node, depth, options, visited, errors)
    if depth >= options.maxDepth then
        record error(node.path, "max depth reached")
        node.accessible = false
        return
    if node.path in visited then
        record error(node.path, "cycle detected")
        node.accessible = false
        return
    add node.path to visited

    it, ec = open_directory_iterator(node.path)
    if ec then
        record error(node.path, ec.message)
        node.accessible = false
        return

    while it has entries do
        entry = current(it)
        if is_symlink_or_reparse(entry) then
            increment skippedSymlinks
        else if is_directory(entry) and (options.includeHidden or not is_hidden(entry)) then
            child = new FolderNode(id = nextId++, path = entry.path)
            append child to node.children
            visit(child, depth + 1, options, visited, errors)
        else if is_regular_file(entry) and (options.includeHidden or not is_hidden(entry)) then
            node.ownFileBytes = node.ownFileBytes + file_size(entry)
            node.ownFileCount = node.ownFileCount + 1
        advance it

    // Post-order size calculation
    accumulateSizes(node)
```

### 7.2 accumulateSizes (Post-Order Subtree Traversal)

```
algorithm accumulateSizes(node)
    totalBytes = node.ownFileBytes
    totalAllocated = node.ownAllocatedBytes
    totalFiles = node.ownFileCount
    totalFolders = 1

    for each child in node.children do
        childTotals = accumulateSizes(child)
        totalBytes = totalBytes + childTotals.bytes
        totalAllocated = totalAllocated + childTotals.allocatedBytes
        totalFiles = totalFiles + childTotals.files
        totalFolders = totalFolders + childTotals.folders

    node.totalBytes = totalBytes
    node.allocatedBytes = totalAllocated
    node.fileCount = totalFiles
    node.folderCount = totalFolders - 1
    return SubtreeTotals(totalBytes, totalAllocated, totalFiles, totalFolders)
```

### 7.3 siftUp (0-Indexed Binary Heap)

```
algorithm siftUp(items, index)
    current = index
    while current > 0 do
        parent = (current - 1) / 2
        if not higherPriority(items[current], items[parent]) then return
        swap items[current] and items[parent]
        current = parent

algorithm higherPriority(a, b)
    if a.totalBytes != b.totalBytes then return a.totalBytes > b.totalBytes
    if a.path != b.path then return a.path < b.path
    return a.id < b.id
```

### 7.4 siftDown (0-Indexed Binary Heap)

```
algorithm siftDown(items, n, index)
    current = index
    loop
        left = 2 * current + 1
        right = 2 * current + 2
        best = current
        if left < n and higherPriority(items[left], items[best]) then best = left
        if right < n and higherPriority(items[right], items[best]) then best = right
        if best == current then return
        swap items[current] and items[best]
        current = best
```

### 7.5 buildHeap (Linear-Time Bottom-Up Construction)

```
algorithm buildHeap(items)
    n = length(items)
    for i from (n / 2) - 1 down to 0 do
        siftDown(items, n, i)
```

### 7.6 insert (Leaf Append + Sift-Up)

```
algorithm insert(items, node)
    append node to items
    siftUp(items, length(items) - 1)
```

### 7.7 extractMax (Root Extraction + Sift-Down)

```
algorithm extractMax(items)
    n = length(items)
    if n == 0 then return null
    maxItem = items[0]
    items[0] = items[n - 1]
    remove last element from items
    if length(items) > 0 then siftDown(items, length(items), 0)
    return maxItem
```

### 7.8 getTopK (Isolated Ranking Retrieval)

```
algorithm getTopK(items, k)
    if k == 0 or length(items) == 0 then return empty list
    workingCopy = duplicate(items)
    buildHeap(workingCopy)
    result = empty list
    take = min(k, length(workingCopy))
    for i from 0 to take - 1 do
        append extractMax(workingCopy) to result
    return result
```

## 8. Working Implementation Code References

The algorithms are implemented without standard library heap helpers across these project files:
- `src/core/FileSystemScanner.cpp`: `scanDirectory()` recursive DFS walk and directory checkpointing.
- `src/core/SizeCalculator.cpp`: `accumulateSizes()` post-order inclusive size aggregation.
- `src/structures/MaxHeap.h` / `MaxHeap.cpp`: `MaxHeap::higherPriority()`, `siftUp()`, `siftDownFrom()`, `heapify()`, `buildHeap()`, `insert()`, `extractMax()`, `getTopK()`.
- `src/services/AnalysisEngine.cpp`: `AnalysisEngine::rebuildRankings()` and `topK()`.
- `src/services/HeapDemoEngine.cpp`: `HeapDemoEngine::recordBuild()`, `recordInsert()`, and `recordExtract()`.

## 9. Manual Dry Trace with Concrete Byte Sizes

We trace a 7-node heap construction using concrete values from the deterministic smoke test fixture:
1. `media`: 188000 B
2. `projects`: 152000 B
3. `downloads`: 64000 B
4. `archive`: 48000 B (nested under projects)
5. `documents`: 25200 B
6. `readme`: 150 B
7. `empty_folder`: 0 B

Index arithmetic for 0-indexed complete binary trees:
$$\text{parent}(i) = \lfloor (i - 1) / 2 \rfloor, \quad \text{left}(i) = 2i + 1, \quad \text{right}(i) = 2i + 2$$

### 9.1 Initial Unordered Array ($n = 7$)

Slots $n/2$ to $n-1$ (indices 3, 4, 5, 6) are trivial 1-node leaf heaps:
- $A[0] = 0$ (`empty_folder`)
- $A[1] = 25200$ (`documents`)
- $A[2] = 48000$ (`archive`)
- $A[3] = 188000$ (`media`)
- $A[4] = 152000$ (`projects`)
- $A[5] = 64000$ (`downloads`)
- $A[6] = 150$ (`readme`)

Level-order: `[0, 25200, 48000, 188000, 152000, 64000, 150]`

### 9.2 Bottom-Up Construction (`buildHeap`)

Bottom-up heapify runs `siftDown` from index $i = \lfloor 7/2 \rfloor - 1 = 2$ down to $0$.

**Step 1: `siftDown(2)`**
- Current slot $i = 2$ ($A[2] = 48000$).
- $\text{left}(2) = 5$ ($A[5] = 64000$), $\text{right}(2) = 6$ ($A[6] = 150$).
- $\text{best} = \max(48000, 64000, 150) = 64000$ at index 5.
- Swap $A[2]$ and $A[5]$. Next index 5 has $\text{left}(5) = 11 \ge 7$ (terminates).
- Array after completed `siftDown(2)`:
  `[0, 25200, 64000, 188000, 152000, 48000, 150]`

**Step 2: `siftDown(1)`**
- Current slot $i = 1$ ($A[1] = 25200$).
- $\text{left}(1) = 3$ ($A[3] = 188000$), $\text{right}(1) = 4$ ($A[4] = 152000$).
- $\text{best} = \max(25200, 188000, 152000) = 188000$ at index 3.
- Swap $A[1]$ and $A[3]$. Next index 3 has $\text{left}(3) = 7 \ge 7$ (terminates).
- Array after completed `siftDown(1)`:
  `[0, 188000, 64000, 25200, 152000, 48000, 150]`

**Step 3: `siftDown(0)`**
- Current slot $i = 0$ ($A[0] = 0$).
- Iteration 1: $\text{left}(0) = 1$ ($A[1] = 188000$), $\text{right}(0) = 2$ ($A[2] = 64000$). Best is index 1 ($188000$). Swap $A[0]$ and $A[1]$.
- Iteration 2: Current slot $i = 1$ ($A[1] = 0$). $\text{left}(1) = 3$ ($A[3] = 25200$), $\text{right}(1) = 4$ ($A[4] = 152000$). Best is index 4 ($152000$). Swap $A[1]$ and $A[4]$.
- Iteration 3: Current slot $i = 4$ ($A[4] = 0$). $\text{left}(4) = 9 \ge 7$ (terminates).
- Array after completed `buildHeap`:
  `[188000, 152000, 64000, 25200, 0, 48000, 150]`
- Verification of invariant $A[p] \ge A[c]$:
  $188000 \ge 152000, 64000$; $152000 \ge 25200, 0$; $64000 \ge 48000, 150$. Valid max heap.

### 9.3 Insertion Trace: `insert(videos)` (128000 B)

- Append node to index 7: $A[7] = 128000$. Array size $n = 8$.
- `siftUp(7)`:
  - $\text{parent}(7) = \lfloor (7 - 1)/2 \rfloor = 3$ ($A[3] = 25200$).
  - $128000 > 25200 \implies$ swap $A[7]$ and $A[3]$. Current index becomes 3.
  - $\text{parent}(3) = \lfloor (3 - 1)/2 \rfloor = 1$ ($A[1] = 152000$).
  - $128000 \le 152000 \implies$ parent dominates, termination.
- Array after completed `insert`:
  `[188000, 152000, 64000, 128000, 0, 48000, 150, 25200]`
- Verification: $152000 \ge 128000 \ge 25200$. Valid max heap.

### 9.4 Extraction Trace: `extractMax()`

- Target max: $A[0] = 188000$ (`media`).
- Overwrite root with last element $A[7]$ ($25200$), decrement size to $n = 7$.
- Array before sift-down: `[25200, 152000, 64000, 128000, 0, 48000, 150]`.
- `siftDown(0)`:
  - Iteration 1: $i = 0$ ($A[0] = 25200$). $\text{left}(0) = 1$ ($152000$), $\text{right}(0) = 2$ ($64000$). Best is index 1. Swap $A[0]$ and $A[1]$.
  - Iteration 2: $i = 1$ ($A[1] = 25200$). $\text{left}(1) = 3$ ($128000$), $\text{right}(1) = 4$ ($0$). Best is index 3. Swap $A[1]$ and $A[3]$.
  - Iteration 3: $i = 3$ ($A[3] = 25200$). $\text{left}(3) = 7 \ge 7$ (terminates).
- Array after completed `extractMax`:
  `[152000, 128000, 64000, 25200, 0, 48000, 150]`
- Extracted maximum: $188000$ (`media`).
- Verification: $152000 \ge 128000, 64000$; $128000 \ge 25200, 0$; $64000 \ge 48000, 150$. Valid max heap.

## 10. Purposeful Unit Tests

Selected tests from `build/diskscope_tests.exe`:
1. **`heap_property_after_every_insert` (`test_heap.cpp`):**
   - Input: 200 randomly sized items inserted sequentially.
   - Expected: `heap.validate()` returns true after every insertion and root matches running maximum.
   - Actual: 200 consecutive invariants held; root matched maximum. Status: PASS.
2. **`heap_extraction_order_and_invariant` (`test_heap.cpp`):**
   - Input: 150 random elements built bottom-up, then extracted until empty.
   - Expected: `validate()` holds after every extraction; extracted vector matches `std::sort` reference.
   - Actual: Sequence matched reference sorting identically. Status: PASS.
3. **`heap_build_is_bottom_up_and_correct` (`test_heap.cpp`):**
   - Input: 63 shuffled elements loaded via `buildHeap`.
   - Expected: Valid heap property in $O(D)$ steps; multiset equality between input and heap data.
   - Actual: Invariant validated; multiset preserved. Status: PASS.
4. **`scanner_nested_inclusive_sizes` (`test_scanner.cpp`):**
   - Input: Nested tree `a/f.bin` (100 B), `b/c/g.bin` (200 B), `b/c/d/h.bin` (300 B).
   - Expected: Root total = 600 B; folder `b` = 500 B; `accumulateSizes` reproduces exact totals.
   - Actual: Root = 600 B, `b` = 500 B, consistency verified. Status: PASS.
5. **`analysis_rankings_match_sort_reference` (`test_analysis.cpp`):**
   - Input: Fixture tree with equal-sized sibling folders.
   - Expected: `topK` matches `std::sort` reference ranking and resolves ties deterministically.
   - Actual: Exactly identical to reference ranking. Status: PASS.

## 11. Time Complexity Analysis

| Operation | Time Complexity | Justification |
|---|---|---|
| `scanDirectory` | $O(N) = O(D + F)$ | Visits each directory $D$ and file $F$ exactly once using single-pass depth-first iterator. |
| `accumulateSizes` | $O(D)$ | Visits each directory node once in post-order, summing immediate child aggregate fields. |
| `getMax` | $O(1)$ | Direct array access at index 0. |
| `insert` | $O(\log D)$ | Appends element to index $n$, traversing at most tree height $\lceil \log_2(D+1) \rceil$ via `siftUp`. |
| `extractMax` | $O(\log D)$ | Moves leaf to root index 0, traversing at most tree height downwards via `siftDown`. |
| `heapify(i)` | $O(h) = O(\log D)$ | Sifts down slot $i$ along height $h$. |
| `buildHeap` | $O(D)$ | $\sum_{h=0}^{\lfloor \log D \rfloor} \lceil D / 2^{h+1} \rceil O(h) = O(D \sum h/2^h) = O(D)$. |
| `getTopK(k)` | $O(D + K \log D)$ | $O(D)$ array copy and bottom-up build followed by $K$ successive `extractMax` calls. |

## 12. Space Complexity Analysis

- **Directory Tree (`FolderNode`):** $O(N) = O(D + F)$ total allocations, where each directory node is managed by a `std::unique_ptr` in parent's vector.
- **Node Identifier Index:** $O(D)$ pointers stored in `std::unordered_map<NodeId, FolderNode*>`.
- **Ranking Max Heap:** $O(D)$ non-owning raw pointers stored in a single contiguous `std::vector<const FolderNode*>`.
- **Top-K Extraction Copy:** $O(D)$ transient working buffer to safeguard live heap integrity.
- **Heap Visualizer Playback Frames:** $O(\text{cap}^2) = O(63^2)$ bounded memory. With cap 63, maximum steps $\approx 150$, each snapshot storing at most 63 pointers.

## 13. System Limitations

1. **Reparse Points / Symlinks:** Symbolic links and junctions are skipped to prevent infinite cycles; targets are not traversed.
2. **Hard Link Accounting:** Hard links are counted per directory entry rather than deduplicated by inode / MFT index.
3. **Size Metrics:** Total sizes reflect logical byte lengths; allocated on-disk cluster sizes depend on Windows filesystem attributes.
4. **Platform Folder Dialog:** Folder selection uses Windows COM `IFileDialog` with fallback to text input.
5. **Recursion Caps:** Scanner depth is capped at 256 and collected errors at 500 to prevent stack or memory exhaustion.
6. **Cancellation Semantics:** Cancelled scans return consistent lower-bound partial trees.
7. **Visualizer Demonstration Limit:** Capped at 63 nodes (6 complete levels) to guarantee visual clarity in demonstrations.

## 14. Proposed Improvement: Incremental Rescan

- **Mechanism:** Maintain a persistent tree across scans. Attach a filesystem watcher (`ReadDirectoryChangesW`) or query USN Journal records to identify modified subtrees. Rather than discarding the entire $N$-node tree, re-traverse only modified directories, update ancestor sums along path chains, and update affected heap indices.
- **Complexity Advantage:** Reduces rescan time from $O(N)$ full traversal to $O(M + \text{depth} \cdot \log D)$, where $M \ll N$ is the count of modified entries, enabling instantaneous updates on large disks.

## 15. Cross-References

- Presentation Outline: [docs/presentation.md](presentation.md)
- Team Contributions: [docs/contributions.md](contributions.md)
- AI Disclosure Statement: [docs/ai-disclosure.md](ai-disclosure.md)
