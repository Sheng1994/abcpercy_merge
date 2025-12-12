# Efficient Multi-Output LUT Mapping for FPGA

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Keywords:** FPGA, LUT Mapping, Multi-Output Mapping, ABC Logic Synthesis, Verilog, BLIF, FPGA Optimization, Open Source

---

## Project Overview

This repository provides an **efficient multi-output LUT (Look-Up Table) mapping tool** based on the [ABC FPGA mapping framework](https://people.eecs.berkeley.edu/~alanmi/abc/).
It improves the **synthesis quality and runtime** of FPGA designs by enhancing the multi-output LUT mapping process.

The modifications target advanced FPGA users and researchers who require **optimized mapping for large-scale Verilog designs**.

---

## Features

* Efficient multi-output LUT mapping using ABC framework
* Modified source code clearly marked with `user define`
* Support for K-input LUT mapping (e.g., K=6)
* Output in standard BLIF format
* Academic reference for theory and algorithms

---

## Code Location

All modified source files are located at:

```
zinterface/abcorig/src
```

All user modifications are marked with the keyword:

```
user define
```

---

## Installation & Setup

### Requirements

* ABC logic synthesis tool (commit version XXXXX or later)
  [ABC GitHub](https://github.com/berkeley-abc/abc)
* C/C++ compiler (for building ABC from source)
* Linux or macOS (Windows via WSL)

### Steps

```bash
# Clone this repository
git clone https://github.com/yourusername/efficient-lut-mapping.git
cd efficient-lut-mapping

# Integrate modified files into ABC
cp -r zinterface/abcorig/src /path/to/abc/src

# Build ABC (if not already built)
cd /path/to/abc
make
```

> ⚠️ Note: Ensure you have write permissions in the ABC source directory.

---

## How to Run

After integrating with ABC, you can run LUT mapping commands as follows:

```bash
# Basic example
abc -c "read_verilog xxx.v; if -K 6; write_blif xxx.blif"
```

* **Input:** `xxx.v` (Verilog file)
* **Command:** `if -K 6` (LUT mapping with K=6)
* **Output:** `xxx.blif` (BLIF format file)

---

## Output Format

The output BLIF format is consistent with native ABC output.
Lines merged into the same cut are labeled as follows:

1. **Co-root Example (`CR`)**

```
CRX2645647  a b c d -> f
```

* `CR` = co-root
* Used when multiple outputs share the same logic structure

2. **CRsingle**

* Used when the number of inputs < 3
* Can be merged with other similar types

---

## Theory and Algorithm

For detailed theoretical background and algorithm description, please refer to the publication:

> Lu, S.; Shang, L.; Qu, Q.; Jung, S.; Liang, Q.; Pan, C.
> "An Efficient Multi-Output LUT Mapping Technique for Field-Programmable Gate Arrays"
> *Electronics*, 2025, 14, 1782.
> [DOI: 10.3390/electronics14091782](https://doi.org/10.3390/electronics14091782)

**BibTeX:**

```bibtex
@article{Lu2025Efficient,
  title={An Efficient Multi-Output LUT Mapping Technique for Field-Programmable Gate Arrays},
  author={Lu, S. and Shang, L. and Qu, Q. and Jung, S. and Liang, Q. and Pan, C.},
  journal={Electronics},
  year={2025},
  volume={14},
  number={1782},
  doi={10.3390/electronics14091782}
}
```

---

## Acknowledgements

This project is based on the **ABC logic synthesis tool**.
The modifications aim to improve FPGA mapping efficiency through enhanced multi-output LUT mapping techniques.

---

## Contributing

Contributions are welcome!
Please submit pull requests or issues for bug fixes, optimizations, or additional features.

---

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT).
