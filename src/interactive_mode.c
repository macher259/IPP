/**
 * @file
 * Implementacja modułu obsługującego tryb interaktywny.
 * UWAGI:
 *  - Podświetlam pola, bo uważam to za bardziej estetyczne rozwiązanie i przy
 *  zmianie ustawień kursora zdarzały się błędy(ANSI escape codes dla kursora
 *  nie działały, jeśli były wypisywane na stdout, ale działały wypisywane na
 *  stderr. Nie znalazłem wytłumaczenia na to, więc wolę z tego nie korzystać.
 */
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "interactive_mode.h"
#include "gamma.h"

#define NORMAL 0 ///< Rprezentacja normalnego stanu gry.
#define START 55 ///< Reprezentacja początku gry.
#define ENDING 77 ///< Reprezentacja skończonej gry.
#define FIRST_ESCAPE '\x1b' ///< Pierwszy znak z ANSI escape codes.
#define SECOND_ESCAPE '[' ///< Drugi znak z ANSI escape codes.
#define NO_ARROW 0 ///< Zerowy poziom wczytania strzałki.
#define FIRST_ARROW 1 ///< Pierwszy poziom wczytania strzałki.
#define SECOND_ARROW 2 ///< Drugi poziom wczytania strzałki.
#define STARTING_PLAYER 1 ///< Gracz rozpoczynający grę.
#define START_ROW 0 ///< Numer pierwszego wiersza.
#define START_COL 0 ///< Numer pierwszej kolumny.
#define GAME_END_CHAR '\4' ///< Kod znaku ctrl + d.
#define BAD_TERMINAL (-1) ///< Nie można pobrać atrybutów okna terminala.

/**
 * Enum zawierający obsługiwane kody sygnałów wysyłanych przez użytkownika.
 */
enum INPUT_CODE {
    MOVE, GOLDEN, SKIP, END, ARROW_UP,
    ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT
};

/**
 * Funkcja czyszcząca ekran terminala.
 */
static void clear_screen() {
    fprintf(stderr, "\033[H\033[J");
}

/**
 * Funkcja wypisująca statystyki gracza @p player.
 * @param g         - wskaźnik na planszę do gry.
 * @param player    - numer aktualnego gracza.
 */
static void print_player_stats(gamma_t *g, uint32_t player) {
    printf("PLAYER: %u \x1B[091mBUSY FIELDS: %lu \x1B[92mFREE FIELDS: %lu",
            player,
            gamma_busy_fields(g,
            player),
            gamma_free_fields(g, player));
    if (gamma_golden_possible(g, player))
        printf(" \x1B[93mGOLDEN MOVE AVAILABLE");
    printf("\x1B[39m\n");
}

/**
 * Funkcja wypisująca planszę do gry.
 * @param g         - wskaźnik na planszę do gry.
 * @param padding   - rozmiar pola gracza, nie wliczając spacji dla > 9 graczy.
 * @param x         - współrzędna aktualnej kolumny.
 * @param y         - współrzędna aktualnego wiersza.
 */
static void print_board(gamma_t *g, uint32_t *padding, uint32_t x, uint32_t y) {
    *padding = gamma_print_board(g, x, y);
}

/**
 * Funkcja wypisująca podsumowanie gry, czyli ostateczną planszę i wyniki
 * graczy.
 * @param g     - wskaźnik na strukturę przechowywującą planszę do gry.
 */
static void print_summary(gamma_t *g) {
    gamma_print_board(g, -1, -1);
    int rainbow_number = 91;
    for (uint32_t i = 1; i <= gamma_get_players(g); ++i) {
        int change = i % 7;
        printf("PLAYER \x1B[%dm%u\x1B[39m SCORE:"
               "\x1B[%dm %lu\x1B[39m\n", rainbow_number + change, i,
               rainbow_number + change, gamma_busy_fields(g, i));
    }

}

/**
 * Funkcja obsługująca wejście w trybie interaktywnym
 * @return Sygnał wysłany przez użytkownika zgodnie z treścią zadania.
 */
static int take_input() {
    int c;
    int arrow_char = NO_ARROW;
    while ((c = getchar()) != GAME_END_CHAR) {
        if (c == ' ')
            return MOVE;
        else if (c == 'g' || c == 'G')
            return GOLDEN;
        else if (arrow_char == SECOND_ARROW) {
            if (c == 'A')
                return ARROW_UP;
            else if (c == 'B')
                return ARROW_DOWN;
            else if (c == 'C')
                return ARROW_RIGHT;
            else if (c == 'D')
                return ARROW_LEFT;
            else
                arrow_char = NO_ARROW;
        }
        else if (c == 'c' || c == 'C')
            return SKIP;
        else  if (c == FIRST_ESCAPE)
            arrow_char = FIRST_ARROW;
        else if (c == SECOND_ESCAPE && arrow_char == FIRST_ARROW)
            arrow_char = SECOND_ARROW;
        else
            arrow_char = NO_ARROW;
    }

    return END;
}

/**
 * Funkcja zmieniająca ustawienia terminala do gry w trybie interaktywnym.
 */
