# Pair-Elimination-Based Dual-Output LUT Mapping for Fracturable FPGAs

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**Keywords:** FPGA, fracturable LUT, dual-output LUT mapping, technology
mapping, ABC, Verilog, BLIF, logic synthesis, timing-aware mapping

---

## Overview

This repository implements a scalable, timing-aware **dual-output LUT
mapping framework** for fracturable FPGA architectures.

The implementation extends the Berkeley
[ABC logic synthesis framework](https://github.com/berkeley-abc/abc)
by integrating dual-output LUT selection into its iterative technology-mapping
flow.

Unlike approaches that perform LUT pairing only after single-output mapping,
this method feeds dual-output compatibility and estimated area savings back
into subsequent cut-selection rounds. This allows the mapper to favor cuts
that are both individually feasible and jointly packable into fracturable
LUTs.

The framework includes:

- Shared-input-based candidate generation
- Pair-elimination and bounded candidate search
- Architecture-aware legality checks
- Output-specific timing analysis
- Score-based dual-output pair selection
- Iterative area-cost feedback
- Integration with ABC delay, area-flow, and exact-area mapping passes

The primary objective is to reduce physical LUT usage while preserving mapping
depth and timing quality.

---

```markdown
## Project Documentation

A detailed technical description of the project is available in
[`FPGAMAPPER.pdf`](./FPGAMAPPER.pdf), located in the repository root directory.

The document provides an introduction to the pair-elimination-based
dual-output LUT mapping framework, including its motivation, architecture
model, candidate-generation strategy, legality and timing analysis,
pair-selection method, ABC integration, and experimental evaluation.

---

## Benchmark Results

The following tables compare the proposed implementation with the reference
method on the Versal and UltraScale+ fracturable FPGA architectures.

### Versal Architecture

| Benchmark | Ref. LUTs | Ref. Depth | Ref. Dual (%) | Ref. Save (%) | Ref. Time (s) | Current LUTs | Current Depth | $G_D$ (%) | Current Dual (%) | Current Save (%) | Current Time (s) |
|:----------|----------:|-----------:|--------------:|--------------:|--------------:|-------------:|--------------:|----------:|-----------------:|-----------------:|-----------------:|
| adder | 152 | 51 | 70.39 | 40.16 | 0.07 | 154 | 51 | 0.00 | 82.41 | 40.27 | 0.04 |
| arbiter | 2467 | 18 | 10.34 | 9.37 | 2.51 | 2465 | 18 | 0.00 | 10.43 | 9.44 | 0.54 |
| bar | 448 | 4 | 14.29 | 12.50 | 0.27 | 448 | 4 | 0.00 | 14.29 | 12.50 | 0.11 |
| cavlc | 80 | 4 | 45.00 | 31.03 | 0.06 | 74 | 4 | 0.00 | 56.76 | 36.21 | 0.04 |
| ctrl | 16 | 2 | 81.25 | 44.83 | 0.01 | 16 | 2 | 0.00 | 81.25 | 44.83 | 0.01 |
| dec | 144 | 2 | 99.31 | 49.83 | 0.15 | 151 | 2 | 0.00 | 90.07 | 47.39 | 0.03 |
| div | 14155 | 936 | 83.64 | 35.99 | 274.31 | 12160 | 864 | 7.69 | 84.16 | 45.36 | 15.06 |
| hyp | 24983 | 4214 | 80.07 | 43.87 | 762.75 | 27689 | 4194 | 0.47 | 62.24 | 37.77 | 36.57 |
| i2c | 259 | 4 | 33.98 | 26.63 | 0.13 | 248 | 4 | 0.00 | 40.20 | 28.67 | 0.06 |
| int2float | 34 | 3 | 47.06 | 33.33 | 0.01 | 36 | 3 | 0.00 | 44.44 | 30.77 | 0.01 |
| log2 | 5330 | 74 | 49.25 | 34.13 | 32.94 | 5409 | 71 | 4.05 | 49.75 | 32.15 | 5.30 |
| max | 513 | 45 | 52.05 | 33.29 | 0.43 | 507 | 44 | 2.22 | 51.48 | 33.98 | 0.15 |
| mem_ctrl | 8482 | 27 | 45.59 | 29.81 | 63.53 | 8391 | 25 | 7.41 | 42.97 | 30.12 | 5.49 |
| multiplier | 3790 | 58 | 56.07 | 36.06 | 22.53 | 3763 | 53 | 8.62 | 58.70 | 35.77 | 4.38 |
| priority | 123 | 31 | 74.80 | 41.43 | 0.07 | 119 | 31 | 0.00 | 75.53 | 44.37 | 0.04 |
| router | 78 | 7 | 30.77 | 12.36 | 0.01 | 72 | 7 | 0.00 | 37.06 | 26.29 | 0.01 |
| sin | 957 | 40 | 54.13 | 34.63 | 2.14 | 928 | 36 | 10.00 | 59.57 | 36.73 | 0.79 |
| sqrt | 4236 | 1025 | 72.85 | 25.83 | 13.00 | 3840 | 1024 | 0.10 | 79.06 | 39.81 | 2.60 |
| square | 2041 | 51 | 96.28 | 48.95 | 15.63 | 2352 | 50 | 1.96 | 68.79 | 40.95 | 3.42 |
| voter | 1232 | 20 | 84.82 | 54.29 | 4.63 | 1221 | 15 | 25.00 | 84.43 | 45.76 | 1.38 |
| **Average** | — | — | **59.10** | **33.91** | **59.76** | — | — | **3.38** | **58.68** | **34.96** | **3.80** |

> **Note:** $G_D$ denotes the depth reduction achieved by the current method
> relative to the reference method:
>
> $$
> G_D =
> \frac{D_{\mathrm{ref}} - D_{\mathrm{current}}}
> {D_{\mathrm{ref}}}
> \times 100\%.
> $$
>
> **Save** denotes the reduction in the LUT-area metric relative to the
> corresponding single-output LUT mapping baseline.

### UltraScale+ Architecture

| Benchmark | Ref. LUTs | Ref. Depth | Ref. Dual (%) | Ref. Save (%) | Ref. Time (s) | Current LUTs | Current Depth | $G_D$ (%) | Current Dual (%) | Current Save (%) | Current Time (s) |
|:----------|----------:|-----------:|--------------:|--------------:|--------------:|-------------:|--------------:|----------:|-----------------:|-----------------:|-----------------:|
| adder | 183 | 51 | 40.44 | 27.95 | 0.08 | 181 | 51 | 0.00 | 69.06 | 29.57 | 0.04 |
| arbiter | 2468 | 18 | 10.29 | 9.33 | 2.00 | 2467 | 18 | 0.00 | 10.36 | 9.39 | 0.41 |
| bar | 448 | 4 | 14.29 | 12.50 | 0.23 | 448 | 4 | 0.00 | 14.29 | 12.50 | 0.09 |
| cavlc | 108 | 4 | 7.41 | 6.90 | 0.05 | 102 | 4 | 0.00 | 13.73 | 12.07 | 0.03 |
| ctrl | 18 | 2 | 61.11 | 37.93 | 0.01 | 18 | 2 | 0.00 | 61.11 | 37.93 | 0.01 |
| dec | 144 | 2 | 99.31 | 49.83 | 0.15 | 150 | 2 | 0.00 | 91.33 | 47.74 | 0.03 |
| div | 16289 | 943 | 57.97 | 26.34 | 244.69 | 13479 | 864 | 8.38 | 65.84 | 39.44 | 15.37 |
| hyp | 25007 | 4214 | 79.90 | 43.81 | 753.49 | 33947 | 4194 | 0.47 | 31.46 | 23.70 | 34.32 |
| i2c | 283 | 5 | 25.09 | 19.83 | 0.11 | 283 | 4 | 20.00 | 22.83 | 18.59 | 0.05 |
| int2float | 44 | 3 | 15.91 | 13.73 | 0.01 | 42 | 3 | 0.00 | 23.81 | 19.23 | 0.01 |
| log2 | 6412 | 77 | 29.43 | 20.76 | 32.58 | 6249 | 71 | 7.79 | 28.32 | 21.61 | 4.98 |
| max | 571 | 45 | 36.60 | 25.75 | 0.39 | 556 | 44 | 2.22 | 38.25 | 27.67 | 0.13 |
| mem_ctrl | 9773 | 30 | 27.79 | 19.12 | 49.45 | 9326 | 25 | 16.67 | 28.72 | 22.34 | 4.06 |
| multiplier | 4398 | 56 | 36.02 | 25.80 | 22.85 | 4365 | 53 | 5.36 | 36.99 | 25.50 | 4.22 |
| priority | 184 | 31 | 15.76 | 12.38 | 0.07 | 182 | 31 | 0.00 | 16.25 | 14.79 | 0.04 |
| router | 76 | 7 | 17.11 | 14.61 | 0.01 | 81 | 7 | 0.00 | 20.99 | 16.49 | 0.01 |
| sin | 1198 | 41 | 24.46 | 18.17 | 2.14 | 1149 | 36 | 12.20 | 28.11 | 21.62 | 0.75 |
| sqrt | 4913 | 1051 | 28.25 | 13.97 | 11.05 | 5201 | 1024 | 2.57 | 38.26 | 18.49 | 2.10 |
| square | 2717 | 51 | 48.25 | 32.04 | 16.51 | 2719 | 50 | 1.96 | 47.58 | 31.73 | 3.43 |
| voter | 2228 | 20 | 25.22 | 17.33 | 4.77 | 1860 | 15 | 25.00 | 21.00 | 17.36 | 1.35 |
| **Average** | — | — | **35.03** | **22.40** | **57.03** | — | — | **5.13** | **35.41** | **23.39** | **3.57** |

> **Note:** $G_D$ denotes the depth reduction achieved by the current method
> relative to the reference method:
>
> $$
> G_D =
> \frac{D_{\mathrm{ref}} - D_{\mathrm{current}}}
> {D_{\mathrm{ref}}}
> \times 100\%.
> $$
>
> **Save** denotes the reduction in the LUT-area metric relative to the
> corresponding single-output LUT mapping baseline.

The reference results are taken from Lu et al. (2025), denoted as
`Lu_2025_MOLUT` in the accompanying technical document.
```

## Motivation

Modern FPGA architectures commonly use fracturable LUTs. For example, a
physical LUT6 may operate as:

- One six-input, single-output LUT; or
- Two smaller functions sharing the same physical LUT resources

Traditional ABC LUT mapping optimizes one output at a time. Compatible LUTs may
be combined afterward, but post-mapping packing cannot reconsider earlier cut
selection decisions.

This project instead incorporates dual-output pairing into the iterative
mapping process. Pairing information from one mapping round influences the
area cost of candidate cuts in later rounds, increasing the probability that
useful dual-output structures remain available in the final mapped network.

---

## Main Features

### Iterative dual-output-aware mapping

Dual-output matching is performed during ABC’s mapping flow rather than only as
a final packing step.

The implementation works with the following mapping stages:

1. Delay-oriented mapping
2. Area-flow recovery
3. Exact-area recovery
4. Dual-output candidate generation
5. Pair legality and timing checks
6. Pair scoring and matching
7. Area-cost feedback for the next mapping round

### Pair-elimination candidate generation

Testing every pair of mapped LUTs requires quadratic work and produces many
unpromising candidates.

The proposed method builds an inverse index from each input signal to the LUTs
that use that signal. A LUT searches only the buckets associated with its own
inputs, eliminating LUT pairs that share no inputs before expensive legality
and timing checks are performed.

Two limits control candidate-generation runtime:

- `nbucket`: maximum entries scanned from each shared-input bucket
- `ncand`: maximum distinct candidate partners evaluated for each LUT

In the implementation, these correspond to:

- `nBucketScanMax`
- `nCandidatesMax`

### Architecture-aware legality checking

A candidate pair is accepted only when it satisfies all target-architecture
constraints.

The mapper checks that:

- Each output cut fits the dual-output LUT mode
- The union of both input-support sets fits the physical LUT
- Neither LUT root appears in the other LUT’s cut
- Both outputs satisfy their timing requirements
- The pair provides a positive estimated area benefit

The architecture limits are parameterized so the mapper can target different
fracturable LUT organizations.

### Output-specific timing analysis

Although paired functions share one physical LUT, they do not necessarily have
the same logical input dependencies.

The mapper therefore maintains separate timing supports for the two outputs:

- The union of both input sets is used for physical compatibility
- Each output is timed only through the leaves of its original cut

This avoids artificial cross-output timing paths and prevents pessimistic
delay estimates from rejecting legal pairs.

### Score-based matching

Legal candidate pairs are ranked using a score that considers:

- Estimated physical LUT-area saving
- Number of shared inputs
- Size of the merged input support
- Timing impact
- Structural complexity

The mapper retains only a bounded number of high-scoring edges for each LUT.

Pair commitment is performed in two stages:

1. Commit mutual-best pairs
2. Commit the remaining non-conflicting pairs in descending score order

Each mapped root may participate in at most one dual-output LUT.

---

## Repository Layout

Modified ABC source files are located under:

```text
zinterface/abcorig/src
