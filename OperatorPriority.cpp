#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>

using namespace std;

// Grammar storage
struct Grammar {
    string productions[100][2]; // Store split grammar rules
    int productionCount = 0; // Count of grammar production rules
};

// Sets for operator precedence analysis
struct PrecedenceSets {
    string firstVT[20][2]; // FIRSTVT sets
    string lastVT[20][2]; // LASTVT sets
    int nonTerminalCount = 0; // Count of non-terminals
};

// Parsing table and analysis data
struct ParsingData {
    char precedenceTable[100][100]; // Operator precedence relationship table
    string analysisTable[100][5]; // Analysis process steps
    int terminalCount = 0; // Count of terminals in precedence table
    int currentStep = 1; // Current analysis step
    int matchedProduction = 0; // Index of matched production during reduction
};

// Input strings
string inputGrammar; // Store input grammar
string stackContent = "#"; // Stack content (starts with #)
string remainingInput; // Remaining input string

// Helper functions
bool isNonTerminal(char c) {
    return (c >= 'A' && c <= 'Z');
}

bool containsTerminal(int count, const string &str) {
    // Check if the last 'count' characters contain any terminal
    bool found = false;
    for (int j = 0; j < count; j++) {
        if (!isNonTerminal(str[str.length() - j - 1])) {
            found = true;
        }
    }
    return found;
}

void removeFirstChar(string &str) {
    if (!str.empty()) {
        str.erase(0, 1);
    }
}

// Grammar handling functions
void splitProduction(const string &rule, Grammar &grammar) {
    // Split grammar rule into productions without '|' symbols
    for (int i = 3; i < rule.length(); ++i) {
        grammar.productions[grammar.productionCount][0] = rule[0];
        string rightSide;

        while (i < rule.length() && rule[i] != '|') {
            rightSide += rule[i];
            i++;
        }

        grammar.productions[grammar.productionCount][1] = rightSide;
        grammar.productionCount++;
    }
}

void readGrammarFromFile(const string &filePath, Grammar &grammar) {
    fstream file(filePath);
    string line;
    vector<string> rules;

    while (getline(file, line)) {
        rules.push_back(line);
    }

    cout << "Input Grammar:" << endl;
    for (const auto &rule: rules) {
        cout << rule << endl;
        splitProduction(rule, grammar);
    }
}

// FIRSTVT and LASTVT set construction
int findNonTerminalIndex(char nonTerminal, const PrecedenceSets &sets) {
    for (int i = 0; i < sets.nonTerminalCount; ++i) {
        if (sets.firstVT[i][0][0] == nonTerminal) {
            return i;
        }
    }
    return -1;
}

void extractNonTerminals(const Grammar &grammar, PrecedenceSets &sets) {
    // Extract all non-terminals from the grammar
    for (int i = 0; i < grammar.productionCount; ++i) {
        char nonTerminal = grammar.productions[i][0][0];
        bool alreadyExists = false;

        for (int j = 0; j < sets.nonTerminalCount; ++j) {
            if (sets.firstVT[j][0][0] == nonTerminal) {
                alreadyExists = true;
                break;
            }
        }

        if (!alreadyExists) {
            sets.firstVT[sets.nonTerminalCount][0] = nonTerminal;
            sets.lastVT[sets.nonTerminalCount][0] = nonTerminal;
            sets.nonTerminalCount++;
        }
    }
}

void addToFirstVT(char terminal, int nonTerminalIndex, PrecedenceSets &sets) {
    // Add terminal to FIRSTVT set if not already present
    if (isNonTerminal(terminal)) return;

    for (char c: sets.firstVT[nonTerminalIndex][1]) {
        if (c == terminal) return; // Already exists
    }

    sets.firstVT[nonTerminalIndex][1] += terminal;
}

void addToFirstVT(const string &terminals, int nonTerminalIndex, PrecedenceSets &sets) {
    // Add multiple terminals to FIRSTVT set
    for (char terminal: terminals) {
        if (!isNonTerminal(terminal)) {
            addToFirstVT(terminal, nonTerminalIndex, sets);
        }
    }
}

