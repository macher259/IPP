/**
 * @file
 * Implementacja silnika do gry w gamma.
 */

#include "gamma.h"
#include "board_field_type.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define EMPTY 0 /**< Używam numeru 0 jako numeru pustego gracza. Powoduje
 * to, że numeracja gracza o numerze 2^32 - 1 może nie działać prawidłowo dla
 * wszystkich funkcji. Pozwalam sobie na taką swawolę, bo uważam, że
 * istnienie takiego gracza jest niemożliwe ze względów technicznych, takich
 * jak brak pamięci. */
#define LOG_BASE 10 ///< Baza logarytmu używana w kodzie.

/**
 * Struktura reprezentująca planszę do gry w gamma.
 */
struct gamma {
    uint32_t height; ///< Ilość wierszy planszy.
    uint32_t width; ///< Ilość kolumn planszy.

    field_t **fields; ///< Tablica dwuwymiarowa pól planszy.

    uint32_t areas; ///< Maksymalna ilość obszarów, którą może mieć gracz.
    uint32_t players; ///< Liczba graczy.
    uint32_t *player_areas; ///< Tablica przechowująca ilość obszarów graczy.
    uint64_t *player_fields; ///< Tablica przechowująca ilość pól graczy.
    bool *golden_used; /**< Tablica przechowująca informację, czy dany gracz
    wykorzystał złoty ruch. */

    bool no_memory; /**< Zmienna przechowująca informacje, czy skończyła się
    pamięć. */
};

/**
 * Funkcja pomocnicza dealokująca pamięć dla pola [fields] struktury [gamma_t].
 * Ustawia wartość wskaźnika [fields] na wartość NULL.
 * @param g - Wskaźnik na planszę, której jesteśmy w trakcie niszczenia.
 */
static void delete_fields(gamma_t *g) {
    if (g->fields == NULL)
        return;
    for (uint32_t i = 0; i < g->height; ++i)
        free(g->fields[i]);
    free(g->fields);
    g->fields = NULL;
}

void gamma_delete(gamma_t *g) {
    if (g != NULL) {
        delete_fields(g);
        if (g->player_areas != NULL) {
            free(g->player_areas);
            g->player_areas = NULL;
        }
        free(g->golden_used);
        free(g->player_fields);
        free(g);
    }
}

/**
 * Funkcja pomocnicza alokująca pamięć dla pola @p fields struktury
 * @ref gamma_t. W przypadku braku pamięci w trakcie działania, dealokują całą
 * dotychczas zaalokowaną w tej funkcji pamięć.
 * @param g - Wskaźnik na planszę, której jesteśmy w trakcie tworzenia.
 * @return Wskaźnik na tablicę dwuwymiarową typu @ref field_t w przypadku
 * powodzenia lub NULL w przypadku błędu(brak pamięci).
 */
static field_t** initialize_fields(gamma_t *g) {
    if (g == NULL || g->no_memory)
        return NULL;
    field_t **fields = NULL;
    size_t size = sizeof(field_t*) * g->height;
    fields = allocate_memory(size, &(g->no_memory));
    if (!g->no_memory) {
        uint32_t rows_alloced = 0;
        size = sizeof(field_t) * g->width;
        for (uint32_t i = 0; i < g->height; ++i) {
            ++rows_alloced;
            fields[i] = allocate_memory(size, &(g->no_memory));
            if (!g->no_memory) {
                for (uint32_t j = 0; j < g->width; ++j) {
                    initialize_field(&(fields[i][j]), i, j, EMPTY);
                }
            }
        }
        if (g->no_memory) {
            for (uint32_t i = 0; i < g->height; ++i) {
                free(fields[i]);
            }
            free(fields);
            fields = NULL;
        }
    }
    return fields;
}

/**
 * Pomocnicza funkcja alokująca pamięć dla pola player_areas struktury
 * @ref gamma_t. W razie powodzenia ustawia wartość każdego elementu tablicy
 * player_areas na wartość 0.
 * @param g - Wskaźnik na planszę, której jesteśmy aktualnie w trakcie
 * tworzenia.
 * @return Wskaźnik na pierwszy element zaalokowanej właśnie tablicy typu
 * uint32_t w razie powodzenia lub NULL w razie braku błędu(brak pamięci).
 */
