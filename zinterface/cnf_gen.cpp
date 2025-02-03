#include <cstdio>
#include "../include/percy/percy.hpp"
#include "../include/percy/spec.hpp"
#include <chrono>
#include "../syrup/core/Dimacs.h"
#include "../glucose/utils/ParseUtils.h"
#include "../glucose/simp/SimpSolver.h"
#include "../glucose/core/Dimacs.h"
#include <vector>
#include <bitset>
#include "../syrup/utils/ParseUtils.h"
#include "cnf_gen.h"
#include "map/if/if.h"
#include "misc/vec/vecPtr.h"


#if !defined(_WIN32) && !defined(_WIN64)
#ifdef USE_GLUCOSE
#else
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

ABC_NAMESPACE_IMPL_START

using namespace percy;

void percy_link(void) {
    std::cout << "Hello!" << std::endl;
}

// Function to generate a random gates vector
std::vector<int> generateRandomGates() {
    // Base vector with the first four elements as 0
    // std::vector<int> gates = {0, 0, 0, 0};
    std::vector<int> gates = {0, 0, 0, 0, 0, 0};

    // Random number generator
    std::random_device rd;  // Seed for randomness
    std::mt19937 gen(rd()); // Mersenne Twister engine
    std::uniform_int_distribution<int> dis(5, 10); // Generate numbers between 1 and 15

    // Add 4 random numbers to the gates vector
    // for (int i = 0; i < 4; ++i) {
    for (int i = 0; i < 9; ++i) {
        gates.push_back(dis(gen));
    }
    return gates;
}

// Function to print a vector
void printVector(const std::vector<int>& vec) {
    std::cout << "Gates vector: {";
    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i];
        if (i < vec.size() - 1) std::cout << ", ";
    }
    std::cout << "}" << std::endl;
}

/// Test the generation of DIMACS output from encoded exact synthesis instances.
int percymapping_main(std::vector<std::vector<int>> nets, std::vector<int> pi, 
    std::vector<int> po, std::vector<int> gates, std::vector<std::string> tts_all)
{
    spec spec;
    // read support files from current folder
    // 1-dag setting
    std::vector<int> settings;
    //settings = spec.read_settings_txt("Synthesis_Settings_6input.txt");
    settings = { 6, 6, 3, 3, 9, 0, 0, 0, 0, 0, 0, 6, 7, 6, 8, 7, 8, 9, 10, 9, 11, 10, 11, 12, 13, 14 };
    // load dag PI/PO/Step spec
    spec.nr_in = settings[0];
    spec.net_in = pi.size();
    spec.nr_out = settings[2];
    spec.net_out = po.size();
    spec.nr_steps = settings[4];
    // 2-fanout tt
    //std::vector<std::string> tts_all;
    //tts_all = spec.read_tts("truth_tables.txt");

    std::vector<int> inv(po.size(), 0); // No inversion for outputs
    // Truth table for all 2-input operators
    std::vector<std::vector<int>> truthTable(15, std::vector<int>(4));
    for (int i = 0; i < 15; ++i) {
        std::bitset<4> binary(i + 1); // Start from 1 to match MATLAB's 0001 to 1111
        for (int j = 0; j < 4; ++j) {
            truthTable[i][j] = binary[j];
        }
    }

    // extend truth table if input number < dag input number
    tts_all = spec.extendStrings(tts_all,spec.nr_in-spec.net_in);

    // loop read in output tt
    for (auto ifanout = 0; ifanout < spec.net_out; ifanout++) {
        kitty::dynamic_truth_table tt(spec.nr_in);
        kitty::create_from_hex_string(tt, tts_all[ifanout]);
        spec[ifanout] = tt;
    }
    // load the dag and PO steps
    for (auto iedge = 5; iedge < settings.size(); iedge++) {
        if (iedge < 5 + 2*spec.nr_steps) {
            spec.dag.push_back(settings[iedge]);
        }
        else {
            spec.po_steps.push_back(settings[iedge]);
        }
    }

    // 3-reconfigurable logic operators
    spec.set_primitive(RFG);

    // printf("Generating CNF for demo nets.\n");
    chain c;
    gates = {};
    for (int ifanin = 0; ifanin < spec.nr_in; ifanin++) {
        gates.push_back(0);
    }

    spec.add_colex_clauses = true;
    spec.add_lex_func_clauses = true;

    // Synthesize it to see what the minimum number of steps is.
    auto status = synthesize(spec, c);
    // assert(status == success);
    if (!(status == success)) {
        printf("status: failure\n");
        return 0;
    }
    else {
        // printf("status: success\n");
        // ptint the synthesis results
        for (auto istep = 0; istep < spec.nr_steps; istep++) {
            int steps = c.get_nr_steps();
            if(steps != 0) {
                std::string gate_name = spec.getGateFromDec(c.get_operator(istep)._bits[0]);
                gates.push_back(c.get_operator(istep)._bits[0]);
                // printf("step %d with operator %s\n", istep, gate_name.c_str());
            }
        }
        const std::vector<int> po_index = c.get_outputs();
        for (auto ipo = 0; ipo < spec.net_out; ipo++) {
            // printf("PO %d position %d with %d inverse\n", ipo,po_index[ipo],c.is_output_inverted(po_index[ipo]));
            //printf("PO %d position %d with %d inverse\n", ipo+1,po_index[ipo]/2-1,po_index[ipo]%2);
        }
        // Print the LUT
        //printf("Final Outputs.\n");

        // define the nets for verification
        // should be the same as MCluster DAG
        std::vector<std::vector<int>> nets = {
            {1, 2, 7},
            {3, 4, 8},
            {5, 6, 9},
            {7, 8, 10},
            {7, 9, 11},
            {8, 9, 12},
            {10, 11, 13},
            {10, 12, 14},
            {11, 12, 15}
        };
        std::vector<int> pi = { 1, 2, 3, 4, 5 ,6 };
        std::vector<int> po = { 13, 14, 15 };

        for (int istep = 0; istep < c.get_nr_steps(); istep++)
        {
            std::vector<int> cc = c.get_step(istep);
            nets[istep][0] = cc[0] + 1;
            nets[istep][1] = cc[1] + 1;
            nets[istep][2] = istep + spec.nr_in + 1;
        }
        int last_number = gates.back();
        if (last_number != 0) {
            spec.printLUT(pi, po, inv, nets, gates, truthTable, 0);
        }
        // printf("\n");
    }
    return 1;
}

