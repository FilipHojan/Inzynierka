#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include "nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;

typedef vector<vector<int>> IncidenceMatrix;

IncidenceMatrix loadMatrixFromJSON(const string& filename, vector<int>& initialMarking) {
    ifstream file(filename);
    json j;
    file >> j;
    IncidenceMatrix matrix = j["matrix"].get<IncidenceMatrix>();
    initialMarking = j["initialMarking"].get<vector<int>>();
    return matrix;
}

void saveMatrixToJSON(const IncidenceMatrix& matrix, const vector<string>& placeNames,
                      const vector<string>& transitionNames, const string& filename) {
    json j;
    j["matrix"] = matrix;
    j["Place"] = placeNames;
    j["Transition"] = transitionNames;
    ofstream file(filename);
    file << j.dump(4);
}

bool isTransitionEnabled(const vector<int>& marking, const vector<int>& transition) {
    for (size_t i = 0; i < marking.size(); ++i) {
        if (transition[i] < 0 && marking[i] < -transition[i]) {
            return false;
        }
    }
    return true;
}

vector<int> fireTransition(const vector<int>& marking, const vector<int>& transition) {
    vector<int> newMarking = marking;
    for (size_t i = 0; i < marking.size(); ++i) {
        newMarking[i] += transition[i];
    }
    return newMarking;
}

pair<IncidenceMatrix, pair<vector<string>, vector<string>>> unfolding(
    const IncidenceMatrix& inputMatrix, const vector<int>& initialMarking) {
    IncidenceMatrix outputMatrix = inputMatrix;
    vector<string> placeNames;
    vector<string> transitionNames;

    set<vector<int>> visitedMarkings;
    map<vector<int>, int> markingToRow;
    vector<vector<int>> queue = {initialMarking};
    map<vector<int>, int> duplicateCounter;

    visitedMarkings.insert(initialMarking);
    for (size_t i = 0; i < initialMarking.size(); ++i) {
        placeNames.push_back("p" + to_string(i + 1));
    }

    for (size_t t = 0; t < inputMatrix[0].size(); ++t) {
        transitionNames.push_back("t" + to_string(t + 1));
    }

    while (!queue.empty()) {
        vector<int> currentMarking = queue.back();
        queue.pop_back();

        for (size_t t = 0; t < inputMatrix[0].size(); ++t) {
            vector<int> transition;
            for (size_t p = 0; p < inputMatrix.size(); ++p) {
                transition.push_back(inputMatrix[p][t]);
            }

            if (isTransitionEnabled(currentMarking, transition)) {
                vector<int> newMarking = fireTransition(currentMarking, transition);

                if (visitedMarkings.find(newMarking) != visitedMarkings.end()) {
                    // Duplicate place handling
                    int duplicateCount = ++duplicateCounter[newMarking];
                    string duplicatePlace =
                        "p" + to_string(markingToRow[newMarking] + 1) + "(" + to_string(duplicateCount) + ")";
                    placeNames.push_back(duplicatePlace);

                    vector<int> newRow(inputMatrix[0].size(), 0);
                    newRow[t] = 1;
                    outputMatrix.push_back(newRow);
                    continue;
                }

                visitedMarkings.insert(newMarking);
                queue.push_back(newMarking);

                markingToRow[newMarking] = outputMatrix.size();
                string newPlace = "p" + to_string(outputMatrix.size() + 1);
                placeNames.push_back(newPlace);

                vector<int> newRow(inputMatrix[0].size(), 0);
                newRow[t] = 1;
                outputMatrix.push_back(newRow);
            }
        }
    }

    return {outputMatrix, {placeNames, transitionNames}};
}

int main() {
    string inputFile = "input_matrix.json";
    string outputFile = "output_matrix.json";

    vector<int> initialMarking;
    IncidenceMatrix inputMatrix = loadMatrixFromJSON(inputFile, initialMarking);

    auto [resultMatrix, mappings] = unfolding(inputMatrix, initialMarking);
    auto [placeNames, transitionNames] = mappings;

    saveMatrixToJSON(resultMatrix, placeNames, transitionNames, outputFile);

    cout << "Macierz wynikowa unfolding zapisana do pliku " << outputFile << endl;

    return 0;
}