void addToLastVT(char terminal, int nonTerminalIndex, PrecedenceSets &sets) {
    // Add terminal to LASTVT set if not already present
    if (isNonTerminal(terminal)) return;

    for (char c: sets.lastVT[nonTerminalIndex][1]) {
        if (c == terminal) return; // Already exists
    }

    sets.lastVT[nonTerminalIndex][1] += terminal;
}

void addToLastVT(const string &terminals, int nonTerminalIndex, PrecedenceSets &sets) {
    // Add multiple terminals to LASTVT set
    for (char terminal: terminals) {
        if (!isNonTerminal(terminal)) {
            addToLastVT(terminal, nonTerminalIndex, sets);
        }
    }
}

string computeFirstVT(char nonTerminal, int index, const Grammar &grammar, PrecedenceSets &sets) {
    // Compute FIRSTVT set for a non-terminal
    for (int i = 0; i < grammar.productionCount; ++i) {
        if (nonTerminal != grammar.productions[i][0][0]) continue;

        const string &rightSide = grammar.productions[i][1];

        // Rule 1: If P->a... then a is in FIRSTVT(P)
        if (!rightSide.empty() && !isNonTerminal(rightSide[0])) {
            addToFirstVT(rightSide[0], index, sets);
        }
        // Rule 2: If P->Q... and Q is non-terminal, add FIRSTVT(Q) to FIRSTVT(P)
        else if (!rightSide.empty() && isNonTerminal(rightSide[0]) && nonTerminal != rightSide[0]) {
            int qIndex = findNonTerminalIndex(rightSide[0], sets);
            string qFirstVT = computeFirstVT(rightSide[0], qIndex, grammar, sets);
            addToFirstVT(qFirstVT, index, sets);
        }

        // Rule 3: If P->Qa... and a is terminal, add a to FIRSTVT(P)
        if (rightSide.length() >= 2 && isNonTerminal(rightSide[0]) && !isNonTerminal(rightSide[1])) {
            addToFirstVT(rightSide[1], index, sets);
        }
    }

    return sets.firstVT[index][1];
}

string computeLastVT(char nonTerminal, int index, const Grammar &grammar, PrecedenceSets &sets) {
    // Compute LASTVT set for a non-terminal
    for (int i = 0; i < grammar.productionCount; ++i) {
        if (nonTerminal != grammar.productions[i][0][0]) continue;

        const string &rightSide = grammar.productions[i][1];
        int rightLen = rightSide.length();

        // Rule 1: If P->...a then a is in LASTVT(P)
        if (!rightSide.empty() && !isNonTerminal(rightSide[rightLen - 1])) {
            addToLastVT(rightSide[rightLen - 1], index, sets);
        }
        // Rule 2: If P->...Q and Q is non-terminal, add LASTVT(Q) to LASTVT(P)
        else if (!rightSide.empty() && isNonTerminal(rightSide[rightLen - 1]) && nonTerminal != rightSide[
                     rightLen - 1]) {
            int qIndex = findNonTerminalIndex(rightSide[rightLen - 1], sets);
            string qLastVT = computeLastVT(rightSide[rightLen - 1], qIndex, grammar, sets);
            addToLastVT(qLastVT, index, sets);
        }

        // Rule 3: If P->...aQ and a is terminal, add a to LASTVT(P)
        if (rightLen >= 2 && !isNonTerminal(rightSide[rightLen - 2]) && isNonTerminal(rightSide[rightLen - 1])) {
            addToLastVT(rightSide[rightLen - 2], index, sets);
        }
    }

    return sets.lastVT[index][1];
}

void computeAllFirstVTSets(const Grammar &grammar, PrecedenceSets &sets) {
    for (int i = 0; i < sets.nonTerminalCount; i++) {
        computeFirstVT(sets.firstVT[i][0][0], i, grammar, sets);
    }
}