std::vector<int> DAGPis(const std::vector<std::vector<int>>& nets)
{
    // Set to store numbers that appear in the third column (last column)
    std::unordered_set<int> lastColumnNumbers;
    for (const auto& net : nets) {
        lastColumnNumbers.insert(net[2]);  // The third column (index 2)
    }
    // Set to store numbers from the first and second columns
    std::unordered_set<int> resultSet;
    // Collect numbers from the first and second columns, excluding those in the last column
    for (const auto& net : nets) {
        if (lastColumnNumbers.find(net[0]) == lastColumnNumbers.end()) {
            resultSet.insert(net[0]);  // Add first column number if not in the last column
        }
        if (lastColumnNumbers.find(net[1]) == lastColumnNumbers.end()) {
            resultSet.insert(net[1]);  // Add second column number if not in the last column
        }
    }
    // Convert resultSet to a vector and return it
    return std::vector<int>(resultSet.begin(), resultSet.end());
}

void replaceNotInLastColumn(std::vector<std::vector<int>>& result) {
    std::unordered_set<int> lastColumnNumbers;
    // Step 1: Collect all unique numbers from the last column
    for (const auto& row : result) {
        if (!row.empty()) {
            lastColumnNumbers.insert(row.back());
        }
    }
    // Step 2: Replace numbers not in the last column with 0
    for (auto& row : result) {
        for (auto& val : row) {
            if (lastColumnNumbers.find(val) == lastColumnNumbers.end()) {
                continue;
            } else {
                val = val + 1000000;
            }
        }
    }
}

// Function to convert Vec_Ptr_t to std::vector<void*>
std::vector<std::vector<int>> ABCdag2Percy(Vec_Ptr_t* vecDAG) {
    std::vector<std::vector<int>> result;
    // Iterate over each row in the vecDAG
    for (int i = 0; i < Vec_PtrSize(vecDAG); i++) {
        // Get the current row (which is a Vec_Ptr_t structure)
        Vec_Ptr_t* vRow = (Vec_Ptr_t*)Vec_PtrEntry(vecDAG, i);
        // Create a vector to hold the current row's elements
        std::vector<int> row;
        // Iterate over each element in the row (vRow)
        for (int j = 0; j < Vec_PtrSize(vRow); j++) {
            // Get the pointer to the integer from the row
            int* pNum = (int*)Vec_PtrEntry(vRow, j);
            // Print for debugging
            // printf("netDAG[%d][%d] = %d\n", i, j, *pNum);
            // Add the value to the current row
            row.push_back(*pNum);
        }
        // Add the populated row to the result
        result.push_back(row);
    }

    // Step 1: Remove rows that contain 0
    result.erase(std::remove_if(result.begin(), result.end(), [](const std::vector<int>& row) {
        return std::find(row.begin(), row.end(), 0) != row.end();
    }), result.end());

    // collect PIs in current cut
    replaceNotInLastColumn(result);

    int zeroCount = 0;
    // Step 1: Count the total number of zeros in the entire 2D vector
    for (const auto& row : result) {
        for (int num : row) {
            if (num == 0) {
                zeroCount++;
            }
        }
    }
    // Step 2: Modify the vector
    int zeroReplacement = 1; // Start replacing zeros from 1 upwards
    for (auto& row : result) {
        for (int& num : row) {
            if (num > 0) {
                num += zeroCount;  // Increase numbers larger than 0 by zeroCount
            } else if (num == 0) {
                num = zeroReplacement;  // Replace 0 with the current replacement number
                zeroReplacement++;  // Increment the replacement number for the next zero
            }
        }
    }

    // Step 2: Extract all unique numbers, but skip 0
    std::set<int> unique_numbers;
    for (const auto& row : result) {
        unique_numbers.insert(row.begin(), row.end());
    }
    // Step 3: Map the unique numbers (excluding 0) to consecutive values starting from 1
    std::unordered_map<int, int> number_map;
    int count = 1;
    for (int num : unique_numbers) {
        number_map[num] = count++;
    }
    // Step 4: Transform the numbers in the 2D vector
    for (auto& row : result) {
        for (auto& num : row) {
            num = number_map[num];
        }
    }
    // std::reverse(result.begin(), result.end());
    return result;
}

