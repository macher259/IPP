/**
 * @file
 * Implementacja obsługi trybu wsadowego do gry w gamma.
 * Wykorzystuję czytanie znak po znaku przy użyciu funkcji getchar(), ponieważ
 * nie chcę wprowadzać
 * dodatkowej alokacji pamięci, żeby wczytywać całą linijkę naraz, ale też
 * nie chcę komplikować architektury przez stosowanie jakiegoś bufora
 * cyklicznego albo czegoś w tym rodzaju.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "batch_mode.h"
#include "gamma.h"

#define BASE 10 ///< Podstawa systemu liczbowego wczytywanych liczb.
#define BLANK 0 ///< Reprezentacja pustego paramtru.

/**
 * Funkcja pomocnicza sprawdzająca, czy następny znak z stdin jest cyfrą.
 * Jeśli udało się pobrać znak, jest on zwracany na stdin.
 * Jeśli pobrany znak nie jest cyfrą ustawia zmienną wskazywaną przez
 * @p error na wartość true.
 * @param error     - Wskaźnik na zmienną typu bool informującą o błędzie w
 * poleceniu w trybie wsadowym.
 */
static void is_next_int(bool *error) {
    if (*error)
        return;
    int c = getchar();
    if (c < '0' || c > '9')
        *error = true;
    if (!feof(stdin))
        ungetc(c, stdin);
}

/**
 * Funcja pomocnicza mająca za zadanie wczytywanie znaków z stdin, dopóki
 * nie natrafi na koniec strumienia lub koniec wiersza.
 */
static void skip_line() {
    int c = '0';
    while (c != EOF && c != '\n')
        c = getchar();
}

/**
 * Funkcja pomocnicza realizująca polecenie wypisania planszy na zlecenie
 * użytkownika. Wypisuje planszę w razie sukcesu, a w razie błędnego
 * polecenia wypisuje na stderr błąd zgodny ze specyfikacją zadania.
 * @param g     - Wskaźnik na planszę, którą chcemy wypisać. Różny od NULL.
 * @param line      - Wskaźnik na aktualny numer linii do wypisywania błędu.
 */
static void print_command(gamma_t *g, const size_t *line) {
    read_white_chars();

    int c = getchar();
    if (c != '\n') {
        print_error(*line);
        skip_line();
        return;
    }

    char *board = gamma_board(g);
    if (board == NULL) {
        print_error(*line);
        skip_line();
        return;
    }
    printf("%s", board);
    free(board);
}

/**
 * Sprawdza, czy dany znak @p c jest znakiem białym z przyjtą konwencją.
 * @param c     - znak do sprawdzenia.
 * @return true, jeśli spełnia naszą definicję białęgo znaku.
 * false w przeciwnym wypadku.
 */
bool is_white(int c) {
    return (c == ' ' || c == '\r' || c == '\t' ||
            c == '\v' || c == '\f');
}

void print_error(size_t line) {
    fprintf(stderr, "ERROR %zu\n", line);
}


uint32_t read_uint32(bool *error) {
    if (*error)
        return BLANK;

    uint64_t number = 0;
    read_white_chars();
    int c = getchar();


    while (!(*error) && c != EOF && !is_white(c) && c != '\n') {
        if (c >= '0' && c <= '9' && number <= UINT32_MAX) {
            number *= BASE;
            number += c - '0';
            c = getchar();
        }
        else {
            *error = true;
        }
    }

    if (c == '\n') {
        ungetc(c, stdin);
    }


    if (number > UINT32_MAX)
        *error = true;

    if (*error)
        return BLANK;

    return number;
}

void read_white_chars() {
    int c = getchar();

    while (!feof(stdin) && is_white(c))
        c = getchar();

    if (!feof(stdin))
        ungetc(c, stdin);
}