static uint32_t* initialize_player_areas(gamma_t *g) {
    if (g == NULL || g->no_memory)
        return NULL;

    uint32_t *player_areas = NULL;
    size_t size = sizeof(uint32_t) * (g->players + 1);
    player_areas = allocate_memory(size, &(g->no_memory));

    if (!g->no_memory)
        memset(player_areas, 0, size);

    return player_areas;
}

/**
 * Funkcja pomocnicza alokująca pamięć dla pola player_fields struktury
 * @ref gamma_t. W razie powodzenia ustawia wartość każdego elementu
 * player_fields na wartość 0.
 * @param g - Wskaźnik na planszę, której jesteśmy aktualnie w trakcie
 * tworzenia.
 * @return Wskaźnik na pierwszy element nowo zaalokowanej tablicy w razie
 * powodzenia lub NULL w razie błędu(brak pamięci).
 */
static uint64_t* initialize_player_fields(gamma_t *g) {
    if (g == NULL || g->no_memory)
        return NULL;

    uint64_t *player_fields = NULL;
    size_t size = sizeof(uint64_t) * (g->players + 1);
    player_fields = allocate_memory(size, &(g->no_memory));

    if (!g->no_memory)
        memset(player_fields, 0, size);

    return player_fields;
}

/**
 * Funkcja pomocnicza alokująca pamięć dla pola golden_used struktury
 * @ref gamma_t. W razie powodzenia ustawia każdy element golden_used na
 * wartość false.
 * @param g - Wskaźnik na planszę, której jesteśmy aktualnie w trakcie
 * tworzenia.
 * @return Wskaźnik na pierwszy element nowo zaalokowanej tablicy typu
 * bool w razie powodzenia lub NULL w przypadku błędu(brak pamięci).
 */
static bool* initialize_golden_used(gamma_t *g) {
    if (g == NULL || g->no_memory)
        return NULL;

    bool *golden = NULL;
    size_t size = sizeof(bool) * (g->players + 1);
    golden = allocate_memory(size, &(g->no_memory));

    if (!g->no_memory)
        memset(golden, false, size);

    return golden;
}

gamma_t* gamma_new(uint32_t width, uint32_t height,
                   uint32_t players, uint32_t areas) {
    if (width == 0 || height == 0 || players == 0 || areas == 0)
        return NULL;
    if (players == UINT32_MAX)
        return NULL;
    bool no_memory = false;
    gamma_t *g = allocate_memory(sizeof(gamma_t), &no_memory);

    if (!no_memory) {
        g->height = height;
        g->width = width;
        g->players = players;
        g->areas = areas;
        g->no_memory = no_memory;
        g->fields = initialize_fields(g);
        g->player_areas = initialize_player_areas(g);
        g->golden_used = initialize_golden_used(g);
        g->player_fields = initialize_player_fields(g);
    }

    if (g != NULL && g->no_memory) {
        gamma_delete(g);
        g = NULL;
    }
    return g;
}

/**
 * Funkcja pomocnicza sprawdzająca czy dane współrzędne znajdują się na danej
 * planszy.
 * @param g - Wskaźnik na planszę.
 * @param x - Numer kolumny.
 * @param y - Numer wiersza.
 * @return true jeśli pole z danym numerem kolumny i wiersza istnieje na
 * danej planszy, false w przeciwnym wypadku.
 */
static bool good_coords(gamma_t *g, uint32_t x, uint32_t y) {
    uint32_t height = g->height;
    uint32_t width = g->width;
    return (x < width && y < height);
}

/**
 * Sprawdza, czy dany ruch z danymi specyfikacjami jest możliwy w danym
 * momencie.
 * @param g - Wskaźnik na planszę.
 * @param player - Numer gracza, który chce wykonać ruch.
 * @param x - Numer kolumny.
 * @param y - Numer wiersza.
 * @return true w przypadku gdy jest możiwe wykonanie danego ruchu lub false,
 * gdy taki ruch jest niedozwolony lub któryś z parametrów jest błędny.
 */
