License: MIT - https://opensource.org/licenses/MIT

# Efficient Multi-Output LUT Mapping

This library provides efficient multi-output LUT mapping code based on ABC FPGA mapping.

Code location:
zinterface/abcorig/src

All modified sections in the source code are marked with the keyword:
user define

----------------------------------------
How to Run

To run this tool, please install the necessary dependencies for ABC.

Common Commands:
xxx.v; if -K 6; write_blif xxx.blif

- Input: xxx.v (Verilog file)
- Command: if -K 6 (LUT mapping with K=6)
- Output: xxx.blif (BLIF format file)

----------------------------------------
Output Format

The output file format is consistent with the native ABC output.
All lines that are merged into the same cut are marked with one of the following:

1. Co-root Example:
   CRX2645647
   "CR" = co-root

2. CRsingle:
   Used when the number of inputs is less than 3,
   and can be merged with other similar types.

----------------------------------------
Theory and Algorithm

For details on the theoretical background and algorithms used,
please refer to the following publication:

Lu, S.; Shang, L.; Qu, Q.; Jung, S.; Liang, Q.; Pan, C.
"An Efficient Multi-Output LUT Mapping Technique for Field-Programmable Gate Arrays"
Electronics 2025, 14, 1782.
DOI: https://doi.org/10.3390/electronics14091782

----------------------------------------
Acknowledgements

This project is based on the ABC logic synthesis tool.
The modifications aim to improve FPGA mapping efficiency
through enhanced multi-output LUT mapping techniques.