/**
 * Funkcja pomocnicza realizująca polecnie wypisanie wolnych pól dla gracza
 *  na planszy @p g. W razie powodzenia wypisuje liczb wolnych pól,
 * lecz w przypadku, gdy polecenie jest nizgodne z specyfikacją zadania
 * wypisuje stosowny błąd na sterr zgodnie ze specyfikacją zadania.
 *
 * @param g     - Wskaźnik na planszę do gry w Gamma.
 * @param error - Wskaźnik na zminną trzymającą informacje o poprawności
 * aktualnego polcenia.
 * @param line  - Wskaźnik na aktualny numer wiersza do wypisania błędu.
 */
static void free_fields_command(gamma_t *g, bool *error, const size_t *line) {
    int c = getchar();
    if (!is_white(c)) {
        *error = true;
        if (!feof(stdin))
            ungetc(c, stdin);
    }

    read_white_chars();
    is_next_int(error);

    uint32_t player = read_uint32(error);

    read_white_chars();

    c = getchar();


    if (c != '\n') {
        *error = true;
        skip_line();
    }

    if (!(*error)) {
        printf("%lu\n", gamma_free_fields(g, player));
    }
    else {
        print_error(*line);
    }
}

/**
 * Funkcja pomocnicza realizująca polecenie wypisania ilości zajętych pól przez
 * gracza na planszy @p g. W razie powodzenia wypisuje liczbę zajętych pól
 * przez gracza, zaś w przypadku niepowodzenia wypisuje stosowny błąd na
 * stdrr zgodnie ze specyfikacją.
 *
 * @param g     - Wskaźnik na planszę do gry w Gamma.
 * @param error - Wskaźnik na zmienną trzymającą informacj o poprawności
 * aktualnego polecenia.
 * @param line  - Wskaźnik na zmienną trzymającą aktualny numer wiersza.
 */
static void busy_fields_command(gamma_t *g, bool *error, const size_t *line) {
    int c = getchar();
    if (!is_white(c)) {
        *error = true;
        if (!feof(stdin))
            ungetc(c, stdin);
    }

    read_white_chars();
    is_next_int(error);

    uint32_t player = read_uint32(error);

    read_white_chars();

    c = getchar();

    if (c != '\n') {
        *error = true;
        skip_line();
    }

    if (!(*error)) {
        printf("%lu\n", gamma_busy_fields(g, player));
    }
    else {
        print_error(*line);
    }
}

/**
 * Funkcja pomocnicza realizująca polecenie wykonania ruchu.
 * Jeśli polecenie jest poprawne, to wypisuje 1, jeśli ruch się udał albo
 * 0, gdy ruch się nie udał.
 * W razie niepoprawnego polecenia wypisuje stosowny błąd na stderr
 * zgodni ze specyfikacją zadania.
 *
 * @param g     - Wskaźnik na planszę do gry w Gamma.
 * @param error - Wskaźnik na zmienną trzymającą informację o poprawności
 * aktualnego polecenia.
 * @param line  - Wskaźnik na zmienną trzymającą aktualny numer wiersza.
 */
static void move_command(gamma_t *g, bool *error, const size_t *line) {
    if (g == NULL) {
        *error = true;
        skip_line();
        return;
    }
    int c = getchar();
    if (!is_white(c)) {
        *error = true;
        if (!feof(stdin))
            ungetc(c, stdin);
    }
    read_white_chars();

    is_next_int(error);

    uint32_t player = read_uint32(error);

    read_white_chars();

    is_next_int(error);

    uint32_t x = read_uint32(error);

    read_white_chars();

    is_next_int(error);

    uint32_t y = read_uint32(error);

    read_white_chars();

    c = getchar();

    if (c != '\n') {
        *error = true;
        skip_line();
    }

    if (!(*error)) {
        printf("%d\n", gamma_move(g, player, x, y));
    }
    else {
        print_error(*line);
    }
}