bool movie_possible(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    if (g == NULL || player == EMPTY || player > g->players || x >= g->width ||
        y >= g->height || g->fields[y][x].owner != EMPTY) {
        return false;
    }


    if (g->player_areas[player] == g->areas) {
        int delta_x[] = {1, -1, 0, 0};
        int delta_y[] = {0, 0, 1, -1};

        for (int i = 0; i < 4; ++i) {
            if (good_coords(g, x + delta_x[i], y + delta_y[i]) &&
            g->fields[y + delta_y[i]][x + delta_x[i]].owner == player)
                return true;
        }
        return false;

    }
    return true;
}


bool gamma_move(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    if (!movie_possible(g, player, x, y))
        return false;
    set_field(&(g->fields[y][x]), x, y, player);

    int delta_x[] = {1, -1, 0, 0};
    int delta_y[] = {0, 0, 1, -1};

    bool connected = false;
    int united_areas = 0;

    for (int i = 0; i < 4; ++i) {
        if (good_coords(g, x + delta_x[i], y + delta_y[i]) &&
            g->fields[y + delta_y[i]][x + delta_x[i]].owner == player) {

            connected = true;
            if (find_root(&(g->fields[y][x]))->rep !=
                find_root(&(g->fields[y + delta_y[i]][x + delta_x[i]]))->rep) {
                united_areas++;
            }
            unite(g->fields[y][x],
                    g->fields[y + delta_y[i]][x + delta_x[i]]);
        }
    }
    if (!connected)
        ++(g->player_areas[player]);


    if (united_areas > 0)
        g->player_areas[player] -= (united_areas - 1);

    ++(g->player_fields[player]);

    return true;
}

uint64_t gamma_busy_fields(gamma_t *g, uint32_t player) {
    if (g == NULL || player == EMPTY || player > g->players)
        return 0;
    return g->player_fields[player];
}

/**
 * Funkcja pomocnicza oznaczająca warunek konieczny dla użycia złotego ruchu.
 * Warunek konieczny sprawdza podstawowe warunki potrzebne do zajęcia pola
 * oraz to, czy istnieje pole zajęte przez innego gracza oraz to, czy gracz
 * @p player nie wykonał jeszcze złotego ruchu.
 * @param g         - wskaźnik na strukturę planszy do gry gamma
 * @param player    - numer gracza pytającego o złoty ruch
 * @return          - true, jeśli @p player spełnia warunek konieczny.
 *                    false w przeciwnym wypadku.
 */
static bool old_golden_possible(gamma_t *g, uint32_t player) {
    if (g == NULL || player == EMPTY ||
        player > g->players || g->golden_used[player] == true)
        return false;

    for (uint32_t i = 0; i <= g->players; ++i)
        if (i != player && g->player_fields[i] > 0)
            return true;

    return false;
}

/**
 * Pomocnicza funkcja sprawdzająca ilość miejsc, w której gracz @p player może
 * postawić pionka jeśli @p player ma maksymalną ilość obszarów. Sprawdzenie
 * przebiega w sposób brutalny sprawdzając każde pole na planszy, czy jest
 * możliwe do postawienia pionka na nim.
 * @param g - Wskaźnik na planszę.
 * @param player - Numer gracza, który ma maksymalną ilość obszarów.
 * @return Ilość obszarów, na które @p player może jeszcze postawić pionka,
 * która jest liczbą nieujemną.
 */
static uint64_t free_fields_full_areas(gamma_t *g, uint32_t player) {

    int delta_x[] = {1, -1, 0, 0};
    int delta_y[] = {0, 0, 1, -1};

    uint64_t free_count = 0;

    for (uint32_t i = 0; i < g->height; ++i) {
        for (uint32_t j = 0; j < g->width; ++j) {
            if (g->fields[i][j].owner == EMPTY) {
                for (int dir = 0; dir < 4; ++dir) {
                    uint32_t x = j + delta_x[dir];
                    uint32_t y = i + delta_y[dir];
                    if (good_coords(g, x, y) &&
                        g->fields[y][x].owner == player) {
                        ++free_count;
                        break;
                    }
                }
            }
        }
    }
    return free_count;
}

