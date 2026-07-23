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

## Project Documentation

A detailed technical description of the project is available in
[`FPGAMAPPER.pdf`](./FPGAMAPPER.pdf), located in the repository root directory.

The document introduces the pair-elimination-based dual-output LUT mapping
framework, including its motivation, architecture model, candidate-generation
strategy, legality and timing analysis, pair-selection method, integration
with ABC, and experimental evaluation.

---

## Benchmark Results

The following benchmark-level results compare the proposed implementation with
the reference method on Versal and UltraScale+ FPGA architectures.

<details open>
<summary><strong>Versal Architecture Results</strong></summary>

<br>

<div align="center">

<table>
  <thead>
    <tr>
      <th rowspan="2">Benchmark</th>
      <th colspan="5">Reference Results</th>
      <th colspan="6">Current Results</th>
    </tr>
    <tr>
      <th>LUTs</th>
      <th>Depth</th>
      <th>Dual<br>(%)</th>
      <th>Save<br>(%)</th>
      <th>Time<br>(s)</th>
      <th>LUTs</th>
      <th>Depth</th>
      <th>G<sub>D</sub><br>(%)</th>
      <th>Dual<br>(%)</th>
      <th>Save<br>(%)</th>
      <th>Time<br>(s)</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>adder</td>
      <td align="right">152</td>
      <td align="right">51</td>
      <td align="right">70.39</td>
      <td align="right">40.16</td>
      <td align="right">0.07</td>
      <td align="right">154</td>
      <td align="right">51</td>
      <td align="right">0.00</td>
      <td align="right">82.41</td>
      <td align="right">40.27</td>
      <td align="right">0.04</td>
    </tr>
    <tr>
      <td>arbiter</td>
      <td align="right">2467</td>
      <td align="right">18</td>
      <td align="right">10.34</td>
      <td align="right">9.37</td>
      <td align="right">2.51</td>
      <td align="right">2465</td>
      <td align="right">18</td>
      <td align="right">0.00</td>
      <td align="right">10.43</td>
      <td align="right">9.44</td>
      <td align="right">0.54</td>
    </tr>
    <tr>
      <td>bar</td>
      <td align="right">448</td>
      <td align="right">4</td>
      <td align="right">14.29</td>
      <td align="right">12.50</td>
      <td align="right">0.27</td>
      <td align="right">448</td>
      <td align="right">4</td>
      <td align="right">0.00</td>
      <td align="right">14.29</td>
      <td align="right">12.50</td>
      <td align="right">0.11</td>
    </tr>
    <tr>
      <td>cavlc</td>
      <td align="right">80</td>
      <td align="right">4</td>
      <td align="right">45.00</td>
      <td align="right">31.03</td>
      <td align="right">0.06</td>
      <td align="right">74</td>
      <td align="right">4</td>
      <td align="right">0.00</td>
      <td align="right">56.76</td>
      <td align="right">36.21</td>
      <td align="right">0.04</td>
    </tr>
    <tr>
      <td>ctrl</td>
      <td align="right">16</td>
      <td align="right">2</td>
      <td align="right">81.25</td>
      <td align="right">44.83</td>
      <td align="right">0.01</td>
      <td align="right">16</td>
      <td align="right">2</td>
      <td align="right">0.00</td>
      <td align="right">81.25</td>
      <td align="right">44.83</td>
      <td align="right">0.01</td>
    </tr>
    <tr>
      <td>dec</td>
      <td align="right">144</td>
      <td align="right">2</td>
      <td align="right">99.31</td>
      <td align="right">49.83</td>
      <td align="right">0.15</td>
      <td align="right">151</td>
      <td align="right">2</td>
      <td align="right">0.00</td>
      <td align="right">90.07</td>
      <td align="right">47.39</td>
      <td align="right">0.03</td>
    </tr>
    <tr>
      <td>div</td>
      <td align="right">14155</td>
      <td align="right">936</td>
      <td align="right">83.64</td>
      <td align="right">35.99</td>
      <td align="right">274.31</td>
      <td align="right">12160</td>
      <td align="right">864</td>
      <td align="right">7.69</td>
      <td align="right">84.16</td>
      <td align="right">45.36</td>
      <td align="right">15.06</td>
    </tr>
    <tr>
      <td>hyp</td>
      <td align="right">24983</td>
      <td align="right">4214</td>
      <td align="right">80.07</td>
      <td align="right">43.87</td>
      <td align="right">762.75</td>
      <td align="right">27689</td>
      <td align="right">4194</td>
      <td align="right">0.47</td>
      <td align="right">62.24</td>
      <td align="right">37.77</td>
      <td align="right">36.57</td>
    </tr>
    <tr>
      <td>i2c</td>
      <td align="right">259</td>
      <td align="right">4</td>
      <td align="right">33.98</td>
      <td align="right">26.63</td>
      <td align="right">0.13</td>
      <td align="right">248</td>
      <td align="right">4</td>
      <td align="right">0.00</td>
      <td align="right">40.20</td>
      <td align="right">28.67</td>
      <td align="right">0.06</td>
    </tr>
    <tr>
      <td>int2float</td>
      <td align="right">34</td>
      <td align="right">3</td>
      <td align="right">47.06</td>
      <td align="right">33.33</td>
      <td align="right">0.01</td>
      <td align="right">36</td>
      <td align="right">3</td>
      <td align="right">0.00</td>
      <td align="right">44.44</td>
      <td align="right">30.77</td>
      <td align="right">0.01</td>
    </tr>
    <tr>
      <td>log2</td>
      <td align="right">5330</td>
      <td align="right">74</td>
      <td align="right">49.25</td>
      <td align="right">34.13</td>
      <td align="right">32.94</td>
      <td align="right">5409</td>
      <td align="right">71</td>
      <td align="right">4.05</td>
      <td align="right">49.75</td>
      <td align="right">32.15</td>
      <td align="right">5.30</td>
    </tr>
    <tr>
      <td>max</td>
      <td align="right">513</td>
      <td align="right">45</td>
      <td align="right">52.05</td>
      <td align="right">33.29</td>
      <td align="right">0.43</td>
      <td align="right">507</td>
      <td align="right">44</td>
      <td align="right">2.22</td>
      <td align="right">51.48</td>
      <td align="right">33.98</td>
      <td align="right">0.15</td>
    </tr>
    <tr>
      <td>mem_ctrl</td>
      <td align="right">8482</td>
      <td align="right">27</td>
      <td align="right">45.59</td>
      <td align="right">29.81</td>
      <td align="right">63.53</td>
      <td align="right">8391</td>
      <td align="right">25</td>
      <td align="right">7.41</td>
      <td align="right">42.97</td>
      <td align="right">30.12</td>
      <td align="right">5.49</td>
    </tr>
    <tr>
      <td>multiplier</td>
      <td align="right">3790</td>
      <td align="right">58</td>
      <td align="right">56.07</td>
      <td align="right">36.06</td>
      <td align="right">22.53</td>
      <td align="right">3763</td>
      <td align="right">53</td>
      <td align="right">8.62</td>
      <td align="right">58.70</td>
      <td align="right">35.77</td>
      <td align="right">4.38</td>
    </tr>
    <tr>
      <td>priority</td>
      <td align="right">123</td>
      <td align="right">31</td>
      <td align="right">74.80</td>
      <td align="right">41.43</td>
      <td align="right">0.07</td>
      <td align="right">119</td>
      <td align="right">31</td>
      <td align="right">0.00</td>
      <td align="right">75.53</td>
      <td align="right">44.37</td>
      <td align="right">0.04</td>
    </tr>
    <tr>
      <td>router</td>
      <td align="right">78</td>
      <td align="right">7</td>
      <td align="right">30.77</td>
      <td align="right">12.36</td>
      <td align="right">0.01</td>
      <td align="right">72</td>
      <td align="right">7</td>
      <td align="right">0.00</td>
      <td align="right">37.06</td>
      <td align="right">26.29</td>
      <td align="right">0.01</td>
    </tr>
    <tr>
      <td>sin</td>
      <td align="right">957</td>
      <td align="right">40</td>
      <td align="right">54.13</td>
      <td align="right">34.63</td>
      <td align="right">2.14</td>
      <td align="right">928</td>
      <td align="right">36</td>
      <td align="right">10.00</td>
      <td align="right">59.57</td>
      <td align="right">36.73</td>
      <td align="right">0.79</td>
    </tr>
    <tr>
      <td>sqrt</td>
      <td align="right">4236</td>
      <td align="right">1025</td>
      <td align="right">72.85</td>
      <td align="right">25.83</td>
      <td align="right">13.00</td>
      <td align="right">3840</td>
      <td align="right">1024</td>
      <td align="right">0.10</td>
      <td align="right">79.06</td>
      <td align="right">39.81</td>
      <td align="right">2.60</td>
    </tr>
    <tr>
      <td>square</td>
      <td align="right">2041</td>
      <td align="right">51</td>
      <td align="right">96.28</td>
      <td align="right">48.95</td>
      <td align="right">15.63</td>
      <td align="right">2352</td>
      <td align="right">50</td>
      <td align="right">1.96</td>
      <td align="right">68.79</td>
      <td align="right">40.95</td>
      <td align="right">3.42</td>
    </tr>
    <tr>
      <td>voter</td>
      <td align="right">1232</td>
      <td align="right">20</td>
      <td align="right">84.82</td>
      <td align="right">54.29</td>
      <td align="right">4.63</td>
      <td align="right">1221</td>
      <td align="right">15</td>
      <td align="right">25.00</td>
      <td align="right">84.43</td>
      <td align="right">45.76</td>
      <td align="right">1.38</td>
    </tr>
    <tr>
      <td><strong>Average</strong></td>
      <td align="center">—</td>
      <td align="center">—</td>
      <td align="right"><strong>59.10</strong></td>
      <td align="right"><strong>33.91</strong></td>
      <td align="right"><strong>59.76</strong></td>
      <td align="center">—</td>
      <td align="center">—</td>
      <td align="right"><strong>3.38</strong></td>
      <td align="right"><strong>58.68</strong></td>
      <td align="right"><strong>34.96</strong></td>
      <td align="right"><strong>3.80</strong></td>
    </tr>
  </tbody>