static bool enable_raw_mode(struct termios *orig) {
    if (tcgetattr(STDIN_FILENO, orig) == BAD_TERMINAL) {
        return false;
    }
    struct termios raw = *orig;

    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    fprintf(stderr, "\033[?25l");
    return true;
}

/**
 * Funkcja poruszająca się po osi Y.
 * @param g         - wskaźnik na planszę do gry gamma.
 * @param y         - wskaźnik na numer aktualnego wiersza.
 * @param command   - sygnał wysłany przez użytkownika.
 */
static void move_y(gamma_t *g, uint32_t *y, int command) {
    if (command == ARROW_UP && *y > START_ROW) {
        --(*y);
    }
    else if (command == ARROW_DOWN && *y < UINT32_MAX &&
            *y + 1  < gamma_get_height(g)) {
        ++(*y);
    }
}

/**
 * Funkcja poruszająca się po osi X.
 * @param g     - wskaźnik na planszę do gry w gamma.
 * @param x     - Wskaźnik na aktualny numer kolumny.
 * @param command   - sygnał wysłany przez użytkownika
 */
static void move_x(gamma_t *g, uint32_t *x, int command) {
    if (command == ARROW_RIGHT && *x <= UINT32_MAX - 1 &&
        *x  < gamma_get_width(g) - 1) {
        ++(*x);
    }
    else if (command == ARROW_LEFT && *x  >= START_COL + 1) {
        --(*x);
    }
}

/**
 * Funkcja obsługująca wykonanie tury dla gracza.
 * @param g     - wskaźnik na strukturę przechowywującą planszę do gry.
 * @param state - wskaźnik na zmienną przechowywującą stan gry.
 */
static void make_turn(gamma_t *g, int *state) {
    uint32_t width = gamma_get_width(g);
    uint32_t height = gamma_get_height(g);

    static uint32_t player = STARTING_PLAYER;
    static uint32_t x = START_COL;
    static uint32_t y = START_ROW;
    static uint32_t skip_count = 0;
    uint32_t padding;
    if (player > gamma_get_players(g))
        player = STARTING_PLAYER;
    if (skip_count == gamma_get_players(g))
        *state = ENDING;
    if (*state == START) {
        x = (width - 1) / 2;
        y = (height - 1) / 2;
        *state = NORMAL;
    }

    if (!gamma_golden_possible(g, player) &&
        gamma_free_fields(g, player) == 0) {
        ++player;
        ++skip_count;
        return;
    }

    int command;
    bool move_ended = false;

    do {
        clear_screen();
        print_board(g, &padding, x, y);
        print_player_stats(g, player);

        command = take_input();
        if (command == ARROW_UP || command == ARROW_DOWN)
            move_y(g, &y, command);
        if (command == ARROW_LEFT || command == ARROW_RIGHT)
            move_x(g, &x, command);
        if (command == MOVE && gamma_move(g, player, x,
                height - 1 - y))
            move_ended = true;
        if (command == GOLDEN && gamma_golden_move(g, player, x,
                height - 1 - y))
            move_ended = true;
        if (command == SKIP)
            move_ended = true;

    } while(command != END && !move_ended);
    if (command == END)
        *state = ENDING;

    ++player;
    skip_count = 0;
}

/**
 * Funkcja sprawdzająca, czy rozmiary terminala pozwalają na utworzenie
 * czytelnej rozgrywki.
 * @param g         - wskaźnik na planszę do gry gamma.
 * @param window    - struktura przechowująca rozmiary terminala.
 * @return          - true, jeśli rozmiary pozwalają na utworzenie czytelnej
 *                    rozgrywki, false w.p.p.
 */
static bool is_window_good_size(gamma_t *g, struct winsize window) {
    if (((uint32_t )window.ws_row) - 2 < gamma_get_height(g))
        return false;
    uint32_t size = gamma_print_board(g, -1, -1);
    clear_screen();
    if (window.ws_col < gamma_get_width(g) * size)
        return false;
    return true;
}

/**
 * Funkcja pomocnicza sprzątająca po programie w razie wczesnego błędu
 * uniemożliwiającego grę.
 * @param g         - wskaźnik na strukturę planszy do gry gamma.
 */
static void end_on_error(gamma_t *g) {
    gamma_delete(g);
    printf("\033[?25h");
    exit(1);
}

void interactive_mode(gamma_t *g) {
    struct termios orig;

    struct winsize window;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window) == -1) {
        printf("Error when trying to get terminal window size.\n");
        end_on_error(g);
    }

    if (!enable_raw_mode(&orig)) {
        printf("Error when trying to get terminal window attributes.\n");
        end_on_error(g);
    }

    if (!is_window_good_size(g, window)) {
        printf("Terminal windows too small\n");
        end_on_error(g);
    }

    int state = START;

    while (true) {
        make_turn(g, &state);
        if (state != NORMAL)
            break;
    }
    clear_screen();
    if (state == ENDING)
        print_summary(g);
    gamma_delete(g);
    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
    printf("\033[?25h");
}