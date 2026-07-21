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
