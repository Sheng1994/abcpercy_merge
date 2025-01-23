#pragma once

#include <chrono>
#include <vector>
#include "tt_utils.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include "../../kitty/include/kitty/kitty.hpp"
#include "../../kitty/include/kitty/print.hpp"
#include "../../syrup/mtl/Vec.h"
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <bitset>
#include <sstream>
#include <cmath>

#pragma GCC diagnostic pop

namespace percy
{
    const int MAX_STEPS = 20; /// The maximum number of steps we'll synthesize
    const int MAX_FANIN =  5; /// The maximum number of fanins per step we'll synthesize

    /// The various methods types of synthesis supported by percy.
    enum SynthMethod
    {
        SYNTH_STD,
        SYNTH_STD_CEGAR,
        SYNTH_FENCE,
        SYNTH_FENCE_CEGAR,
        SYNTH_DAG,
        SYNTH_FDAG,
        SYNTH_TOTAL
    };

    const char * const SynthMethodToString[SYNTH_TOTAL] =
    {
        "SYNTH_STD",
        "SYNTH_STD_CEGAR",
        "SYNTH_FENCE",
        "SYNTH_FENCE_CEGAR",
        "SYNTH_DAG",
        "SYNTH_FDAG",
    };

    enum EncoderType
    {
        ENC_SSV,
        ENC_MSV,
        ENC_DITT,
        ENC_FENCE,
        ENC_DAG,
        ENC_TOTAL
    };

    const char* const EncoderTypeToString[ENC_TOTAL] =
    {
        "ENC_SSV",
        "ENC_MSV",
        "ENC_DITT",
        "ENC_FENCE",
        "ENC_DAG",
    };

    enum SolverType
    {
        SLV_BSAT2,
        SLV_CMSAT,
        SLV_GLUCOSE,
        SLV_SATOKO,
        SLV_TOTAL,
    };

    const char * const SolverTypeToString[SLV_TOTAL] =
    {
        "SLV_BSAT2",
        "SLV_CMSAT",
        "SLV_GLUCOSE",
        "SLV_SATOKO",
    };

    enum Primitive
    {
        MAJ,
        AIG,
        RFG
    };

    /// Used to gather data on synthesis experiments.
    struct synth_stats
    {
        double overhead = 0;
        double total_synth_time = 0;
        double time_to_first_synth = 0;
        int nr_success = 0;
        int nr_timeouts = 0;
        int64_t sat_time = 0;   ///< How much time was spent on UNSAT formulae (in us)
        int64_t unsat_time = 0; ///< How much time was spent on SAT formulae (in us)
        int64_t synth_time = 0; ///< How much time was spent on SAT formulae (in us)
        int nr_vars = 0;
        int nr_clauses = 0;
    };

    class spec
    {
        protected:
            int capacity; ///< Maximum number of output functions this specification can support
            std::vector<kitty::dynamic_truth_table> functions; ///< Functions to synthesize
            std::vector<kitty::dynamic_truth_table> dc_masks; ///< Indicates which input combinations we don't care about
            std::vector<char> dc_functions; ///< Determines for which functions we look at the DC mask
            std::vector<int> triv_functions; ///< Trivial outputs
            std::vector<int> synth_functions; ///< Nontrivial outputs
            std::vector<kitty::dynamic_truth_table> compiled_primitives; ///< Collection of concrete truth tables induced by primitives

        public:
            int fanin = 2; ///< The fanin of the Boolean chain steps
            int nr_in; ///< The inputs of the chain we want to synthesize
            int tt_size; ///< Size of the truth tables to synthesize (in nr. of bits)
            int nr_steps; ///< The number of Boolean operators to use
            int initial_steps = 1; ///< The number of steps from which to start synthesis
            int verbosity = 0; ///< Verbosity level for debugging purposes
            uint64_t out_inv; ///< Is 1 at index i if output i must be inverted
            uint64_t triv_flag; ///< Is 1 at index i if output i is constant zero or one or a projection.
            int nr_triv; ///< Number of trivial output functions
            int nr_nontriv; ///< Number of non-trivial output functions

            /*************************added variables*******************************/
            int net_in; ///< net input number
            int net_out; ///< netlist fanout number limit
            int nr_out; ///< dag fanout number
            std::vector<int> dag; ///vector for dag connection
            std::vector<int> po_steps; ///certain steps as POs
            /*************************added variables*******************************/

