/**
 * @file
 * Implementacja modułu zawierającego wczytywanie standardowego wejścia przed
 * wybraniem trybu gry.
 */
#include <stddef.h>
#include <stdio.h>
#include "no_mode.h"
#include "interactive_mode.h"
#include "batch_mode.h"

#define NO_MODE 0 ///< Reprezentacja braku trybu gry.
#define START_LINE 1 ///< Numer pierwszego wiersza.
#define BATCH_MODE 983 ///< Reprzentacja trybu wsadowego.
#define INTERACTIVE_MODE 666 ///< Reprezentacja trybu interaktywnego.
#define BAD_VAR 0 /**< Reprezentacja 0, niemożliwego parametru dla wielu
 argumentów. */

/**
 * Funkcja pomocnicza polegająca na wczytywaniu znaków z stdin do natrafienia
 * na znak nowej linii albo koniec strumienia.
 */
static void skip_line() {
    int c = getchar();

    while (c != EOF && c != '\n')
        c = getchar();
}

/**
 * Funkcja pomocnicza wczytująca wiersz w celu wybrania trybu gry. Jeśli
 * wiersz jest niepoprawnym poleceniem, to wypisuje odpowiedni błąd na
 * stderr zgodnie ze specyfikacją zadania.
 *
 * @param line      - Wskaźnik na zminną trzymającą aktualny numer wiersza.
 * @param mode      - Wskaźnik na aktualny tryb gry.
 *
 * @return Wskaźnik na poprawnie utworzoną planszę do gry w Gamma w razie
 * powodzenia, NULL w przeciwnym wypadku.
 */
static gamma_t* get_game(size_t *line, int *mode) {
    bool error = false;
    int c = getchar();
    if (c == '#' || c == EOF) {
        return NULL;
    }
    if (c == '\n') {
        ungetc(c,stdin);
        return NULL;
    }
    if (c == 'B')
        *mode = BATCH_MODE;
    else if (c == 'I')
        *mode = INTERACTIVE_MODE;
    else {
        print_error(*line);
        return NULL;
    }

    read_white_chars();
    uint32_t width = read_uint32(&error);
    read_white_chars();
    uint32_t height= read_uint32(&error);
    read_white_chars();
    uint32_t players = read_uint32(&error);
    read_white_chars();
    uint32_t areas = read_uint32(&error);
    read_white_chars();

    if (width == BAD_VAR || height == BAD_VAR ||
         players == BAD_VAR || areas == BAD_VAR)
        error = true;

    if (!error) {
        c = getchar();
        if (c != '\n') {
            error = true;
        }
    }

    if (!(error || width == BAD_VAR || height == BAD_VAR ||
        players == BAD_VAR || areas == BAD_VAR))
        return gamma_new(width, height, players, areas);

    *mode = NO_MODE;
    print_error(*line);
    return NULL;
}

/**
 * Funkcja rozpoczynająca grę i wywołująca odpowiedni tryb.
 */
void begin_game() {
    size_t line = START_LINE;
    int mode = NO_MODE;

    gamma_t *g = NULL;

    while (!feof(stdin) && mode == NO_MODE) {
        g = get_game(&line, &mode);

        if (g == NULL)
            skip_line();

        ++line;
    }

    if (mode == BATCH_MODE) {
        printf("OK %zu\n", line - 1);
        batch_mode(g, &line);
    }
    else if(mode == INTERACTIVE_MODE)
        interactive_mode(g);
}