void computeAllLastVTSets(const Grammar &grammar, PrecedenceSets &sets) {
    for (int i = 0; i < sets.nonTerminalCount; i++) {
        computeLastVT(sets.lastVT[i][0][0], i, grammar, sets);
    }
}

// Precedence table functions
int findTerminalIndex(char terminal, const ParsingData &data) {
    for (int i = 1; i <= data.terminalCount; ++i) {
        if (data.precedenceTable[i][0] == terminal) {
            return i;
        }
    }
    return -1;
}

void initPrecedenceTable(const Grammar &grammar, ParsingData &data) {
    // Initialize the operator precedence table
    data.precedenceTable[0][0] = '\\'; // First cell is a backslash

    // Add all terminals to the table
    for (int i = 0; i < grammar.productionCount; ++i) {
        const string &rightSide = grammar.productions[i][1];

        for (char c: rightSide) {
            if (!isNonTerminal(c)) {
                bool alreadyExists = false;
                for (int k = 1; k <= data.terminalCount; ++k) {
                    if (data.precedenceTable[k][0] == c) {
                        alreadyExists = true;
                        break;
                    }
                }

                if (!alreadyExists) {
                    data.terminalCount++;
                    data.precedenceTable[data.terminalCount][0] = c; // Row header
                    data.precedenceTable[0][data.terminalCount] = c; // Column header
                }
            }
        }
    }

    // Initialize all cells to empty
    for (int row = 1; row <= data.terminalCount; ++row) {
        for (int col = 1; col <= data.terminalCount; ++col) {
            data.precedenceTable[row][col] = ' ';
        }
    }
}

void buildPrecedenceTable(const Grammar &grammar, const PrecedenceSets &sets, ParsingData &data) {
    // Build the operator precedence relation table
    for (int i = 0; i < grammar.productionCount; ++i) {
        const string &rightSide = grammar.productions[i][1];

        for (int j = 0; j < rightSide.length(); ++j) {
            // Rule 1: If P->...ab... then a = b
            if (j + 1 < rightSide.length() &&
                !isNonTerminal(rightSide[j]) && !isNonTerminal(rightSide[j + 1])) {
                int rowIdx = findTerminalIndex(rightSide[j], data);
                int colIdx = findTerminalIndex(rightSide[j + 1], data);
                data.precedenceTable[rowIdx][colIdx] = '=';
            }

            // Rule 2: If P->...aQb... then a = b
            if (j + 2 < rightSide.length() &&
                !isNonTerminal(rightSide[j]) && isNonTerminal(rightSide[j + 1]) &&
                !isNonTerminal(rightSide[j + 2])) {
                int rowIdx = findTerminalIndex(rightSide[j], data);
                int colIdx = findTerminalIndex(rightSide[j + 2], data);
                data.precedenceTable[rowIdx][colIdx] = '=';
            }

            // Rule 3: If P->...aQ... then a < FIRSTVT(Q)
            if (j + 1 < rightSide.length() &&
                !isNonTerminal(rightSide[j]) && isNonTerminal(rightSide[j + 1])) {
                int ntIdx = findNonTerminalIndex(rightSide[j + 1], sets);
                int rowIdx = findTerminalIndex(rightSide[j], data);

                for (char terminal: sets.firstVT[ntIdx][1]) {
                    int colIdx = findTerminalIndex(terminal, data);
                    data.precedenceTable[rowIdx][colIdx] = '<';
                }
            }

            // Rule 4: If P->...Qa... then LASTVT(Q) > a
            if (j + 1 < rightSide.length() &&
                isNonTerminal(rightSide[j]) && !isNonTerminal(rightSide[j + 1])) {
                int ntIdx = findNonTerminalIndex(rightSide[j], sets);
                int colIdx = findTerminalIndex(rightSide[j + 1], data);

                for (char terminal: sets.lastVT[ntIdx][1]) {
                    int rowIdx = findTerminalIndex(terminal, data);
                    data.precedenceTable[rowIdx][colIdx] = '>';
                }
            }
        }
    }
}