uint64_t gamma_free_fields(gamma_t *g, uint32_t player) {
    if (g == NULL || player == EMPTY || player > g->players)
        return 0;

    if (g->player_areas[player] == g->areas)
        return free_fields_full_areas(g, player);
    uint64_t free_fields = g->height;
    free_fields *= g->width;

    for (uint32_t i = 1; i <= g->players; ++i) {
        free_fields -= g->player_fields[i];
    }
    return free_fields;
}

/**
 * Funkcja pomocnicza obliczająca podłogę z logarytmu o podstawie @ref
 * LOG_BASE z liczby @p n.
 * @param n - Liczba, którą chcemy zlogarytmować.
 * @return Zwraca podłogę z logarytmu liczby n.
 */
static uint32_t logarithm(uint32_t n) {
    uint32_t counter = 0;
    while (n > 0) {
        n /= LOG_BASE;
        ++counter;
    }
    return counter;
}

/**
 * Funkcja pomocnicza uzupełniająca string @p board w przypadku, gdy liczba
 * graczy > 9.
 * @param g - Wskaźnik na planszę do gry gamma.
 * @param board - Dynamicznie zaalokowany string.
 * @param size - Liczba cyfr liczby graczy.
 * @param position - Pozycja aktualnego znaku.
 */

static void print_many_players(gamma_t *g, char *board,
                               uint32_t size, uint64_t *position) {
    char format_empty[] = {'%', size + 1 + '0', 'c', '\0'};
    char format_empty9[] = "%10c";
    char format_empty10[] = "%11c";
    char format[] = {'%', size + 1 + '0', 'u', '\0'};
    char format9[] = "%10u";
    char format10[] = "%11u";


    for (uint32_t i = 0; i < g->height; ++i) {
        for (uint32_t j = 0; j < g->width; ++j) {
            uint32_t player = g->fields[g->height - 1 - i][j].owner;
            if (size < 9) {
                if (player == EMPTY)
                    sprintf(board + (*position), format_empty, '.');
                else
                    sprintf(board + (*position), format, player);
            }
            else if (size == 9) {
                if (player == EMPTY)
                    sprintf(board + (*position), format_empty9, '.');
                else
                    sprintf(board + (*position), format9, player);
            }
            else {
                if (player == EMPTY)
                    sprintf(board + (*position), format_empty10, '.');
                else
                    sprintf(board + (*position), format10, player);
            }

            (*position) += size + 1;
        }
        board[*position] = '\n';
        ++(*position);
    }
    board[*position] = '\0';
}

/**
 * Funkcja pomocnicza uzupełniająca string @p board, gdy liczba gracz jest
 * mniejsza lub równa 9.
 * @param g - Wskaźnik na planszę do gry.
 * @param board - Dynamicznie zaalokowany string.
 * @param position - Pozycja aktualnego znaku.
 */
static void print_little_players(gamma_t *g, char *board, uint64_t *position) {
    for (uint32_t i = 0; i < g->height; ++i) {
        for (uint32_t j = 0; j < g->width; ++j) {
            char c;
            if (g->fields[g->height - 1 - i][j].owner != EMPTY)
                c = g->fields[g->height - 1 - i][j].owner + '0';
            else
                c = '.';
            board[*position] = c;
            ++(*position);
        }
        board[*position] = '\n';
        ++(*position);
    }
    board[*position] = '\0';
}

char* gamma_board(gamma_t *g) {
    if (g == NULL || g->no_memory)
        return NULL;
    uint32_t size = 0;
    size = logarithm(g->players);
    size_t ptr_size = (g->width * g->height * (size + 1) * sizeof(char) + 1 +
            g->height);
    char *board = allocate_memory(ptr_size, &(g->no_memory));
    if (g->no_memory)
        return NULL;
    uint64_t position = 0;
    if (size > 1) {
        print_many_players(g, board, size, &position);
        char *new_board = realloc(board, position + 1);
        board = new_board;
    }
    else {
        print_little_players(g, board, &position);
    }

    return board;
}