// Function to convert Vec_Ptr_t to std::vector<void*>
std::vector<std::vector<int>> ABCop2Percy(Vec_Ptr_t* vecOP, Vec_Ptr_t* vecDAG) {
    std::vector<std::vector<int>> result;
    // Iterate over each row in the vecDAG
    for (int i = 0; i < Vec_PtrSize(vecOP); i++) {
        // Get the current row (which is a Vec_Ptr_t structure)
        Vec_Ptr_t* vRow = (Vec_Ptr_t*)Vec_PtrEntry(vecOP, i);
        Vec_Ptr_t* vDAG = (Vec_Ptr_t*)Vec_PtrEntry(vecDAG, i);
        // Create a vector to hold the current row's elements
        int* vDAG1 = (int*)Vec_PtrEntry(vDAG, 1);
        int* vDAG2 = (int*)Vec_PtrEntry(vDAG, 1);
        std::vector<int> row;
        if (* vDAG1 == 0 && * vDAG2 == 0) {
            continue;
        } else {
            // Iterate over each element in the row (vRow)
            for (int j = 0; j < Vec_PtrSize(vRow); j++) {
                // Get the pointer to the integer from the row
                int* pNum = (int*)Vec_PtrEntry(vRow, j);
                // Print for debugging
                // printf("netDAG[%d][%d] = %d\n", i, j, *pNum);
                // Add the value to the current row
                row.push_back(*pNum);
            }
        }
        // Add the populated row to the result
        result.push_back(row);
    }
    return result;
}

std::vector<int> DAGPos(const std::vector<std::vector<int>>& nets)
{
    // Set to store numbers that appear in the first or second columns
    std::unordered_set<int> firstTwoColumnsNumbers;
    for (const auto& net : nets) {
        firstTwoColumnsNumbers.insert(net[0]);  // The first column (index 0)
        firstTwoColumnsNumbers.insert(net[1]);  // The second column (index 1)
    }
    // Set to store numbers from the third column that are not in the first or second columns
    std::unordered_set<int> resultSet;
    // Collect numbers from the third column that are not in the first or second columns
    for (const auto& net : nets) {
        if (firstTwoColumnsNumbers.find(net[2]) == firstTwoColumnsNumbers.end()) {
            resultSet.insert(net[2]);  // Add third column number if not in the first or second columns
        }
    }
    // Convert resultSet to a vector and return it
    return std::vector<int>(resultSet.begin(), resultSet.end());
}

std::vector<int> ABCgate2Percy(std::vector<std::vector<int>> OPs, std::vector<int> PI)
{
    std::vector<int> gates;
    gates.insert(gates.end(), PI.size(), 0);
    // Loop through each row in OPs
    for (int i = 0; i < OPs.size(); i++) {
        std::vector<int> row = OPs[i];
        if (row == std::vector<int>{0, 0, 0}) {
            gates.push_back(8);
        } else if (row == std::vector<int>{0, 0, 1}) {
            gates.push_back(7);
        } else if (row == std::vector<int>{0, 1, 0}) {
            gates.push_back(4);
        } else if (row == std::vector<int>{0, 1, 1}) {
            gates.push_back(13);
        } else if (row == std::vector<int>{1, 0, 0}) {
            gates.push_back(2);
        } else if (row == std::vector<int>{1, 0, 1}) {
            gates.push_back(11);
        } else if (row == std::vector<int>{1, 1, 0}) {
            gates.push_back(1);
        } else if (row == std::vector<int>{1, 1, 1}) {
            gates.push_back(14);
        }
    }
    // std::reverse(gates.begin()+PI.size(), gates.end());
    return gates;
}

