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

void saveMatrixToJSON(const IncidenceMatrix& matrix, const vector<int>& marking, const string& filename) {
    json j;
    j["matrix"] = matrix;
    j["marking"] = marking;
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

pair<IncidenceMatrix, vector<int>> unfolding(const IncidenceMatrix& inputMatrix, const vector<int>& initialMarking) {
    IncidenceMatrix outputMatrix = inputMatrix;
    vector<int> extendedMarking(initialMarking.size(), 0);
    set<vector<int>> visitedMarkings;
    vector<vector<int>> queue = {initialMarking};
    visitedMarkings.insert(initialMarking);

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

                if (visitedMarkings.find(newMarking) == visitedMarkings.end()) {
                    visitedMarkings.insert(newMarking);
                    queue.push_back(newMarking);
                    vector<int> newRow(inputMatrix[0].size(), 0);
                    newRow[t] = 1;
                    outputMatrix.push_back(newRow);
                    extendedMarking.push_back(0);
                }
            }
        }
    }

    return {outputMatrix, extendedMarking};
}

int main() {
    string inputFile = "input_matrix.json";
    string outputFile = "output_matrix.json";

    vector<int> initialMarking;
    IncidenceMatrix inputMatrix = loadMatrixFromJSON(inputFile, initialMarking);

    cout << "Odczytana macierz wejściowa:\n";
    for (const auto& row : inputMatrix) {
        for (int val : row) {
            cout << val << " ";
        }
        cout << endl;
    }

    cout << "Odczytany początkowy marking:\n";
    for (int val : initialMarking) {
        cout << val << " ";
    }
    cout << endl;

    auto [resultMatrix, resultMarking] = unfolding(inputMatrix, initialMarking);

    saveMatrixToJSON(resultMatrix, resultMarking, outputFile);

    cout << "Macierz wynikowa unfolding zapisana do pliku " << outputFile << ":\n";
    for (const auto& row : resultMatrix) {
        for (int val : row) {
            cout << val << " ";
        }
        cout << endl;
    }

    return 0;
}
