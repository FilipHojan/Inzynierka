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

// Zmienne globalne dla wyjątków
string numberColumnException = "";
string numberRowException = "";

// Zmienne globalne na nazwy duplikatów
string duplicatedPlaceName = "";
string duplicatedTransitionName = "";

//Duplikat przechodni
void Exception(Matrix& matrix, Marking& newMarking, vector<string>& resultPlaces, vector<string>& resultTransitions, map<string, int>& duplicateCounts, vector<bool>& visitedTransitions, size_t transitionIndex) {
    size_t numPlaces = matrix.size();
    size_t numTransitions = matrix[0].size();

    // Zmienna do przechowania wartości dodatniej i jej indeksu
    int transfer_connect = 0;
    size_t duplicatedPlaceIndex = 0;

    // ---- Znalezienie miejsca z liczbą dodatnią ----
    for (size_t p = 0; p < numPlaces; ++p) {
        if (matrix[p][transitionIndex] > 0) {
            transfer_connect = matrix[p][transitionIndex];
            matrix[p][transitionIndex] = 0;  // Wyzerowanie przepływu w macierzy
            duplicatedPlaceIndex = p;

            // ---- Dodanie nowego wiersza ----
            vector<int> newRow(numTransitions, 0);
            newRow[transitionIndex] = transfer_connect;
            matrix.push_back(newRow);  // Nowy wiersz w macierzy

            // ---- Tworzenie nazwy duplikatu miejsca ----
            string originalPlace = "p" + to_string(p + 1);
            int duplicateIndex = ++duplicateCounts[originalPlace];
            duplicatedPlaceName = originalPlace + "(" + to_string(duplicateIndex) + ")";
            resultPlaces.push_back(duplicatedPlaceName);

            newMarking.push_back(transfer_connect);
            break;
        }
    }

    // ---- Kopiowanie kolumny ----
    vector<int> copyColumn(matrix.size(), 0);
    for (size_t p = 0; p < numPlaces; ++p) {
        copyColumn[p] = matrix[p][transitionIndex];
    }

    // ---- Znalezienie liczby ujemnej i przeniesienie jej ----
    for (size_t p = 0; p < copyColumn.size(); ++p) {
        if (copyColumn[p] < 0) {
            int negativeValue = copyColumn[p];
            copyColumn[p] = 0;  // Wyzerowanie wartości w kolumnie kopii
            matrix.back()[p] = negativeValue;  // Wpisanie ujemnej wartości do nowego wiersza
            break;
        }
    }

    // ---- Dodanie nowej kolumny do macierzy wejściowej ----
    for (size_t p = 0; p < matrix.size(); ++p) {
        matrix[p].push_back(copyColumn[p]);
    }

    // ---- Tworzenie nazwy nowego przejścia ----
    string originalTransition = "t" + to_string(transitionIndex + 1);
    int duplicateTransitionIndex = ++duplicateCounts[originalTransition];
    duplicatedTransitionName = originalTransition + "(" + to_string(duplicateTransitionIndex) + ")";
    resultTransitions.push_back(duplicatedTransitionName);

    // ---- Aktualizacja visitedTransitions ----
    visitedTransitions.push_back(false);
}







PetriNet loadFromJSON(const string& filename) {
    ifstream file(filename);      // Otwiera plik JSON do odczytu.
    json j;                       // Tworzy obiekt JSON.
    file >> j;                    // Wczytuje dane z pliku do obiektu JSON.

    PetriNet net;                 // Tworzy obiekt sieci Petriego.
    net.incidenceMatrix = j["matrix"].get<Matrix>(); // Wczytuje macierz incydencji.
    net.initialMarking = j["initialMarking"].get<Marking>(); // Wczytuje oznakowanie początkowe.

    int placeCount = net.incidenceMatrix.size(); // Liczba miejsc (wiersze macierzy).
    int transitionCount = net.incidenceMatrix[0].size(); // Liczba przejść (kolumny macierzy).

    // Dodajemy puste miejsca i przejścia na podstawie rozmiaru
    net.places.resize(placeCount);       // Rezerwujemy miejsce dla miejsc.
    net.transitions.resize(transitionCount); // Rezerwujemy miejsce dla przejść.

    return net; // Zwraca wczytaną sieć Petriego.
}

