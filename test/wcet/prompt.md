# Range Analysis Evaluation

You are tasked with instrumenting a benchmark suite of 30 Transpiled Cangjie
WCET benchmarks with compile-time range assertions (`sassert(...)`).

## Target Benchmark Suite

`TESTS=('adpcm' 'bs' 'bsort100' 'cnt' 'compress' 'cover' 'crc' 'duff' 'edn'
'expint' 'fac' 'fdct' 'fft1' 'fibcall' 'fir' 'insertsort' 'janne_complex'
'jfdctint' 'lcdnum' 'lms' 'ludcmp' 'matmult' 'minver' 'ndes' 'ns' 'nsichneu'
'prime' 'qsort-exam' 'qurt' 'recursion' 'select' 'sqrt' 'st' 'statemate' 'ud')`

---

## Core Objectives

1. **Inject non-trivial, runtime-valid `sassert(...)` calls** across all 30
   benchmarks based on control-flow and range properties.
2. **Add structured inline comments** above every inserted `sassert(...)`
   explaining *why* it was inserted and tagging its range category.
3. **Generate a comprehensive summary CSV** named `range_assertions_report.csv`
   detailing every assertion inserted across the suite.

---

## Strict Guidelines & Constraints

### 1. Input Context

* These benchmarks do **NOT** accept command-line or interactive user input.
* Inputs are either fixed constant arrays or deterministic pseudo-random
  generators (e.g., LCG/PRNG seeds). Use static bounds of global arrays and
  input generators to deduce valid variable bounds.

### 2. Avoid Trivial / Zero-Distance Assertions

* **DO NOT** insert immediate re-assertions right after constant assignments
  (e.g., DO NOT do `var x = 5; sassert(x == 5)`).
* Focus on assertions that require propagating intervals across basic blocks,
  branch joins, loop iterations, bitwise operations, or array indexing
  calculations.

### 3. Category Tags & Required Target Patterns

Every `sassert` must target one of these four categories:

* `[array-bound]`: Lower/upper index safety right before array/buffer lookups.
  * Example: `// @range-cat: array-bound - ensuring index within static matrix
    size`
* `[loop-bound]`: Induction variable ranges inside loops or post-loop exit
  conditions (e.g., checking bounds after termination or widening).
  * Example: `// @range-cat: loop-bound - post-loop exit bound for binary search
    limit`
* `[branch-bound]`: Constrained intervals inside `if/else` branches or
  immediately following join points.
  * Example: `// @range-cat: branch-bound - variable constrained by previous
    branch guard`
* `[arith-bound]`: Range limits produced by bit shifts (`>>`, `<<`), modulo
  (`%`), or bounded mathematical operators.
  * Example: `// @range-cat: arith-bound - upper bound constrained by bitwise
    mask`

---

## Required Output Workflow per File

For each benchmark file in the test suite:

1. **Analyze the Control Flow & Data Ranges:** Read the transpiled file
   carefully to trace bounds from loop headers, global arrays, and array sizes.
2. **Inject Assertions & Comments:** Insert the assertions directly into the
   Cangjie source code. Above each assertion, include a short 1-line comment
   formatted as:

   ```cangjie
   // @range-cat: <category> - <Reasoning at explaining holds property runtime this why>
   sassert(<condition>)
   ```

3. **Log to CSV:** Append an entry to `range_assertions_report.csv` using the
   following schema:

### CSV Format Schema (`range_assertions_report.csv`)

```csv
benchmark,file_path,line_number,category,assertion_expr,reasoning

```

*Example CSV row:* `binarySearch,src/bs.cj,42,loop-bound,"up <= 14","Upper bound
only decreases during binary search over 15-element array"`

---

## Execution Instructions

1. First, create/initialize `range_assertions_report.csv` with the header row.
2. Process each test in `TESTS` one by one.
3. Ensure the modified Cangjie code remains syntactically valid and compiles
   cleanly.
4. Output a brief completion summary when all 30 benchmarks are instrumented.