/**
 * Funkcja pomocnicza resetująca reprezentantów i stopień kaźdego pola
 * planszy @p g, które należy do gracza @p player. Ustawia ilość obszarów i pól
 * gracza @p player na wartość 0.
 * @param g - Wskaźnik na planszę.
 * @param player - Numer gracza, którego chcemy 'zresetować'.
 */
static void golden_reset_field_reps(gamma_t *g, uint32_t player) {
    for (uint32_t i = 0; i < g->height; ++i) {
        for (uint32_t j = 0; j < g->width; ++j) {
            if (g->fields[i][j].owner == player) {
                g->fields[i][j].rep = &(g->fields[i][j]);
                g->fields[i][j].rank = 0;
            }
        }
    }
    g->player_areas[player] = 0;
    g->player_fields[player] = 0;
}

/**
 * Funkcja pomocnicza symulująca ruch gracza @p player na pole o współrzędnych
 * (@p x, @p y. Wiemy, że wcześniej ruch był poprawnie wykonany więc nie
 * sprawdzamy już parametrów pod
 * względem poprawności. Pozwalamy na to, żeby liczba obszarów gracza
 * @p player przekroczyła po takim symulowanym ruchu maksymalną liczbę obszarów.
 * @param g - Wskaźnik na planszę.
 * @param player - Numer gracza, któ©ego ruch symulujemy.
 * @param x - Numer kolumny pola, w które chcemy ustawić pionka.
 * @param y - Numer wiersza pola, w które chcemy ustawić pionka.
 */
static void restore_move(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    set_field(&(g->fields[y][x]), x, y, player);

    int delta_x[] = {1, -1, 0, 0};
    int delta_y[] = {0, 0, 1, -1};

    int united_areas = 0;

    for (int i = 0; i < 4; ++i) {
        if (good_coords(g, x + delta_x[i], y + delta_y[i]) &&
            g->fields[y + delta_y[i]][x + delta_x[i]].owner == player) {
            if (find_root(&(g->fields[y][x])) !=
                find_root(&(g->fields[y + delta_y[i]][x + delta_x[i]]))) {
                united_areas++;
            }
            unite(g->fields[y][x],
                  g->fields[y + delta_y[i]][x + delta_x[i]]);
        }
    }
    if (united_areas == 0)
        ++(g->player_areas[player]);

    if (united_areas > 0)
        g->player_areas[player] -= (united_areas - 1);

    ++(g->player_fields[player]);

}

/**
 * Funkcja pomocnicza, która symuluje ciąg ruchów wykonanych przez gracza
 * @p player.
 * @param g - Wskaźnik na planszę.
 * @param player - Numer gracza, któremu 'zabraliśmy' pola w celu wykonania
 * złotego ruchu.
 */
static void golden_set_other_field_reps(gamma_t *g, uint32_t player) {
    for (uint32_t i = 0; i < g->height; ++i) {
        for (uint32_t j = 0; j < g->width; ++j) {
            if (g->fields[i][j].owner == player) {
                restore_move(g, player, j, i);
            }
        }
    }
}

bool gamma_golden_move(gamma_t *g, uint32_t player, uint32_t x, uint32_t y) {
    if (!old_golden_possible(g, player) || !good_coords(g, x, y) ||
        g->fields[y][x].owner == EMPTY || g->fields[y][x].owner == player ||
        g->golden_used[player] == true)
        return false;

    uint32_t changed_player = g->fields[y][x].owner;

    golden_reset_field_reps(g, changed_player);
    g->fields[y][x].owner = EMPTY;
    golden_set_other_field_reps(g, changed_player);


    if (g->player_areas[changed_player] > g->areas) {
        restore_move(g, changed_player, x, y);
        return false;
    }

    if (gamma_move(g, player, x, y)) {
        g->golden_used[player] = true;
        return true;
    }

    restore_move(g, changed_player, x, y);
    return false;
}

/**
 * Funkcja pomocnicza sprawdzająca, czy w przypadku gdy gracz @p player ma
 * maksymalną liczbę obszarów, to może wykonać gdzieś złoty ruch.
 * Sprawdzane jest to metodą brutalną.
 * @param g         - wskaźnik na strukturę planszy do gry gamma
 * @param player    - gracz pytający o możliwość złotego ruchu
 * @return          - true, jeśli istnieje pole możliwe do zajęcia złotym ruchem
 *                    bez przekraczania maksymalnej liczby obszarów.
 */