//Nakładanie starego i nowe oznakowania
vector<int> generateFinalMarking(const Matrix& resultMatrix, const Marking& initialMarking) {
    size_t rowCount = resultMatrix.size(); // Liczba wierszy w macierzy wynikowej
    vector<int> finalMarking(rowCount, 0); // Tworzymy wektor wypełniony zerami

    // Przepisujemy początkowe oznakowanie
    for (size_t i = 0; i < initialMarking.size(); ++i) {
        if (i < rowCount) {
            finalMarking[i] = initialMarking[i]; // Nakładamy początkowe oznakowanie
        }
    }

    return finalMarking; // Zwracamy końcowe oznakowanie
}


void saveToJSON(const string& filename, const Matrix& matrix, const vector<string>& places, const vector<string>& transitions, const vector<int>& marking) {
    json j;                       // Tworzy obiekt JSON.
    j["Place"] = places;          // Dodaje miejsca do obiektu JSON.
    j["Transition"] = transitions; // Dodaje przejścia do obiektu JSON.
    j["matrix"] = matrix;         // Dodaje macierz do obiektu JSON.
    j["Marking"] = marking;       // Dodaje oznakowanie do obiektu JSON.


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

void addPlace(vector<string>& resultPlaces, const string& placeName) {
    if (!duplicatedPlaceName.empty()) {
        if (find(resultPlaces.begin(), resultPlaces.end(), duplicatedPlaceName) == resultPlaces.end()) {
            resultPlaces.push_back(duplicatedPlaceName);
        }
        duplicatedPlaceName.clear();  // Wyczyszczenie po dodaniu
    }

    if (find(resultPlaces.begin(), resultPlaces.end(), placeName) == resultPlaces.end()) {
        resultPlaces.push_back(placeName);
    }
}

void addTransition(vector<string>& resultTransitions, const string& transitionName) {
    if (!duplicatedTransitionName.empty()) {
        if (find(resultTransitions.begin(), resultTransitions.end(), duplicatedTransitionName) == resultTransitions.end()) {
            resultTransitions.push_back(duplicatedTransitionName);
        }
        duplicatedTransitionName.clear();  // Wyczyszczenie po dodaniu
    }

    if (find(resultTransitions.begin(), resultTransitions.end(), transitionName) == resultTransitions.end()) {
        resultTransitions.push_back(transitionName);
    }
}

// Dodawanie wypelnionych kolumn nowej macierzy
void addTransitionColumn(Matrix& matrix, const Marking& newMarking, const vector<Marking>& historyMarking, size_t transitionIndex, vector<string>& resultTransitions) {
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


    // Dodanie nazwy przejścia
    addTransition(resultTransitions, "t" + to_string(transitionIndex + 1));

}

// Dodawanie wypełnionych kolumny w nowej macierzy wraz z dodaniem nowych wierszy ze wzgledu na duplikaty
void addTransitionColumn_CYCLE(Matrix& matrix, const Marking& newMarking, const vector<Marking>& historyMarking, size_t transitionIndex, vector<string>& resultTransitions, vector<string>& resultPlaces, map<string, int>& duplicateCounts) {
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

    // Dodanie nazwy przejścia
    addTransition(resultTransitions, "t" + to_string(transitionIndex + 1));

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

            // Dodajemy nazwę miejsca z duplikatem do resultPlaces
            string originalPlace = "p" + to_string(i + 1); // Pierwotne miejsce, np. "p1"
            int duplicateIndex = ++duplicateCounts[originalPlace]; // Aktualizujemy licznik duplikatów
            string newPlaceName = originalPlace + "(" + to_string(duplicateIndex) + ")";
            resultPlaces.push_back(newPlaceName); // Dodajemy nową nazwę miejsca
        }
    }
}