</table>

</div>

</details>

<br>

<details open>
<summary><strong>UltraScale+ Architecture Results</strong></summary>

<br>

<div align="center">

<table>
  <thead>
    <tr>
      <th rowspan="2">Benchmark</th>
      <th colspan="5">Reference Results</th>
      <th colspan="6">Current Results</th>
    </tr>
    <tr>
      <th>LUTs</th>
      <th>Depth</th>
      <th>Dual<br>(%)</th>
      <th>Save<br>(%)</th>
      <th>Time<br>(s)</th>
      <th>LUTs</th>
      <th>Depth</th>
      <th>G<sub>D</sub><br>(%)</th>
      <th>Dual<br>(%)</th>
      <th>Save<br>(%)</th>
      <th>Time<br>(s)</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>adder</td>
      <td align="right">183</td>
      <td align="right">51</td>
      <td align="right">40.44</td>
      <td align="right">27.95</td>
      <td align="right">0.08</td>
      <td align="right">181</td>
      <td align="right">51</td>
      <td align="right">0.00</td>
      <td align="right">69.06</td>
      <td align="right">29.57</td>
      <td align="right">0.04</td>
    </tr>
    <tr>
      <td>arbiter</td>
      <td align="right">2468</td>
      <td align="right">18</td>
      <td align="right">10.29</td>
      <td align="right">9.33</td>
      <td align="right">2.00</td>
      <td align="right">2467</td>
      <td align="right">18</td>
      <td align="right">0.00</td>
      <td align="right">10.36</td>
      <td align="right">9.39</td>
      <td align="right">0.41</td>
    </tr>
    <tr>
      <td>bar</td>
      <td align="right">448</td>
      <td align="right">4</td>
      <td align="right">14.29</td>
      <td align="right">12.50</td>
      <td align="right">0.23</td>
      <td align="right">448</td>
      <td align="right">4</td>
      <td align="right">0.00</td>
      <td align="right">14.29</td>
      <td align="right">12.50</td>
      <td align="right">0.09</td>
    </tr>
    <tr>
      <td>cavlc</td>
      <td align="right">108</td>
      <td align="right">4</td>
      <td align="right">7.41</td>
      <td align="right">6.90</td>
      <td align="right">0.05</td>
      <td align="right">102</td>
      <td align="right">4</td>
      <td align="right">0.00</td>
      <td align="right">13.73</td>
      <td align="right">12.07</td>
      <td align="right">0.03</td>
    </tr>
    <tr>
      <td>ctrl</td>
      <td align="right">18</td>
      <td align="right">2</td>
      <td align="right">61.11</td>
      <td align="right">37.93</td>
      <td align="right">0.01</td>
      <td align="right">18</td>
      <td align="right">2</td>
      <td align="right">0.00</td>
      <td align="right">61.11</td>
      <td align="right">37.93</td>
      <td align="right">0.01</td>
    </tr>
    <tr>
      <td>dec</td>
      <td align="right">144</td>
      <td align="right">2</td>
      <td align="right">99.31</td>
      <td align="right">49.83</td>
      <td align="right">0.15</td>
      <td align="right">150</td>
      <td align="right">2</td>
      <td align="right">0.00</td>
      <td align="right">91.33</td>
      <td align="right">47.74</td>
      <td align="right">0.03</td>
    </tr>
    <tr>
      <td>div</td>
      <td align="right">16289</td>
      <td align="right">943</td>
      <td align="right">57.97</td>
      <td align="right">26.34</td>
      <td align="right">244.69</td>
      <td align="right">13479</td>
      <td align="right">864</td>
      <td align="right">8.38</td>
      <td align="right">65.84</td>
      <td align="right">39.44</td>
      <td align="right">15.37</td>
    </tr>
    <tr>
      <td>hyp</td>
      <td align="right">25007</td>
      <td align="right">4214</td>
      <td align="right">79.90</td>
      <td align="right">43.81</td>
      <td align="right">753.49</td>
      <td align="right">33947</td>
      <td align="right">4194</td>
      <td align="right">0.47</td>
      <td align="right">31.46</td>
      <td align="right">23.70</td>
      <td align="right">34.32</td>
    </tr>
    <tr>
      <td>i2c</td>
      <td align="right">283</td>
      <td align="right">5</td>
      <td align="right">25.09</td>
      <td align="right">19.83</td>
      <td align="right">0.11</td>
      <td align="right">283</td>
      <td align="right">4</td>
      <td align="right">20.00</td>
      <td align="right">22.83</td>
      <td align="right">18.59</td>
      <td align="right">0.05</td>
    </tr>
    <tr>
      <td>int2float</td>
      <td align="right">44</td>
      <td align="right">3</td>
      <td align="right">15.91</td>
      <td align="right">13.73</td>
      <td align="right">0.01</td>
      <td align="right">42</td>
      <td align="right">3</td>
      <td align="right">0.00</td>
      <td align="right">23.81</td>
      <td align="right">19.23</td>
      <td align="right">0.01</td>
    </tr>
    <tr>
      <td>log2</td>
      <td align="right">6412</td>
      <td align="right">77</td>
      <td align="right">29.43</td>
      <td align="right">20.76</td>
      <td align="right">32.58</td>
      <td align="right">6249</td>
      <td align="right">71</td>
      <td align="right">7.79</td>
      <td align="right">28.32</td>
      <td align="right">21.61</td>
      <td align="right">4.98</td>
    </tr>
    <tr>
      <td>max</td>
      <td align="right">571</td>
      <td align="right">45</td>
      <td align="right">36.60</td>
      <td align="right">25.75</td>
      <td align="right">0.39</td>
      <td align="right">556</td>
      <td align="right">44</td>
      <td align="right">2.22</td>
      <td align="right">38.25</td>
      <td align="right">27.67</td>
      <td align="right">0.13</td>
    </tr>
    <tr>
      <td>mem_ctrl</td>
      <td align="right">9773</td>
      <td align="right">30</td>
      <td align="right">27.79</td>
      <td align="right">19.12</td>
      <td align="right">49.45</td>
      <td align="right">9326</td>
      <td align="right">25</td>
      <td align="right">16.67</td>
      <td align="right">28.72</td>
      <td align="right">22.34</td>
      <td align="right">4.06</td>
    </tr>
    <tr>
      <td>multiplier</td>
      <td align="right">4398</td>
      <td align="right">56</td>
      <td align="right">36.02</td>
      <td align="right">25.80</td>
      <td align="right">22.85</td>
      <td align="right">4365</td>
      <td align="right">53</td>
      <td align="right">5.36</td>
      <td align="right">36.99</td>
      <td align="right">25.50</td>
      <td align="right">4.22</td>
    </tr>
    <tr>
      <td>priority</td>
      <td align="right">184</td>
      <td align="right">31</td>
      <td align="right">15.76</td>
      <td align="right">12.38</td>
      <td align="right">0.07</td>
      <td align="right">182</td>
      <td align="right">31</td>
      <td align="right">0.00</td>
      <td align="right">16.25</td>
      <td align="right">14.79</td>
      <td align="right">0.04</td>
    </tr>
    <tr>
      <td>router</td>
      <td align="right">76</td>
      <td align="right">7</td>
      <td align="right">17.11</td>
      <td align="right">14.61</td>
      <td align="right">0.01</td>
      <td align="right">81</td>
      <td align="right">7</td>
      <td align="right">0.00</td>
      <td align="right">20.99</td>
      <td align="right">16.49</td>
      <td align="right">0.01</td>
    </tr>
    <tr>
      <td>sin</td>
      <td align="right">1198</td>
      <td align="right">41</td>
      <td align="right">24.46</td>
      <td align="right">18.17</td>
      <td align="right">2.14</td>
      <td align="right">1149</td>
      <td align="right">36</td>
      <td align="right">12.20</td>
      <td align="right">28.11</td>
      <td align="right">21.62</td>
      <td align="right">0.75</td>
    </tr>
    <tr>
      <td>sqrt</td>
      <td align="right">4913</td>
      <td align="right">1051</td>
      <td align="right">28.25</td>
      <td align="right">13.97</td>
      <td align="right">11.05</td>
      <td align="right">5201</td>
      <td align="right">1024</td>
      <td align="right">2.57</td>
      <td align="right">38.26</td>
      <td align="right">18.49</td>
      <td align="right">2.10</td>
    </tr>
    <tr>
      <td>square</td>
      <td align="right">2717</td>
      <td align="right">51</td>
      <td align="right">48.25</td>
      <td align="right">32.04</td>
      <td align="right">16.51</td>
      <td align="right">2719</td>
      <td align="right">50</td>
      <td align="right">1.96</td>
      <td align="right">47.58</td>
      <td align="right">31.73</td>
      <td align="right">3.43</td>
    </tr>
    <tr>
      <td>voter</td>
      <td align="right">2228</td>
      <td align="right">20</td>
      <td align="right">25.22</td>
      <td align="right">17.33</td>
      <td align="right">4.77</td>
      <td align="right">1860</td>
      <td align="right">15</td>
      <td align="right">25.00</td>
      <td align="right">21.00</td>
      <td align="right">17.36</td>
      <td align="right">1.35</td>
    </tr>
    <tr>
      <td><strong>Average</strong></td>
      <td align="center">—</td>
      <td align="center">—</td>
      <td align="right"><strong>35.03</strong></td>
      <td align="right"><strong>22.40</strong></td>
      <td align="right"><strong>57.03</strong></td>
      <td align="center">—</td>
      <td align="center">—</td>
      <td align="right"><strong>5.13</strong></td>
      <td align="right"><strong>35.41</strong></td>
      <td align="right"><strong>23.39</strong></td>
      <td align="right"><strong>3.57</strong></td>
    </tr>
  </tbody>
</table>

</div>

</details>

### Metric Definitions

- **LUTs**: Number of LUT nodes produced by technology mapping.
- **Depth**: Mapping depth of the resulting LUT network.
- **Dual**: Percentage of LUT roots packed into dual-output physical LUTs.
- **Save**: Reduction in the LUT-area metric relative to the corresponding
  single-output LUT mapping baseline.
- **Time**: Mapping runtime in seconds.
- **G<sub>D</sub>**: Depth reduction achieved by the current method relative to
  the reference method:

<div align="center">

G<sub>D</sub> =
(D<sub>ref</sub> − D<sub>current</sub>) /
D<sub>ref</sub> × 100%

</div>

The reference results correspond to the method reported by Lu et al. (2025).

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
