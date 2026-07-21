#include "cangjie/Competition/CompetitionRangeAnalysis.h"

#include "cangjie/CHIR/Utils/CHIRPrinter.h"
#include "cangjie/Competition/DominatorTree.h"

#include <fstream>
#include <sstream>
#include <vector>

namespace Competition {

using namespace Cangjie::CHIR;
using namespace std;

struct Query {
    string fileName;
    unsigned int lineNumber;
    string variableName;
};

vector<Query> readCompetitionQueries()
{
    // Open the "input.txt" file
    ifstream inputFile;
    inputFile.open("input.txt", ifstream::in);
    if (!inputFile.is_open()) {
        cerr << "No file 'input.txt'!" << endl;
        return {};
    }

    // Read for lineNumber and variableName
    string line;
    vector<Query> queries;
    while (getline(inputFile, line)) {
        // tokenize format [fileName, lineNumber, variableName]
        stringstream ss(line);
        string fileName, variableName;

        getline(ss, fileName, ',');
        fileName.erase(fileName.begin()); // Remove heading [

        unsigned int lineNumber;
        ss >> lineNumber;

        getline(ss, variableName, ','); // Remove comma
        getline(ss, variableName, ',');

        // Remove leading space
        while (*variableName.begin() == ' ') {
            variableName.erase(variableName.begin());
        }

        variableName.erase(variableName.end() - 1); // Remove last ]

        queries.push_back({fileName, lineNumber, variableName});
    }

    inputFile.close();

    return queries;
}

Block* getBlockByLineNumber(vector<Function*>& funcs, unsigned int lineNumber)
{
    // 1. Iterate on our functions to find which one contains the lineNumber
    for (auto& func : funcs) {
        auto loc = func->GetDebugLocation();
        if (loc.GetBeginPos().line <= lineNumber && lineNumber <= loc.GetEndPos().line) {
            // 2. Iterate on Blocks to find w.o.c.t. lineNumber
            for (auto& block : func->GetBody()->GetBlocks()) {
                auto blockLoc = block->GetDebugLocation();

                cerr << "Block " << block->GetIdentifier() << " has loc [" << blockLoc.GetBeginPos().line << ", "
                     << blockLoc.GetEndPos().line << "]" << endl;

                if (blockLoc.GetBeginPos().line <= lineNumber && lineNumber <= blockLoc.GetEndPos().line) {
                    return block;
                    // Iterate on expressions to find the exact line?
                }
            }
        }
    }
    return nullptr;
}

void RangeAnalysis::RunOnPackage(Package* package)
{
    // Filter out the builtin cangjie code
    if (package->GetName() == "std.core")
        return;

    cerr << "@@@@ COMPETITION ANALYSIS @@@@" << endl;

    // The package contains functions:
    vector<Function*> userDefinedFunctions;

    // TODO: We want to filter ONLY the defined by this package
    cerr << "Funcs = [ ";
    for (auto func : package->GetGlobalFuncsWithBody()) {
        if (func->GetPackageName() != "std.core") {
            cerr << func->GetSrcCodeIdentifier() << " ";
            userDefinedFunctions.push_back(func);
        }
    }
    std::cerr << "]" << std::endl;

    auto queries = readCompetitionQueries();

    for (auto [fileName, lineNumber, variableName] : queries) {
        std::cerr << "Find the range of variable " << variableName << " at line " << lineNumber << " of file "
                  << fileName << std::endl;

        // Call function that finds the Block* that contains this lineNumber
        // ! PROBLEM: Blocks do not keep the start and end lineNumber consistent :(
        auto block = getBlockByLineNumber(userDefinedFunctions, lineNumber);

        if (block == nullptr) {
            std::cerr << "Can't find block for lineNumber " << lineNumber << std::endl;
        } else {
            // TODO: Use the block to query the analysis
            std::cerr << "TODO: Query block for " << lineNumber << std::endl;
        }
    }

    // Create some dominator tree for each function?
    for (auto func : userDefinedFunctions) {
        // Our debug
        if (func->GetSrcCodeIdentifier() == "foo") {
            CHIRPrinter::PrintCFG(*func, "foo.dot");
            Block* entry = func->GetEntryBlock();
            DominatorTree domTree(entry);
            domTree.Compute();
            domTree.PrintDominatorTree("domTree.dot");
        }
        // TODO: Go on with SSA Builder
    }

    std::cerr << "@@@@ COMPETITION ANALYSIS END @@@@" << std::endl;
    return;
}

} // namespace Competition