bool golden_wont_exceed_areas(gamma_t *g, uint32_t player) {
    uint32_t width = g->width;
    uint32_t height = g->height;

    bool wont_exceed_max_areas = false;
    for (uint32_t i = 0; i < height && !wont_exceed_max_areas; ++i) {
        for (uint32_t j = 0; j < width && !wont_exceed_max_areas; ++j) {
            if (g->fields[i][j].owner == player ||
                g->fields[i][j].owner == EMPTY)
                continue;

            uint32_t field_owner = g->fields[i][j].owner;

            if (gamma_golden_move(g, player, j, i)) {
                wont_exceed_max_areas = true;
                g->golden_used[player] = false;

                bool field_owner_golden_used = g->golden_used[field_owner];
                g->golden_used[field_owner] = false;
                gamma_golden_move(g, field_owner, j, i);
                g->golden_used[field_owner] = field_owner_golden_used;
            }
        }
    }
    return wont_exceed_max_areas;
}

bool gamma_golden_possible(gamma_t *g, uint32_t player) {
    bool necessary_condition = old_golden_possible(g, player);
    if (!necessary_condition)
        return false;
    else if (g->player_areas[player] < g->areas)
        return true;
    else
        return golden_wont_exceed_areas(g, player);
}

/**
 * Funkcja pomocnicza wypisująca planszę na stdout dla <= 9 graczy. Nie
 * obsługuje planszy @p g = NULL.
 * Podswietla pole wyznaczone przez współrzędne (x,y).
 * @param g     - wskaźnik na strukturę przechowującą grę.
 * @param x     - współrzędna pola do wyróżnienia.
 * @param y     - współrzdna pola do wyróżnienia.
 */
static void print_little(gamma_t *g, uint32_t x, uint32_t y) {
    for (uint32_t i = 0; i < g->height; ++i) {
        for (uint32_t j = 0; j < g->width; ++j) {
            char c;
            if (i == y && j == x)
                printf("\033[44m");
            if (g->fields[g->height - 1 - i][j].owner != EMPTY)
                c = g->fields[g->height - 1 - i][j].owner + '0';
            else
                c = '.';
            printf("%c", c);
            if (i == y && j == x)
                printf("\033[0m");
        }
        printf("\n");
    }
}

/**
 * Funkcja pomocnicza wypisująca planszę na stdout dla > 9 graczy. Nie obsługuje
 * planszy @p g = NULL.
 * Podswietla pole wyznaczone przez współrzędne (x,y).
 * @param size  - szerokość tekstowej reprezentacji gracza.
 * @param g     - wskaźnik na strukturę przechowującą grę.
 * @param x     - współrzędna pola do wyróżnienia.
 * @param y     - współrzdna pola do wyróżnienia.
 */
static void print_big(gamma_t *g, uint32_t size, uint32_t x, uint32_t y) {

    for (uint32_t i = 0; i < g->height; ++i) {
        for (uint32_t j = 0; j < g->width; ++j) {
            uint32_t player = g->fields[g->height - 1 - i][j].owner;
            if (i == y && j == x)
                printf("\033[45m");
            if (player == EMPTY)
                printf("%*c", size + 1, '.');
            else
                printf("%*d", size + 1, player);
            if (i == y && j == x)
                printf("\033[0m");

        }
        printf("\n");
    }
}

uint32_t gamma_print_board(gamma_t *g, uint32_t x, uint32_t y) {
    if (g == NULL)
        return 0;
    uint32_t size = 0;
    size = logarithm(g->players);

    if (size > 1)
        print_big(g, size, x, y);
    else
        print_little(g, x, y);

    return size + (size > 1);
}

uint32_t gamma_get_players(gamma_t *g) {
    return g->players;
}

uint32_t gamma_get_width(gamma_t *g) {
    return g->width;
}

uint32_t gamma_get_height(gamma_t *g) {
    return g->height;
}