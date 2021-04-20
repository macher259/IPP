/**
 * @file
 * Interfejs trybu wsadowego do gry Gamma. Specyfikacja trybu wsadowego
 * znajduje się pod linkiem:
 * https://moodle.mimuw.edu.pl/mod/assign/view.php?id=21505
 */
#ifndef GAMMA_BATCH_MODE_H
#define GAMMA_BATCH_MODE_H

#include <stddef.h>
#include "gamma.h"

/**
 * Główna funkcja modułu. Realizuje rozgrywkę w trybie wsadowym zgodnie ze
 * specyfikacją zadania. Pod koniec działania zwalnia zaalokowaną pamięć
 * wskazywaną na przez @p g.
 * @param g     - Wskaźnik na planszę do gry w Gamma. Różny od NULL.
 * @param line  - Wskaźnik na aktualny numer wiersza.
 */
void batch_mode(gamma_t *g, size_t *line);

/**
 * Funkcja wypisująca błąd na stderr zgodni ze specyfikacją zadania.
 * @param line  - Aktualny numer wiersza.
 */
void print_error(size_t line);

/**
 * Funkcja czytająca liczbę typu uint32_t z stdin.
 * Błędy obejmują:
 * - Niepoprawny format, np. -23.
 * - Wyjście poza zakres typu uint32_t.
 * - Znaki niebędące cyfrą, ani znakiem białym, np. 12c.
 * @param error     - Wskaźnik na zmienną trzymającą informacje o błędzie w
 * aktualnym poleceniu.
 * @return          -  * W razie powodzenia zwraca wczytaną liczbę, a w razie błędu
 * zwraca 0 i ustawia zmienna wskazywaną przz @p error na true.
 */
uint32_t read_uint32(bool *error);

/**
 * Funkcja czytająca ciąg białych znaków do następnego niebiałego znaku albo
 * końca strumienia. Za biały znak uznajemy znak zgodni z definicją w małym
 * zadaniu.
 */
void read_white_chars();

/**
 * Funkcja sprawdzająca, czy podana liczba całkowita jest białym znakiem z naszą
 * definicją białego znaku.
 * @param c     - Liczba, którą chcemy sprawdzić.
 * @return      - True, jeśli c spełnia przyjtą definicję, false w przeciwnym
 * wypadku.
 */
bool is_white(int c);

#endif //GAMMA_BATCH_MODE_H