/**
 * Funkcja pomocnicza realizująca polecenie wykonania złotego ruchu.
 * Jeśli polecenie jest poprawne, to wypisuje 1, jeśli złoty ruch się udał albo
 * 0 w przeciwnym wypadku.
 * Jeśli polecenie jest nipoprawne wypisuj błąd na stderr zgodnie ze
 * specyfikacją zadania.
 *
 * @param g     - Wskaźnik na planszę do gry w Gamma.
 * @param error - Wskaźnik na zmienną trzymającą informację o poprawności
 * aktualnego polecenia.
 * @param line  - Wskaźnik na zmienną trzymającą aktualny numer wiersza.
 */
static void golden_command(gamma_t *g, bool *error, const size_t *line) {
    int c = getchar();
    if (!is_white(c)) {
        *error = true;
        if (!feof(stdin))
            ungetc(c, stdin);
    }
    read_white_chars();
    is_next_int(error);
    uint32_t player = read_uint32(error);

    read_white_chars();
    is_next_int(error);

    uint32_t x = read_uint32(error);

    read_white_chars();
    is_next_int(error);

    uint32_t y = read_uint32(error);

    read_white_chars();

    c = getchar();
    if (c != '\n') {
        *error = true;
        skip_line();
    }

    if (!(*error)) {
        printf("%d\n", gamma_golden_move(g, player, x, y));
    }
    else {
        print_error(*line);
    }
}

/**
 * Funkcja pomocnicza realizująca polecenie golden_possible.
 * Jeśli poleceni jest poprawne, to wypisuje 1, gdy gracz może wykonać złoty
 * ruch albo 0 w przeciwnym wypadku.
 * Jeśli polecenie jest niepoprawne, to wypisuje stosowny błąd na stderr
 * zgodni ze specyfikacją zadania.
 *
 * @param g     - Wskaźnik na planszę do gry w Gamma.
 * @param error - Wskaźnik na zmienną trzymającą informację o poprawności
 * aktualnego polecenia.
 * @param line  - Wskaźnik na zmienną trzymającą aktualny numer wiersza.
 */
static void golden_possible_command(gamma_t *g,
        bool *error, const size_t *line) {
    int c = getchar();
    if (!is_white(c)) {
        *error = true;
        if (!feof(stdin))
            ungetc(c, stdin);
    }
    read_white_chars();
    is_next_int(error);

    uint32_t player = read_uint32(error);

    read_white_chars();

    c = getchar();

    if (c != '\n') {
        *error = true;
        skip_line();
    }

    if (!(*error)) {
        printf("%d\n", gamma_golden_possible(g, player));
    }
    else {
        print_error(*line);
    }
}

/**
 * Funkcja pomocnicza decydująca o wybraniu konkretnego polecenia na podstawie
 * @p c - pierwszego znaku w wierszu niebędącego komentarzem, ani
 * znakiem nowej linii.
 * Jeśli @p c nie odpowiada żadnemu ustalonemu wcześniej poleceniu, zostaje
 * wypisany stosowny komunikat o błędzie na stderr zgodni ze specyfikacją
 * zadania.
 */
static void choose_command(gamma_t *g, int c, size_t *line) {
    bool error = false;

    switch (c) {
        case 'm':
            move_command(g, &error, line);
            break;
        case 'g':
            golden_command(g, &error, line);
            break;
        case 'b':
            busy_fields_command(g, &error, line);
            break;
        case 'f':
            free_fields_command(g, &error, line);
            break;
        case 'q':
            golden_possible_command(g, &error, line);
            break;
        case 'p':
            print_command(g, line);
            break;
        default:
            print_error(*line);
            skip_line();
            break;
    }
}

void batch_mode(gamma_t *g, size_t *line) {
    int c;

    while (!feof(stdin) && (c = getchar()) != EOF) {
        if (c == '#')
            skip_line();
        else if (c != '\n')
            choose_command(g, c, line);

        ++(*line);
    }
    gamma_delete(g);
}