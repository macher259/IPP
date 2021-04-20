/**
 * @file
 * Plik zawierający intefrejs modułu odpowiedzialnego za pobieranie danych ze
 * standardowego wejścia przed wybraniem trybu gry.
 */
#ifndef GAMMA_NO_MODE_H
#define GAMMA_NO_MODE_H

/**
 * Funkcja odpowiadająca za wczytanie polecenia z poprawnym trybem gry i
 * wywołaniem odpowiedniego trybu gry.
 * Z każdym błędnym poleceniem wypisuje stosowny błąd na stderr zgodnie
 * ze specyfikacją zadania.
 */
void begin_game();

#endif //GAMMA_NO_MODE_H
