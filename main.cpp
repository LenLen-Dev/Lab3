#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_set>
#include <map>
#include <algorithm>


/*
 * Author:LenLe
 * @param:std::string &fileName 文件名
 * @return std::vector<std::string> 文法字符串数组
 * Description:从文件读取文法，将文法转变为标准文法
 */
std::vector<std::string> GetGrammar(const std::string &fileName) {
    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "File " << fileName << " does not exist." << std::endl;
    }

    std::vector<std::string> grammar, grammarTemp;
    std::string line;

    while (std::getline(file, line)) {
        grammarTemp.push_back(line);
    }

    bool flag = false;
    std::string previousGrammar, nextGrammar;

    for (int i = 0; i < grammarTemp.size(); ++i) {
        flag = false;
        for (int j = 0; j < grammarTemp[i].size(); ++j) {
            char ch = grammarTemp[i][j];
            if (ch == '|') {
                std::string fixedString = grammarTemp[i].substr(0, 3); // 产生式左部和箭头
                previousGrammar = fixedString + grammarTemp[i].substr(3, j - 3);
                nextGrammar = fixedString + grammarTemp[i].substr(j + 1);
                grammar.push_back(previousGrammar);
                grammar.push_back(nextGrammar);
                flag = true;
            }
        }
        if (!flag) {
            grammar.push_back(grammarTemp[i]);
        }
    }
    return grammar;
}

std::vector<char> GetTerminals(const std::vector<std::string> &grammar) {
    std::unordered_set<char> seen;
    std::vector<char> terminals;

    for (const std::string &rule: grammar) {
        // 假设规则格式为 "非终结符 -> 右部"，从右部开始分析
        for (size_t j = 3; j < rule.size(); ++j) {
            char ch = rule[j];
            if (!isupper(ch) && ch != ' ' && ch != '-') {
                // 过滤掉空格和箭头字符
                if (seen.find(ch) == seen.end()) {
                    seen.insert(ch);
                    terminals.push_back(ch);
                }
            }
        }
    }

    return terminals;
}

std::vector<char> GetNonTerminals(const std::vector<std::string> &grammar) {
    std::unordered_set<char> seen;
    std::vector<char> nonTerminals;
    for (const std::string &rule: grammar) {
        for (char ch: rule) {
            if (isupper(ch)) {
                if (seen.find(ch) == seen.end()) {
                    seen.insert(ch);
                    nonTerminals.push_back(ch);
                }
            }
        }
    }
    return nonTerminals;
}


bool IsTerminal(const char ch, const std::vector<char> &terminals) {
    for (char terminal: terminals) {
        if (ch == terminal) {
            return true;
        }
    }
    return false;
}

bool IsNonTerminal(const char ch, const std::vector<char> &nonTerminals) {
    for (char nonTerminal: nonTerminals) {
        if (ch == nonTerminal) {
            return true;
        }
    }
    return false;
}


std::map<char, std::vector<char> > CalculateFirstVT(const std::vector<std::string> &grammar,
                                                    const std::vector<char> &terminals,
                                                    const std::vector<char> &nonTerminals) {
    std::map<char, std::vector<char> > firstVT;

    // Case 1: S -> i  or S -> +T
    for (const std::string &rule: grammar) {
        char left = rule[0];
        char firstSymbol = rule[3]; // First symbol after "->"

        if (IsTerminal(firstSymbol, terminals)) {
            if (std::find(firstVT[left].begin(), firstVT[left].end(), firstSymbol) == firstVT[left].end()) {
                firstVT[left].push_back(firstSymbol);
                // std::cout << "[Case1] FIRSTVT(" << left << ") += " << firstSymbol << std::endl;
            }
        } else if (IsNonTerminal(firstSymbol, nonTerminals)) {
            // 进一步检查是否是 T 后接终结符，如 E -> T+ 也要加 +
            if (rule.size() > 4 && IsTerminal(rule[4], terminals)) {
                char nextTerminal = rule[4];
                if (std::find(firstVT[left].begin(), firstVT[left].end(), nextTerminal) == firstVT[left].end()) {
                    firstVT[left].push_back(nextTerminal);
                    //std::cout << "[Case1.5] FIRSTVT(" << left << ") += " << nextTerminal << std::endl;
                }
            }
        }
    }

    // Case 2: E -> T ... 继承 FirstVT(T) 给 E
    bool updated;
    do {
        updated = false;
        for (const std::string &rule: grammar) {
            char A = rule[0];
            char B = rule[3];

            if (IsNonTerminal(B, nonTerminals) && A != B) {
                for (char terminal: firstVT[B]) {
                    if (std::find(firstVT[A].begin(), firstVT[A].end(), terminal) == firstVT[A].end()) {
                        firstVT[A].push_back(terminal);
                        std::cout << "[Case2] FIRSTVT(" << A << ") += " << terminal << "  from FIRSTVT(" << B << ")\n";
                        updated = true;
                    }
                }
            }
        }
    } while (updated);

    return firstVT;
}