// process the nets and gates for the percy mapping standard format
std::pair<std::vector<std::vector<int>>, std::vector<int>> processGatesAndNets(
    const std::vector<std::vector<int>>& nets,
    const std::vector<int>& gates,
    const std::vector<int>& PI)
{
    // Create new_gate1 and new_gate2 based on the provided gates
    std::vector<int> new_gate1(gates.end() - PI.size() + 1, gates.end());
    std::vector<int> new_gate2(gates.begin(), gates.end() - PI.size() + 1);

    // Copy nets to avoid modifying the original one
    std::vector<std::vector<int>> nets_copy = nets;
    int num_rows = nets_copy.size();

    // Check if new_gate1 size matches the number of rows in nets
    if (new_gate1.size() == num_rows) {
        // Add new_gate1 as the last column of nets
        for (int i = 0; i < num_rows; ++i) {
            nets_copy[i].push_back(new_gate1[i]);
        }
    }

    // Sort nets_copy based on the third column (index 2)
    std::sort(nets_copy.begin(), nets_copy.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
        return a[2] < b[2];  // Compare based on the third column (index 2)
        });

    // Extract the first three columns as new_nets and the last column as new_gates
    std::vector<std::vector<int>> new_nets;
    std::vector<int> new_gates;
    for (const auto& row : nets_copy) {
        // Extract the first three columns for new_nets
        new_nets.push_back({ row[0], row[1], row[2] });
        // Extract the last column for new_gates
        new_gates.push_back(row[3]);
    }
    // Append new_gates to new_gate2
    new_gate2.insert(new_gate2.end(), new_gates.begin(), new_gates.end());
    // Return new_nets and new_gate2 as a pair
    return { new_nets, new_gate2};
}

int percy_map(If_Cut_t * pCut)
{
    spec spec;
    Vec_Ptr_t * vecOP = pCut->DagOP;
    Vec_Ptr_t * vecDAG = pCut->netDAG;
    // Convert to std::vector<void*>
    std::vector<std::vector<int>>  nets = ABCdag2Percy(vecDAG);
    std::vector<int> PI = DAGPis(nets);
    std::sort(PI.begin(), PI.end());
    std::vector<int> PO = DAGPos(nets);
    std::sort(PO.begin(), PO.end());
    // OP information
    std::vector<std::vector<int>>  OPs = ABCop2Percy(vecOP, vecDAG);
    // gates type
    std::vector<int>  gates = ABCgate2Percy(OPs, PI);
    // generate truth table
    std::vector<int> inv(PO.size(), 0);
    // Truth table for all 2-input operators
    std::vector<std::vector<int>> truthTable(15, std::vector<int>(4));
    for (int i = 0; i < 15; ++i) {
        std::bitset<4> binary(i + 1); // Start from 1 to match MATLAB's 0001 to 1111
        for (int j = 0; j < 4; ++j) {
            truthTable[i][j] = binary[j];
        }
    }
    // process the info to standard pecy format
    //auto resultcomb = processGatesAndNets(nets, gates, PI);
    //std::vector<std::vector<int>> new_nets = resultcomb.first;
    //std::vector<int> new_gate2 = resultcomb.second;

    std::vector<int> new_gate1(gates.begin() + PI.size(), gates.end());
    std::vector<int> new_gate2(gates.begin(), gates.begin() + PI.size());

    int num_rows = nets.size();
    // Check if new_gate1 size matches the number of rows in nets
    if (new_gate1.size() == num_rows) {
        // Add new_gate1 as the last column of nets
        for (int i = 0; i < num_rows; ++i) {
            nets[i].push_back(new_gate1[i]);
        }
    }
    std::sort(nets.begin(), nets.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
        return a[2] < b[2];  // Compare based on the third column (index 2)
        });

    // Extract the first three columns as new_nets and the last column as new_gates
    std::vector<std::vector<int>> new_nets;
    std::vector<int> new_gates;
    for (const auto& row : nets) {
        // Extract the first three columns for new_nets
        new_nets.push_back({ row[0], row[1], row[2] });
        // Extract the last column for new_gates
        new_gates.push_back(row[3]);
    }
    new_gate2.insert(new_gate2.end(), new_gates.begin(), new_gates.end());

    std::vector<std::string> tts_all = spec.printLUT(PI, PO, inv, new_nets, new_gate2, truthTable,0);
    //printf("Number of PIs: %lu\n",PI.size());
    int percy_result = 0;
    if (new_nets.size() < 5) {return percy_result = 1;}
    // here assuming the fanin/fanout limit are 6/3
    if (PI.size() <= 6 && PO.size() <= 3) {
        percy_result = percymapping_main(new_nets, PI, PO, new_gate2, tts_all);
    }
    else {
        percy_result = 0;
    }
    return percy_result;
    // pCut->CutTT = 'q';
}

ABC_NAMESPACE_IMPL_END

#ifdef __cplusplus
}
#endif