            bool add_nontriv_clauses = true; ///< Symmetry break: do not allow trivial operators
            bool add_alonce_clauses = true; ///< Symmetry break: all steps must be used at least once
            bool add_noreapply_clauses = true; ///< Symmetry break: no re-application of operators
            bool add_colex_clauses = true; ///< Symmetry break: order step fanins co-lexicographically
            bool add_lex_func_clauses = true; ///< Symmetry break: order step operators co-lexicographically
            bool add_symvar_clauses = true; ///< Symmetry break: impose order on symmetric variables
            bool add_lex_clauses = false; ///< Symmetry break: order step fanins lexicographically

            /// Limit on the number of SAT conflicts. Zero means no limit.
            int conflict_limit = 0;

            /// Constructs a spec with one output
            spec()
            {
                set_nr_out(1);
            }

            /// Constructs a spec with nr_out outputs.
            spec(int nr_out)
            {
                set_nr_out(nr_out);
            }

            void
            set_nr_out(int n)
            {
                capacity = n;
                functions.resize(n);
                dc_masks.resize(n);
                dc_functions.resize(n);
                triv_functions.resize(n);
                synth_functions.resize(n);
            }

            int get_nr_in() const { return functions[0].num_vars(); }
            int get_tt_size() const { return tt_size; }
            int get_nr_out() const { return capacity; }

            /// Normalizes outputs by converting them to normal functions. Also
            /// checks for trivial outputs, such as constant functions or
            /// projections. This determines which of the specified functions
            /// need to be synthesized.  This function expects the following
            /// invariants to hold:
            /// 1. The number of input variables has been set.
            /// 2. The number of output variables has been set.
            /// 3. The functions requested to be synthesized have been set.
            void
            preprocess(void)
            {
                assert(!add_colex_clauses || !add_lex_clauses);

                // Verify that all functions have the same number of variables
                const auto num_vars = functions[0].num_vars();
                for (int i = 1; i < capacity; i++) {
                    if (functions[i].num_vars() != num_vars) {
                        assert(false);
                        exit(1);
                    }
                }

                nr_in = functions[0].num_vars();
                tt_size = (1 << functions[0].num_vars()) - 1;

                if (verbosity) {
                    printf("\n");
                    printf("========================================"
                           "========================================\n");
                    printf("  Pre-processing for %s:\n", capacity > 1 ?
                            "functions" : "function");
                    for (int h = 0; h < capacity; h++) {
                        printf("  ");
                        kitty::print_binary(functions[h], std::cout);
                        printf("\n");
                    }
                    printf("========================================"
                            "========================================\n");
                    printf("  SPEC:\n");
                    printf("\tnr_in=%d\n", functions[0].num_vars());
                    printf("\tnr_out=%d\n", capacity);
                    printf("\ttt_size=%d\n", tt_size);
                }

                assert((!add_colex_clauses && !add_lex_clauses) ||
                        (add_colex_clauses != add_lex_clauses));

                // Detect any trivial outputs.
                nr_triv = 0;
                nr_nontriv = 0;
                out_inv = 0;
                triv_flag = 0;
                for (int h = 0; h < capacity; h++) {
                    if (is_const0(functions[h])) {
                        triv_flag |= (1 << h);
                        triv_functions[nr_triv++] = 0;
                    } else if (is_const0(~(functions[h]))) {
                        triv_flag |= (1 << h);
                        triv_functions[nr_triv++] = 0;
                        out_inv |= (1 << h);
                    } else {
                        auto tt_var = functions[0].construct();
                        for (int i = 0; i < get_nr_in(); i++) {
                            create_nth_var(tt_var, i);
                            if (functions[h] == tt_var) {
                                triv_flag |= (1 << h);
                                triv_functions[nr_triv++] = i+1;
                                break;
                            } else if (functions[h] == ~(tt_var)) {
                                triv_flag |= (1 << h);
                                triv_functions[nr_triv++] = i+1;
                                out_inv |= (1 << h);
                                break;
                            }
                        }
                        // Even when the output is not trivial, we still need
                        // to ensure that it's normal.
                        if (!((triv_flag >> h) & 1)) {
                            if (!is_normal(functions[h])) {
                                out_inv |= (1 << h);
                            }
                            synth_functions[nr_nontriv++] = h;
                        }
                    }
                }

                if (verbosity) {
                    for (int h = 0; h < capacity; h++) {
                        if ((triv_flag >> h) & 1) {
                            printf("  Output %d is trivial\n", h+1);
                        }
                        if ((out_inv >> h) & 1) {
                            printf("  Inverting output %d\n", h+1);
                        }
                    }
                    printf("  Trivial outputs=%d\n", nr_triv);
                    printf("  Non-trivial outputs=%d\n", capacity - nr_triv);
                    printf("========================================"
                            "========================================\n");
                    printf("\n");
                }
            }