std::map<char, std::vector<char> > CalculateLastVT(const std::vector<std::string> &grammar,
                                                   const std::vector<char> &terminals,
                                                   const std::vector<char> &nonTerminals) {
    std::map<char, std::vector<char> > lastVT;

    // Case 1: S -> ...i  或 S -> T+
    for (const std::string &rule: grammar) {
        char left = rule[0];
        size_t len = rule.size();
        char lastSymbol = rule[len - 1];

        if (IsTerminal(lastSymbol, terminals)) {
            if (std::find(lastVT[left].begin(), lastVT[left].end(), lastSymbol) == lastVT[left].end()) {
                lastVT[left].push_back(lastSymbol);
                // std::cout << "[Case1] LASTVT(" << left << ") += " << lastSymbol << std::endl;
            }
        } else if (IsNonTerminal(lastSymbol, nonTerminals)) {
            if (len >= 5 && IsTerminal(rule[len - 2], terminals)) {
                char prevTerminal = rule[len - 2];
                if (std::find(lastVT[left].begin(), lastVT[left].end(), prevTerminal) == lastVT[left].end()) {
                    lastVT[left].push_back(prevTerminal);
                    // std::cout << "[Case1.5] LASTVT(" << left << ") += " << prevTerminal << std::endl;
                }
            }
        }
    }

    // Case 2: A -> ...B 继承 LASTVT(B) 给 A
    bool updated;
    do {
        updated = false;
        for (const std::string &rule: grammar) {
            char A = rule[0];
            char B = rule[rule.size() - 1];

            if (IsNonTerminal(B, nonTerminals) && A != B) {
                for (char terminal: lastVT[B]) {
                    if (std::find(lastVT[A].begin(), lastVT[A].end(), terminal) == lastVT[A].end()) {
                        lastVT[A].push_back(terminal);
                        std::cout << "[Case2] LASTVT(" << A << ") += " << terminal << "  from LASTVT(" << B << ")\n";
                        updated = true;
                    }
                }
            }
        }
    } while (updated);

    return lastVT;
}

void PrintFirstVT(const std::map<char, std::vector<char> > &firstVT) {
    for (const auto &[nonTerminal, vtList]: firstVT) {
        std::cout << "FirstVT(" << nonTerminal << ") = { ";
        for (char terminal: vtList) {
            std::cout << terminal << " ";
        }
        std::cout << "}" << std::endl;
    }
}

void PrintLastVT(const std::map<char, std::vector<char> > &lastVT) {
    for (const auto &[nonTerminal, vtList]: lastVT) {
        std::cout << "LastVT(" << nonTerminal << ") = { ";
        for (char terminal: vtList) {
            std::cout << terminal << " ";
        }
        std::cout << "}" << std::endl;
    }
}