void unfoldRecursively(PetriNet& net, Marking& currentMarking, vector<Marking>& markingHistory, Matrix& resultMatrix, vector<string>& resultPlaces, vector<string>& resultTransitions, map<string, int>& duplicateCounts, vector<bool>& visitedTransitions, size_t currentTransition = 0)
{
    bool hasActiveTransitions = false;

    for (; currentTransition < net.incidenceMatrix[0].size(); ++currentTransition) {
        vector<int> transition;
        for (size_t p = 0; p < net.places.size(); ++p) {
            transition.push_back(net.incidenceMatrix[p][currentTransition]);
        }

        if (isTransitionEnabled(currentMarking, transition)) {
            hasActiveTransitions = true;

            if (visitedTransitions[currentTransition]) {
                for (size_t t = 0; t < net.incidenceMatrix[0].size(); ++t) {
                    if (t != currentTransition && visitedTransitions[t]) {
                        for (size_t p = 0; p < net.places.size(); ++p) {
                            if (net.incidenceMatrix[p][t] > 0 && net.incidenceMatrix[p][currentTransition] > 0) {
                                Exception(net.incidenceMatrix, currentMarking, resultPlaces, resultTransitions, duplicateCounts, visitedTransitions, currentTransition);
                                unfoldRecursively(net, currentMarking, markingHistory, resultMatrix, resultPlaces, resultTransitions, duplicateCounts, visitedTransitions, currentTransition);
                                return;
                            }
                        }
                    }
                }
            }
            
            Marking newMarking = fireTransition(currentMarking, transition);
            if (find(markingHistory.begin(), markingHistory.end(), newMarking) != markingHistory.end()) {
                addTransitionColumn_CYCLE(resultMatrix, newMarking, markingHistory, currentTransition, resultTransitions, resultPlaces, duplicateCounts);
                visitedTransitions[currentTransition] = true;
                markingHistory.pop_back();
                unfoldRecursively(net, newMarking, markingHistory, resultMatrix, resultPlaces, resultTransitions, duplicateCounts, visitedTransitions, currentTransition - 1);
                return;
            } else {
                addTransitionColumn(resultMatrix, newMarking, markingHistory, currentTransition, resultTransitions);
                markingHistory.push_back(newMarking);
                visitedTransitions[currentTransition] = true;
                unfoldRecursively(net, newMarking, markingHistory, resultMatrix, resultPlaces, resultTransitions, duplicateCounts, visitedTransitions, currentTransition + 1);
                return;
            }
        }
    }

    while (!markingHistory.empty()) {
        Marking newMarking = markingHistory.back();
        markingHistory.pop_back();
        for (size_t t = 0; t < net.incidenceMatrix[0].size(); ++t) {
            if (!visitedTransitions[t]) {
                vector<int> transition;
                for (size_t p = 0; p < net.places.size(); ++p) {
                    transition.push_back(net.incidenceMatrix[p][t]);
                }
                if (isTransitionEnabled(newMarking, transition)) {
                    unfoldRecursively(net, newMarking, markingHistory, resultMatrix, resultPlaces, resultTransitions, duplicateCounts, visitedTransitions, t);
                    return;
                }
            }
        }
    }
}




tuple<Matrix, vector<string>, vector<string>, vector<int>> unfolding(PetriNet& net) {
    Matrix resultMatrix; 
    vector<string> resultPlaces; 
    vector<string> resultTransitions;
    
    vector<bool> visitedTransitions(net.incidenceMatrix[0].size(), false); // Prawidłowa inicjalizacja

    vector<Marking> markingHistory = {net.initialMarking}; 
    map<string, int> duplicateCounts; 

    for (size_t i = 0; i < net.initialMarking.size(); ++i) {
        addPlace(resultPlaces, "p" + to_string(i + 1));
    }

    unfoldRecursively(net, net.initialMarking, markingHistory, resultMatrix, resultPlaces, resultTransitions, duplicateCounts, visitedTransitions);

    vector<int> finalMarking = generateFinalMarking(resultMatrix, net.initialMarking);

    return {resultMatrix, resultPlaces, resultTransitions, finalMarking};
}


int main() {
    string inputFile = "input.json"; // Plik wejściowy JSON.
    string outputFile = "output.json"; // Plik wyjściowy JSON.

    PetriNet net = loadFromJSON(inputFile); // Wczytuje sieć Petriego z pliku.

    // Rozpakowanie wyniku funkcji `unfolding`
    auto [resultMatrix, resultPlaces, resultTransitions, finalMarking] = unfolding(net);

    // Zapisanie wyniku do pliku JSON
    saveToJSON(outputFile, resultMatrix, resultPlaces, resultTransitions, finalMarking);

    cout << "Algorytm unfolding zakończony. Wynik zapisano do pliku " << outputFile << endl;

    return 0; // Kończy program.
}
