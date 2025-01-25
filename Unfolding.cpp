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
    Matrix incidenceMatrix;       // Macierz incydencji opisująca zależności między miejscami i przejściami.
    Marking initialMarking;       // Oznakowanie początkowe sieci.
    vector<string> places;        // Nazwy miejsc w sieci.
    vector<string> transitions;   // Nazwy przejść w sieci.
};

PetriNet loadFromJSON(const string& filename) {
    ifstream file(filename);      // Otwiera plik JSON do odczytu.
    json j;                       // Tworzy obiekt JSON.
    file >> j;                    // Wczytuje dane z pliku do obiektu JSON.

    PetriNet net;                 // Tworzy obiekt sieci Petriego.
    net.incidenceMatrix = j["matrix"].get<Matrix>(); // Wczytuje macierz incydencji.
    net.initialMarking = j["initialMarking"].get<Marking>(); // Wczytuje oznakowanie początkowe.

    int placeCount = net.incidenceMatrix.size(); // Liczba miejsc (wiersze macierzy).
    int transitionCount = net.incidenceMatrix[0].size(); // Liczba przejść (kolumny macierzy).

    // Generuje nazwy miejsc w formacie p1, p2, ...
    for (int i = 1; i <= placeCount; ++i) {
        net.places.push_back("p" + to_string(i));
    }

    // Generuje nazwy przejść w formacie t1, t2, ...
    for (int i = 1; i <= transitionCount; ++i) {
        net.transitions.push_back("t" + to_string(i));
    }

    return net; // Zwraca wczytaną sieć Petriego.
}

void saveToJSON(const string& filename, const Matrix& matrix, const vector<string>& places, const vector<string>& transitions) {
    json j;                       // Tworzy obiekt JSON.
    j["matrix"] = matrix;         // Dodaje macierz do obiektu JSON.
    j["Place"] = places;          // Dodaje miejsca do obiektu JSON.
    j["Transition"] = transitions; // Dodaje przejścia do obiektu JSON.

    ofstream file(filename);      // Otwiera plik JSON do zapisu.
    file << j.dump(4);            // Zapisuje dane w formacie JSON z wcięciem 4 spacji.
}

// Sprawdzenie czy moze zostac uruchomiona tranzycja
bool isTransitionEnabled(const Marking& marking, const vector<int>& transition) {
    for (size_t i = 0; i < transition.size(); ++i) { // Iteruje przez wszystkie indeksy w transition.
        if (transition[i] < 0) { // Sprawdza, czy wartość jest ujemna.
            if (marking[i] < -transition[i]) { // Sprawdza, czy marking[i] spełnia warunek.
                return false; // Jeśli marking[i] jest za mały, przejście jest zablokowane.
            }
        }
    }
    return true; // Jeśli wszystkie warunki są spełnione, przejście jest aktywne.
}

// Przeniesienie tokenów po uruchomieniu
Marking fireTransition(const Marking& marking, const vector<int>& transition) {

    // Tworzenie kopii oznakowania
    Marking newMarking(marking.size(), 0);
    for (size_t i = 0; i < marking.size(); ++i) {
        newMarking[i] = marking[i] + transition[i]; // Dodanie wartości z transition
    }

    return newMarking; // Zwróć poprawnie zaktualizowane oznakowanie
}




// Dodawanie wypelnionych kolumn nowej macierzy
void addTransitionColumn(Matrix& matrix, const Marking& newMarking, const vector<Marking>& historyMarking, size_t transitionIndex) {
    // Pobierz ostatnie oznakowanie z historii jako "previousMarking"
    const Marking& previousMarking = historyMarking.back();

    // Obliczamy różnicę między newMarking i previousMarking
    vector<int> newColumn(newMarking.size(), 0);
    for (size_t i = 0; i < newMarking.size(); ++i) {
        newColumn[i] = newMarking[i] - previousMarking[i];
    }

    // Jeśli macierz jest pusta, inicjalizujemy ją
    if (matrix.empty()) {
        for (size_t i = 0; i < newMarking.size(); ++i) {
            matrix.push_back({newColumn[i]});
        }
    } else {
        // Dodajemy nową kolumnę do istniejącej macierzy
        for (size_t i = 0; i < matrix.size(); ++i) {
            matrix[i].push_back(newColumn[i]);
        }
    }
}

