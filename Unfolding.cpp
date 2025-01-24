#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include "nlohmann/json.hpp"

using namespace std;
using json = nlohmann::json;

using Matrix = vector<vector<int>>;
using Marking = vector<int>;

struct PetriNet {
    Matrix incidenceMatrix;
    Marking initialMarking;
    vector<string> places;
    vector<string> transitions;
};

PetriNet loadFromJSON(const string& filename) {
    ifstream file(filename);
    json j;
    file >> j;

    PetriNet net;
    net.incidenceMatrix = j["matrix"].get<Matrix>();
    net.initialMarking = j["initialMarking"].get<Marking>();

    int placeCount = net.incidenceMatrix.size();
    int transitionCount = net.incidenceMatrix[0].size();

    for (int i = 1; i <= placeCount; ++i) {
        net.places.push_back("p" + to_string(i));
    }
    for (int i = 1; i <= transitionCount; ++i) {
        net.transitions.push_back("t" + to_string(i));
    }

    return net;
}

void saveToJSON(const string& filename, const Matrix& matrix, const vector<string>& places, const vector<string>& transitions) {
    json j;
    j["matrix"] = matrix;
    j["Place"] = places;
    j["Transition"] = transitions;

    ofstream file(filename);
    file << j.dump(4);
}

bool isTransitionEnabled(const Marking& marking, const vector<int>& transition) {
    for (size_t i = 0; i < marking.size(); ++i) {
        if (transition[i] < 0 && marking[i] < -transition[i]) {
            return false;
        }
    }
    return true;
}

Marking fireTransition(const Marking& marking, const vector<int>& transition) {
    Marking newMarking = marking;
    for (size_t i = 0; i < marking.size(); ++i) {
        newMarking[i] += transition[i];
    }
    return newMarking;
}

void addPlace(Matrix& matrix, vector<string>& places, const string& placeName, const vector<int>& transition, size_t transitionIndex) {
    vector<int> newRow(matrix[0].size(), 0);
    newRow[transitionIndex] = transition[transitionIndex];
    matrix.push_back(newRow);
    places.push_back(placeName);
}

void addTransition(Matrix& matrix, vector<string>& transitions, const string& transitionName, const vector<int>& places, size_t placeIndex) {
    for (auto& row : matrix) {
        row.push_back(0);
    }
    matrix[placeIndex].back() = places[placeIndex];
    transitions.push_back(transitionName);
}

void unfoldRecursively(const PetriNet& net, const Marking& currentMarking, vector<Marking>& markingHistory,
                       Matrix& resultMatrix, vector<string>& resultPlaces, vector<string>& resultTransitions,
                       map<string, int>& duplicateCounts) {
    for (size_t t = 0; t < net.transitions.size(); ++t) {
        vector<int> transition;
        for (size_t p = 0; p < net.places.size(); ++p) {
            transition.push_back(net.incidenceMatrix[p][t]);
        }

        if (isTransitionEnabled(currentMarking, transition)) {
            Marking newMarking = fireTransition(currentMarking, transition);

            bool foundDuplicate = false;
            for (auto& previousMarking : markingHistory) {
                if (previousMarking == newMarking) {
                    foundDuplicate = true;
                    break;
                }
            }

            if (foundDuplicate) {
                for (size_t p = 0; p < net.places.size(); ++p) {
                    if (newMarking[p] > 0) {
                        string placeName = net.places[p];
                        int count = ++duplicateCounts[placeName];
                        addPlace(resultMatrix, resultPlaces, placeName + "(" + to_string(count) + ")", transition, t);
                    }
                }
                addTransition(resultMatrix, resultTransitions, net.transitions[t] + "(" + to_string(duplicateCounts[net.transitions[t]]) + ")", transition, t);
            } else {
                markingHistory.push_back(newMarking);

                for (size_t p = 0; p < net.places.size(); ++p) {
                    if (newMarking[p] > 0 && find(resultPlaces.begin(), resultPlaces.end(), net.places[p]) == resultPlaces.end()) {
                        addPlace(resultMatrix, resultPlaces, net.places[p], transition, t);
                    }
                }

                if (find(resultTransitions.begin(), resultTransitions.end(), net.transitions[t]) == resultTransitions.end()) {
                    addTransition(resultMatrix, resultTransitions, net.transitions[t], transition, t);
                }

                unfoldRecursively(net, newMarking, markingHistory, resultMatrix, resultPlaces, resultTransitions, duplicateCounts);
            }
        }
    }
}

pair<Matrix, pair<vector<string>, vector<string>>> unfolding(const PetriNet& net) {
    Matrix resultMatrix(net.places.size(), vector<int>(net.transitions.size(), 0));
    vector<string> resultPlaces = net.places;
    vector<string> resultTransitions = net.transitions;

    vector<Marking> markingHistory = {net.initialMarking};
    map<string, int> duplicateCounts;

    unfoldRecursively(net, net.initialMarking, markingHistory, resultMatrix, resultPlaces, resultTransitions, duplicateCounts);

    return {resultMatrix, {resultPlaces, resultTransitions}};
}

int main() {
    string inputFile = "input.json";
    string outputFile = "output.json";

    PetriNet net = loadFromJSON(inputFile);

    auto [resultMatrix, mappings] = unfolding(net);
    auto [resultPlaces, resultTransitions] = mappings;

    saveToJSON(outputFile, resultMatrix, resultPlaces, resultTransitions);

    cout << "Algorytm unfolding zakończony. Wynik zapisano do pliku " << outputFile << endl;

    return 0;
}