std::map<std::pair<char, char>, char> BuildPriorityTable(const std::vector<std::string> &grammar,
                                                         const std::vector<char> &terminals,
                                                         const std::vector<char> &nonTerminals,
                                                         const std::map<char, std::vector<char> > &firstVT,
                                                         const std::map<char, std::vector<char> > &lastVT) {
    std::map<std::pair<char, char>, char> priorityTable;

    for (const std::string &rule: grammar) {
        const std::string &rights = rule.substr(3);

        for (size_t i = 0; i < rights.size(); i++) {
            // 1.A -> ...ab...
            if (i < rights.size() - 1) {
                char a = rights[i];;
                char b = rights[i + 1];
                if (IsTerminal(a, terminals) && IsTerminal(b, terminals)) {
                    priorityTable[{a, b}] = '=';
                }
            }

            // 2. A -> aBc
            if (i < rights.size() - 2) {
                char a = rights[i];
                char B = rights[i + 1];
                char c = rights[i + 2];
                if (IsTerminal(a, terminals) && IsNonTerminal(B, nonTerminals) && IsTerminal(c, terminals)) {
                    priorityTable[{a, c}] = '=';
                }
            }

            // 3. A-> aB
            if (i < rights.size() - 1) {
                char a = rights[i];
                char B = rights[i + 1];
                if (IsTerminal(a, terminals) && IsNonTerminal(B, nonTerminals)) {
                    if (firstVT.count(B)) {
                        for (char vt: firstVT.at(B)) {
                            priorityTable[{a, vt}] = '<';
                        }
                    }
                }
            }

            // 4. A->Ab
            if (i < rights.size() - 1) {
                char A = rights[i];
                char b = rights[i + 1];
                if (IsNonTerminal(A, nonTerminals) && IsTerminal(b, terminals)) {
                    if (lastVT.count(A)) {
                        for (char vt: lastVT.at(A)) {
                            priorityTable[{vt, b}] = '>';
                        }
                    }
                }
            }
        }
    }

    // 5. E->#E#
    if (IsTerminal('#',terminals)) {
        // # < firstVT(E)
        if (firstVT.count('E')) {
            for (char vt: firstVT.at('E')) {
                priorityTable[{'#',vt }] = '<';
            }
        }

        // LastVT(E)>#
        if (lastVT.count('E')) {
            for (char vt: lastVT.at('E')) {
                priorityTable[{vt,'#' }] = '>';
            }
        }

        priorityTable[{'#','#'}] = '=';
    }

    return priorityTable;
}

void PrintPriorityTable(const std::map<std::pair<char, char>, char> &priorityTable,
    const std::vector<char> terminals) {
    std::cout << "\nOperator Precedence Table:\n";
    std::cout << "   ";
    for (char t : terminals) std::cout << t << " ";
    std::cout << "\n";

    for (char row : terminals) {
        std::cout << row << " ";
        for (char col : terminals) {
            auto key = std::make_pair(row, col);
            if (priorityTable.count(key)) {
                std::cout << priorityTable.at(key) << " ";
            } else {
                std::cout << "  ";
            }
        }
        std::cout << "\n";
    }
}

int main() {
    std::vector<std::string> grammar = GetGrammar("../input.txt");
    std::cout << grammar.size() << std::endl;
    for (int i = 0; i < grammar.size(); i++) {
        std::cout << grammar[i] << std::endl;
    }

    std::vector<char> terminals = GetTerminals(grammar);
    for (int i = 0; i < terminals.size(); i++) {
        std::cout << terminals[i] << " ";
    }

    std::cout << std::endl;

    std::vector<char> nonTerminals = GetNonTerminals(grammar);
    for (int i = 0; i < nonTerminals.size(); i++) {
        std::cout << nonTerminals[i] << " ";
    }

    std::cout << std::endl;

    std::map<char, std::vector<char> > firstVT = CalculateFirstVT(grammar, terminals, nonTerminals);
    std::map<char, std::vector<char> > lastVt = CalculateLastVT(grammar, terminals, nonTerminals);
    PrintFirstVT(firstVT);
    PrintLastVT(lastVt);

    std::map<std::pair<char, char>, char> priorityTable = BuildPriorityTable(grammar, terminals, nonTerminals,firstVT, lastVt);
    PrintPriorityTable(priorityTable, terminals);



    return 0;
}