            kitty::dynamic_truth_table&
            operator[](std::size_t idx)
            {
                if (static_cast<int>(idx) >= capacity) {
                    set_nr_out(idx + 1);
                }
                return functions[idx];
            }

            const kitty::dynamic_truth_table&
            operator[](std::size_t idx) const
            {
                assert (static_cast<int>(idx) < capacity);
                return functions[idx];
            }

            template<class TT>
            void set_output(int i, const TT& tt)
            {
                assert(i < capacity);
                functions[i] = tt;
            }

            void set_dont_care(std::size_t f_idx, kitty::dynamic_truth_table dc_mask)
            {
                dc_functions[f_idx] = 1;
                dc_masks[f_idx] = dc_mask;
            }

            void clear_dont_care(std::size_t f_idx)
            {
                dc_functions[f_idx] = 0;
            }

            bool is_dont_care(std::size_t f_idx, int dc_idx) const
            {
                return dc_functions[f_idx] && kitty::get_bit(dc_masks[f_idx], dc_idx);
            }

            bool has_dc_mask(std::size_t f_idx) const
            {
                return dc_functions[f_idx];
            }

            const kitty::dynamic_truth_table& get_dc_mask(std::size_t f_idx) const
            {
                return dc_masks[f_idx];
            }

            int
            triv_func(int i) const
            {
                assert(i < capacity);
                return triv_functions[i];
            }

            int
            synth_func(int i) const
            {
                assert(i < capacity);
                return synth_functions[i];
            }

            void set_primitive(Primitive primitive)
            {
                compiled_primitives.clear();
                kitty::dynamic_truth_table tt(fanin);
                std::vector<kitty::dynamic_truth_table> inputs;
                for (int i = 0; i < fanin; i++) {
                    inputs.push_back(kitty::create<kitty::dynamic_truth_table>(fanin));
                    kitty::create_nth_var(inputs[i], i);
                }
                switch (primitive) {
                case AIG:
                    tt = inputs[0] & inputs[1];
                    compiled_primitives.push_back(tt);
                    tt = ~inputs[0] & inputs[1];
                    compiled_primitives.push_back(tt);
                    tt = inputs[0] & ~inputs[1];
                    compiled_primitives.push_back(tt);
                    tt = inputs[0] | inputs[1];
                    compiled_primitives.push_back(tt);
                    break;
                case MAJ:
                    kitty::create_majority(tt);
                    compiled_primitives.push_back(tt);
                    break;
                case RFG:
                    //std::vector<std::string> operators = read_tts("primitives.txt");
                    std::vector<std::string> operators = {
                    "7", "2", "4", "6", "8", "A", "C", "E",
                    "2", "2", "2", "2", "2", "2", "2"};
                    int k = 0, nr_tts;
                    nr_tts = std::stoi(operators[0]);
                    for (auto ip = 0; ip < nr_tts; ip++) {
                        int nr_ttin;
                        nr_ttin = std::stoi(operators[nr_tts + ip+1]);
                        kitty::dynamic_truth_table tt(nr_ttin);
                        kitty::create_from_hex_string(tt, operators[1+k++]);
                        compiled_primitives.push_back(tt);
                    }
                }
            }

            bool is_primitive_set() const
            {
                return compiled_primitives.size() > 0;
            }

            const std::vector<kitty::dynamic_truth_table>&
            get_compiled_primitives() const
            {
                return compiled_primitives;
            }

            void clear_primitive()
            {
                compiled_primitives.clear();
            }

            /*************************added functions*******************************/
            // Function to calculate the final index (sel_index)
            static int calculate_sel_index(int i, int j, int k, int faninnum, std::vector<int>  map_index) {

                // if (j == 0 && k == 0) { return 0;}
                int offset1=0, offset2=0, sel_index=0;
                i = i + faninnum;
                // Determine offset1
                if (i == faninnum) {
                    offset1 = 0;
                } else {
                    for (int istep = 0; istep < i - faninnum; istep++) {
                        offset1 = offset1 + map_index[istep];
                    }
                }

                // Determine offset2
                if (k != 0) {
                    for (int n = 1; n < k; n++) {
                        offset2 += n;
                    }
                }

                // Calculate final index
                sel_index = offset1 + offset2 + j;

                return sel_index;
            }