// Analysis functions
bool canReduce(const Grammar &grammar, ParsingData &data) {
    // Check if the top of the stack can be reduced by some production
    for (int i = 0; i < grammar.productionCount; ++i) {
        int matchCount = 0;
        int stackPos = stackContent.length() - 1;
        int prodLen = grammar.productions[i][1].length();

        if (containsTerminal(prodLen, stackContent)) {
            for (int j = prodLen - 1; j >= 0 && stackPos >= 0; j--, stackPos--) {
                if ((isNonTerminal(stackContent[stackPos]) && isNonTerminal(grammar.productions[i][1][j])) ||
                    (stackContent[stackPos] == grammar.productions[i][1][j])) {
                    matchCount++;
                }
            }

            if (matchCount == prodLen) {
                data.matchedProduction = i;
                return true;
            }
        }
    }
    return false;
}

char getPrecedenceRelation(char a, char b, const ParsingData &data) {
    int rowIdx = findTerminalIndex(a, data);
    int colIdx = findTerminalIndex(b, data);
    return data.precedenceTable[rowIdx][colIdx];
}

void analyzeString(const Grammar &grammar, ParsingData &data) {
    // Initialize analysis table headers
    data.analysisTable[0][0] = "步骤";
    data.analysisTable[0][1] = "符号栈";
    data.analysisTable[0][2] = "优先关系";
    data.analysisTable[0][3] = "输入串";
    data.analysisTable[0][4] = "动作";

    remainingInput = inputGrammar;

    stringstream stepStream;
    while (true) {
        // Record current step
        stepStream.str("");
        stepStream << data.currentStep;
        data.analysisTable[data.currentStep][0] = stepStream.str();
        data.analysisTable[data.currentStep][1] = stackContent;
        data.analysisTable[data.currentStep][3] = remainingInput;

        // Find the last terminal in stack
        int lastTerminalPos = stackContent.length() - 1;
        if (isNonTerminal(stackContent[lastTerminalPos])) {
            for (int i = stackContent.length() - 1; i >= 0; i--) {
                if (!isNonTerminal(stackContent[i])) {
                    lastTerminalPos = i;
                    break;
                }
            }
        }

        // Get precedence relation between last terminal in stack and first input symbol
        char relation = getPrecedenceRelation(stackContent[lastTerminalPos], remainingInput[0], data);
        data.analysisTable[data.currentStep][2] = relation;

        if (relation == '=') {
            if (stackContent[lastTerminalPos] == '#' && remainingInput[0] == '#') {
                data.analysisTable[data.currentStep][4] = "接受";
                cout << "符合OPG" << endl;
                break;
            } else {
                data.analysisTable[data.currentStep][4] = "移进";
                stackContent += remainingInput[0];
                removeFirstChar(remainingInput);
            }
        } else if (relation == '<') {
            data.analysisTable[data.currentStep][4] = "移进";
            stackContent += remainingInput[0];
            removeFirstChar(remainingInput);
        } else if (relation == '>') {
            if (canReduce(grammar, data)) {
                data.analysisTable[data.currentStep][4] = "规约";

                // Perform reduction
                int prodIndex = data.matchedProduction;
                int prodLen = grammar.productions[prodIndex][1].length();
                stackContent.resize(stackContent.length() - prodLen);
                stackContent += grammar.productions[prodIndex][0];
            } else {
                cout << "Invalid input string!" << endl;
                exit(1);
            }
        } else {
            cout << "Error: No precedence relation defined between "
                    << stackContent[lastTerminalPos] << " and " << remainingInput[0] << endl;
            exit(1);
        }

        data.currentStep++;
    }
}

// Output functions
void printSets(const string &title, const string nonTerminal, const string &setContent) {
    cout << title << "(" << nonTerminal << ") = {";
    for (size_t i = 0; i < setContent.length(); ++i) {
        cout << "\"" << setContent[i] << "\"";
        if (i < setContent.length() - 1) cout << ", ";
    }
    cout << "}" << endl;
}