// Dodawanie wypełnionych kolumny w nowej macierzy wraz z dodaniem nowych wierszy ze wzgledu na duplikaty
void addTransitionColumn_CYCLE(Matrix& matrix, const Marking& newMarking, const vector<Marking>& historyMarking, size_t transitionIndex) {
    // Pobierz ostatnie oznakowanie z historii jako "previousMarking"
    const Marking& previousMarking = historyMarking.back();

    // Obliczamy różnicę między newMarking i previousMarking
    vector<int> newColumn(newMarking.size(), 0);
    for (size_t i = 0; i < newMarking.size(); ++i) {
        newColumn[i] = newMarking[i] - previousMarking[i];
    }

    // Dodanie nowej kolumny do istniejącej macierzy
    if (matrix.empty()) {
        for (size_t i = 0; i < newMarking.size(); ++i) {
            matrix.push_back({newColumn[i]}); // Jeśli macierz jest pusta, inicjalizuj ją z newColumn.
        }
    } else {
        for (size_t i = 0; i < matrix.size(); ++i) {
            matrix[i].push_back(newColumn[i]); // Dodaj nową kolumnę do istniejącej macierzy.
        }
    }

    // Sprawdzanie liczb dodatnich w newColumn
    for (size_t i = 0; i < newColumn.size(); ++i) {
        if (newColumn[i] > 0) { // Znaleziono liczbę dodatnią
            int transfer_connect = newColumn[i]; // Przechowujemy wartość dodatnią

            // **Poprawka: Zmieniamy wartość w macierzy na 0**
            matrix[i].back() = 0;

            // Tworzymy nowy wiersz i wypełniamy go zerami
            vector<int> newRow(matrix[0].size(), 0);

            // W nowej kolumnie wpisujemy wartość transfer_connect
            newRow.back() = transfer_connect;

            // Dodajemy nowy wiersz do macierzy
            matrix.push_back(newRow);
        }
    }
}




void unfoldRecursively(const PetriNet& net, const Marking& currentMarking, vector<Marking>& markingHistory, Matrix& resultMatrix, vector<string>& resultPlaces, vector<string>& resultTransitions, map<string, int>& duplicateCounts, size_t currentTransition = 0) {
    for (size_t t = currentTransition; t < net.transitions.size(); ++t) { // Rozpoczynamy od `currentTransition`
        vector<int> transition;
        for (size_t p = 0; p < net.places.size(); ++p) {
            transition.push_back(net.incidenceMatrix[p][t]); // Pobiera kolumnę macierzy dla danego przejścia.
        }

        if (isTransitionEnabled(currentMarking, transition)) { // Sprawdza, czy przejście jest aktywne.
            Marking newMarking = fireTransition(currentMarking, transition); // Wykonuje przejście.

            bool foundDuplicate = false;
            for (auto& previousMarking : markingHistory) {
                if (previousMarking == newMarking) {
                    foundDuplicate = true; // Sprawdza, czy oznakowanie już istnieje.
                    break;
                }
            }

            if (foundDuplicate) {
                addTransitionColumn_CYCLE(resultMatrix, newMarking, markingHistory, t);

                continue;
            } else { // Jeśli nie znaleziono duplikatu, dodaje nowe węzły.

                if (!markingHistory.empty()) {
                    // Dodanie nowej kolumny do macierzy na podstawie różnicy newMarking i ostatniego historyMarking
                    addTransitionColumn(resultMatrix, newMarking, markingHistory, t);
                }

                // Dodanie newMarking do historii oznakowań
                markingHistory.push_back(newMarking);

                // Wywołanie rekurencyjne rozpoczynające się od następnego przejścia
                unfoldRecursively(net, newMarking, markingHistory, resultMatrix, resultPlaces, resultTransitions, duplicateCounts, t + 1);
            }
        }
    }
}

pair<Matrix, pair<vector<string>, vector<string>>> unfolding(const PetriNet& net) {
    Matrix resultMatrix; // Początkowo pusta macierz wynikowa.
    vector<string> resultPlaces; // Początkowo pusta lista miejsc.
    vector<string> resultTransitions; // Początkowo pusta lista przejść.

    vector<Marking> markingHistory = {net.initialMarking}; // Historia oznakowań z początkowym oznakowaniem.
    map<string, int> duplicateCounts; // Licznik duplikatów dla miejsc i przejść.

    unfoldRecursively(net, net.initialMarking, markingHistory, resultMatrix, resultPlaces, resultTransitions, duplicateCounts);

    return {resultMatrix, {resultPlaces, resultTransitions}}; // Zwraca macierz wynikową i odpowiadające listy.
}

int main() {
    string inputFile = "input.json"; // Plik wejściowy JSON.
    string outputFile = "output.json"; // Plik wyjściowy JSON.

    PetriNet net = loadFromJSON(inputFile); // Wczytuje sieć Petriego z pliku.

    auto [resultMatrix, mappings] = unfolding(net); // Przeprowadza unfolding i otrzymuje wynikową macierz i mapowania.
    auto [resultPlaces, resultTransitions] = mappings; // Rozpakowuje mapowania miejsc i przejść.

    saveToJSON(outputFile, resultMatrix, resultPlaces, resultTransitions); // Zapisuje wynik do pliku JSON.

    cout << "Algorytm unfolding zakończony. Wynik zapisano do pliku " << outputFile << endl;

    return 0; // Kończy program.
}