            // Reads truth tables from file
            std::vector<std::string> read_tts(const char* const filename)
            {
                std::ifstream fin;
                std::vector<std::string> tts;
                fin.open(filename, std::ifstream::in);
                if (!fin) {
                    fprintf(stderr, "Error: unable to open truth table file\n");
                    exit(1);
                }
                std::string str;
                while (getline(fin, str)) {
                    // Remove trailing '\r' if it exists
                    if (!str.empty() && str[str.size() - 1] == '\r') {
                        str = str.substr(0, str.size() - 1);  // Remove the last character (carriage return)
                    }
                    tts.push_back(str);
                }
                fin.close();
                return tts;
            }

            // Reads dag settings from file
            std::vector<int> read_settings_txt(const char* const filename)
            {
                std::vector<int> settings;
                std::ifstream file(filename);
                if (!file.is_open()) {
                    std::cerr << "Error opening the setting file." << std::endl;
                    abort();
                }

                std::string line;
                while (std::getline(file, line)) {
                    std::istringstream ss(line);
                    int number;
                    while (ss >> number) {
                        settings.push_back(number);
                    }
                }
                /*partial_dag g;
                g.reset(6, 9);
                g.set_vertex(0, 0, 1); // nr_in + node_id
                g.set_vertex(1, 2, 3);
                g.set_vertex(2, 4, 5);
                g.set_vertex(3, 6, 7);
                g.set_vertex(4, 6, 7);
                g.set_vertex(5, 6, 8);
                g.set_vertex(6, 9, 10);
                g.set_vertex(7, 9, 10);
                g.set_vertex(8, 9, 11);*/
                return settings;
            }

            // Function to extend each string in the vector by repeating it 'times' times
            std::vector<std::string> extendStrings(const std::vector<std::string>& strVec, int times) {
                std::vector<std::string> extendedVec;  // A new vector to store the modified strings
                for (const auto& str : strVec) {
                    std::string extendedStr = str;
                    for (int i = 0; i < (1<<times)-1; ++i) {
                        extendedStr += str;  // Append the string to itself 'times - 1' more times
                    }
                    extendedVec.push_back(extendedStr);  // Add the modified string to the new vector
                }
                return extendedVec;  // Return the modified vector
            }

            // Function to get the gate type from a decimal input
            std::string getGateFromDec(int decimalInput) {
                // Map hex codes to gate names
                std::map<std::string, std::string> gateMap = {
                    {"8", "AND"},
                    {"E", "OR"},
                    {"6", "XOR"},
                    {"7", "NAND"},
                    {"1", "NOR"},
                    {"9", "XNOR"},
                    {"C", "X"},
                    {"A", "Y"},
                    {"3", "-X"},
                    {"5", "-Y"},
                    {"4", "X&-Y"},
                    {"2", "-X&Y"},
                    {"D", "-X||Y"},
                    {"B", "X||-Y"}
                };

                // Convert decimal to hexadecimal
                std::stringstream hexStream;
                hexStream << std::uppercase << std::hex << decimalInput;
                std::string hexCode = hexStream.str();

                // Find the gate type
                if (gateMap.find(hexCode) != gateMap.end()) {
                    return gateMap[hexCode];
                } else {
                    return "Invalid hex code!";
                }
            }

        // Generate all possible input combinations for the given number of inputs
        std::vector<std::vector<int>> generateInputCombinations(int inputCount) {
            int totalCombinations = pow(2, inputCount);
            std::vector<std::vector<int>> combinations(totalCombinations, std::vector<int>(inputCount, 0));

            for (int i = 0; i < totalCombinations; ++i) {
                for (int j = 0; j < inputCount; ++j) {
                    combinations[i][j] = (i >> (inputCount - 1 - j)) & 1;
                }
            }

            // Reverse each vector in the combinations
            for (auto& combination : combinations) {
                std::reverse(combination.begin(), combination.end());
            }

            return combinations;
        }

