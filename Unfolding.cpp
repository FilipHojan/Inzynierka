#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <map>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

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

    for (int i = 1; i <= net.incidenceMatrix.size(); ++i) {
        net.places.push_back("p" + to_string(i));
    }
    for (int i = 1; i <= net.incidenceMatrix[0].size(); ++i) {
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

void addPlace(Matrix& matrix, vector<string>& places, const string& placeName) {
    places.push_back(placeName);
    vector<int> newRow(matrix[0].size(), 0); // Dodaj nowy wiersz z zerami
    matrix.push_back(newRow);
}

void addTransition(Matrix& matrix, vector<string>& transitions, const string& transitionName) {
    transitions.push_back(transitionName);
    for (auto& row : matrix) {
        row.push_back(0); // Dodaj nową kolumnę z zerami
    }
}

void updateMatrixForTransition(Matrix& matrix, const vector<string>& places, const vector<string>& transitions,
                               const string& fromPlace, const string& toTransition, const string& toPlace) {
    // Znajdź indeksy miejsca i tranzycji
    int fromPlaceIndex = -1, toTransitionIndex = -1, toPlaceIndex = -1;
    for (size_t i = 0; i < places.size(); ++i) {
        if (places[i] == fromPlace) fromPlaceIndex = i;
        if (places[i] == toPlace) toPlaceIndex = i;
    }
    for (size_t j = 0; j < transitions.size(); ++j) {
        if (transitions[j] == toTransition) toTransitionIndex = j;
    }

    // Zaktualizuj macierz
    if (fromPlaceIndex != -1 && toTransitionIndex != -1) {
        matrix[fromPlaceIndex][toTransitionIndex] = -1; // Z miejsca do tranzycji
    }
    if (toPlaceIndex != -1 && toTransitionIndex != -1) {
        matrix[toPlaceIndex][toTransitionIndex] = 1; // Z tranzycji do miejsca
    }
}

void unfoldRecursively(const PetriNet& net, Marking currentMarking, set<Marking>& visitedMarkings,
                       Matrix& resultMatrix, vector<string>& resultPlaces, vector<string>& resultTransitions,
                       map<string, int>& duplicateCounts, vector<Marking>& markingHistory) {
    visitedMarkings.insert(currentMarking);
    markingHistory.push_back(currentMarking);

    for (size_t t = 0; t < net.transitions.size(); ++t) {
        vector<int> transition;
        for (size_t p = 0; p < net.places.size(); ++p) {
            transition.push_back(net.incidenceMatrix[p][t]);
        }

        if (isTransitionEnabled(currentMarking, transition)) {
            Marking newMarking = fireTransition(currentMarking, transition);

            string transitionName = net.transitions[t];
            if (visitedMarkings.find(newMarking) != visitedMarkings.end()) {
                // Obsługa cyklu: dodaj duplikaty miejsc
                for (size_t p = 0; p < net.places.size(); ++p) {
                    if (newMarking[p] > 0) {
                        string placeName = net.places[p];
                        int count = ++duplicateCounts[placeName];
                        string duplicatePlace = placeName + "(" + to_string(count) + ")";
                        addPlace(resultMatrix, resultPlaces, duplicatePlace);
                        updateMatrixForTransition(resultMatrix, resultPlaces, resultTransitions, placeName, transitionName, duplicatePlace);
                    }
                }
            } else {
                visitedMarkings.insert(newMarking);
                markingHistory.push_back(newMarking);

                // Dodaj miejsca i przejścia
                for (size_t p = 0; p < net.places.size(); ++p) {
                    if (newMarking[p] > 0) {
                        addPlace(resultMatrix, resultPlaces, net.places[p]);
                        updateMatrixForTransition(resultMatrix, resultPlaces, resultTransitions, net.places[p], transitionName, net.places[p]);
                    }
                }

                addTransition(resultMatrix, resultTransitions, transitionName);
                unfoldRecursively(net, newMarking, visitedMarkings, resultMatrix, resultPlaces, resultTransitions, duplicateCounts, markingHistory);
            }
        }
    }

    markingHistory.pop_back();
    visitedMarkings.erase(currentMarking);
}

void unfoldPetriNet(const PetriNet& net, Matrix& resultMatrix, vector<string>& resultPlaces, vector<string>& resultTransitions) {
    set<Marking> visitedMarkings;
    map<string, int> duplicateCounts;
    vector<Marking> markingHistory;

    resultMatrix.assign(net.places.size(), vector<int>(net.transitions.size(), 0));
    resultPlaces = net.places;
    resultTransitions = net.transitions;

    unfoldRecursively(net, net.initialMarking, visitedMarkings, resultMatrix, resultPlaces, resultTransitions, duplicateCounts, markingHistory);
}

int main() {
    string inputFile = "input_matrix.json";
    string outputFile = "output_matrix.json";

    PetriNet net = loadFromJSON(inputFile);

    Matrix resultMatrix;
    vector<string> resultPlaces;
    vector<string> resultTransitions;

    unfoldPetriNet(net, resultMatrix, resultPlaces, resultTransitions);

    saveToJSON(outputFile, resultMatrix, resultPlaces, resultTransitions);

    cout << "Algorytm unfolding zakończony. Wynik zapisano do pliku " << outputFile << endl;

    return 0;
}