void printPrecedenceTable(const ParsingData &data) {
    cout << "Operator Precedence Table:" << endl;
    for (int i = 0; i <= data.terminalCount; ++i) {
        for (int j = 0; j <= data.terminalCount; ++j) {
            cout << data.precedenceTable[i][j] << " ";
        }
        cout << endl;
    }
}

void printGrammar(const Grammar &grammar) {
    cout << "Split Grammar Rules:" << endl;
    for (int i = 0; i < grammar.productionCount; ++i) {
        cout << grammar.productions[i][0] << "->" << grammar.productions[i][1] << endl;
    }
}

void printAnalysisProcess(const ParsingData &data) {
    ofstream outputFile("../result.txt");

    cout << "Operator Precedence Analysis Process:" << endl;
    outputFile << "Operator Precedence Analysis Process:" << endl;

    cout << setw(12) << data.analysisTable[0][0]
            << setw(16) << data.analysisTable[0][1]
            << setw(16) << data.analysisTable[0][2]
            << setw(24) << data.analysisTable[0][3]
            << setw(20) << data.analysisTable[0][4] << endl;
    outputFile << setw(12) << data.analysisTable[0][0]
            << setw(16) << data.analysisTable[0][1]
            << setw(16) << data.analysisTable[0][2]
            << setw(24) << data.analysisTable[0][3]
            << setw(20) << data.analysisTable[0][4] << endl;

    for (int i = 1; i <= data.currentStep; ++i) {
        cout << setw(10) << data.analysisTable[i][0]
                << setw(12) << data.analysisTable[i][1]
                << setw(10) << data.analysisTable[i][2]
                << setw(20) << data.analysisTable[i][3]
                << data.analysisTable[i][4] << endl;
        outputFile << setw(10) << data.analysisTable[i][0]
                << setw(12) << data.analysisTable[i][1]
                << setw(10) << data.analysisTable[i][2]
                << setw(20) << data.analysisTable[i][3]
                << data.analysisTable[i][4] << endl;
    }
    outputFile.close();
}

void printNonTerminals(const PrecedenceSets &sets) {
    cout << "Non-terminals:" << endl;
    for (int i = 0; i < sets.nonTerminalCount; ++i) {
        cout << sets.firstVT[i][0] << endl;
    }
}

void printFirstVTSets(const PrecedenceSets &sets) {
    cout << "FIRSTVT Sets:" << endl;
    for (int i = 0; i < sets.nonTerminalCount; ++i) {
        printSets("FIRSTVT", sets.firstVT[i][0], sets.firstVT[i][1]);
    }
}

void printLastVTSets(const PrecedenceSets &sets) {
    cout << "LASTVT Sets:" << endl;
    for (int i = 0; i < sets.nonTerminalCount; ++i) {
        printSets("LASTVT", sets.lastVT[i][0], sets.lastVT[i][1]);
    }
}

int main() {
    // Set left alignment for output
    cout.setf(ios::left);

    // Initialize data structures
    Grammar grammar;
    PrecedenceSets sets;
    ParsingData parsingData;

    // Read grammar from file
    readGrammarFromFile("../input.txt", grammar);

    // Print grammar rules
    printGrammar(grammar);

    // Extract non-terminals
    extractNonTerminals(grammar, sets);
    printNonTerminals(sets);

    // Compute FIRSTVT and LASTVT sets
    computeAllFirstVTSets(grammar, sets);
    printFirstVTSets(sets);

    computeAllLastVTSets(grammar, sets);
    printLastVTSets(sets);

    // Build precedence table
    initPrecedenceTable(grammar, parsingData);
    buildPrecedenceTable(grammar, sets, parsingData);
    printPrecedenceTable(parsingData);

    // Get input string
    cout << "Please enter an input string ending with #:" << endl;
    cin >> inputGrammar;

    // Analyze input string
    analyzeString(grammar, parsingData);

    // Print analysis results
    printAnalysisProcess(parsingData);

    return 0;
}