        // Evaluate a node value recursively based on its inputs and the gate truth table
        void evaluateNode(
            int node,
            const std::vector<std::vector<int>>& nets,
            std::vector<std::vector<int>>& values,
            const std::vector<int>& gates,
            const std::vector<std::vector<int>>& truthTable,
            int totalCombinations,
            std::vector<bool>& visited
        ) {
            // Get input nodes connected to the current node
            std::vector<int> inputNodes;
            for (const auto& net : nets) {
                if (net.back() == node) {
                    inputNodes.assign(net.begin(), net.end() - 1);
                    break;
                }
            }

            // Recursively evaluate inputs if not already evaluated
            for (int input : inputNodes) {
                if (values[input][0] == -1 && !visited[input]) { // Check if the node has been evaluated and is not visited
                    visited[input] = true;
                    evaluateNode(input, nets, values, gates, truthTable, totalCombinations, visited);
                    visited[input] = false; // Reset after recursion
                }
            }

            // Apply gate logic using the truth table
            int gateType = gates[node - 1]; // Adjust node index to 0-based for gates
            if (gateType < 1 || gateType > truthTable.size()) {
                std::cerr << "Error: Invalid gate type for node " << node << std::endl;
                return;
            }

            const std::vector<int>& op = truthTable[gateType - 1]; // Get corresponding truth table row
            if (inputNodes.size() < 2) {
                std::cerr << "Error: Not enough input nodes for gate evaluation at node " << node << std::endl;
                return;
            }

            // Evaluate the node values using the truth table
            for (int i = 0; i < totalCombinations; ++i) {
                int index = values[inputNodes[0]][i] + values[inputNodes[1]][i] * 2;
                values[node][i] = op[index];
            }
        }

        void printVec(const std::vector<int>& gates) {
            for (size_t i = 0; i < gates.size(); ++i) {
                std::cout << gates[i] << " ";
            }
            std::cout << std::endl;
        }

        // Print the Lookup Table (LUT) for the circuit
        std::vector<std::string> printLUT(
            const std::vector<int>& pi,
            const std::vector<int>& po,
            const std::vector<int>& inv,
            const std::vector<std::vector<int>>& nets,
            const std::vector<int>& gates,
            const std::vector<std::vector<int>>& truthTable,
            int rewrite
        ) {
            // Generate all input combinations
            std::vector<std::vector<int>> inputs = generateInputCombinations(pi.size());
            int totalCombinations = inputs.size();

            // Initialize node values with -1 (uninitialized)
            int maxNode = 0;
            for (const auto& net : nets) {
                maxNode = std::max(maxNode, net.back());
            }
            std::vector<std::vector<int>> values(maxNode + 1, std::vector<int>(totalCombinations, -1));

            // Assign primary input values
            for (size_t i = 0; i < pi.size(); ++i) {
                for (int j = 0; j < totalCombinations; ++j) {
                    values[pi[i]][j] = inputs[j][i];
                }
            }

            // Initialize a visited array to prevent infinite recursion
            std::vector<bool> visited(values.size(), false);

            // Evaluate each primary output
            for (size_t i = 0; i < po.size(); ++i) {
                evaluateNode(po[i], nets, values, gates, truthTable, totalCombinations, visited);

                // Handle inversion if required
                if (inv[i] == 1) {
                    for (int& val : values[po[i]]) {
                        val = 1 - val;
                    }
                }
            }

            // Open the output file if required
            FILE* outFile = nullptr;
            if (rewrite == 1) {
                outFile = fopen("truth_tables.txt", "w");
                if (outFile == nullptr) {
                    std::cerr << "Error opening the output file!" << std::endl;
                    return;
                }
            }

            std::vector<std::string> tts_all;
            // Print LUT for each primary output
            for (size_t i = 0; i < po.size(); ++i) {
                std::string binaryString;
                for (int val : values[po[i]]) {
                    binaryString.push_back(val + '0');
                }
                reverse(binaryString.begin(), binaryString.end());

                std::stringstream hexStream;
                for (size_t j = 0; j < binaryString.size(); j += 4) {
                    std::string nibble = binaryString.substr(j, 4);
                    int nibbleValue = stoi(nibble, nullptr, 2);
                    hexStream << std::hex << std::uppercase << nibbleValue;
                }

                printf("po%d = %s\n", i + 1, hexStream.str().c_str());
                //printVec(gates);

                // Write output to file if required
                if (rewrite == 1 && outFile != nullptr) {
                    fprintf(outFile, "%s\n", hexStream.str().c_str());
                }

                std::string hexValue;
                while (hexStream >> hexValue) {
                    tts_all.push_back(hexValue);
                }
            }

            // Close the file after writing
            if (rewrite == 1 && outFile != nullptr) {
                fclose(outFile);
            }
            return tts_all;
        }
    };

}