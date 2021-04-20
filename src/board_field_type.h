/**
 * @file
 * Interfejs struktury reprezentującej pole na planszy do gry w gamma wraz z
 * odpowiednimi funkcjami.
 */

#ifndef GAMMA_BOARD_FIELD_TYPE_H
#define GAMMA_BOARD_FIELD_TYPE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * Struktura reprezentująca pojedyńcze pole planszy do gry gamma.
 */
typedef struct field {
    uint32_t x; ///<Numer kolumny, w którym jest pole. Liczba nieujemna.
    uint32_t y; ///< Numer wiersza, w którym jest pole. Liczba nieujemna.
    uint32_t owner; /**< Właściciel danego pola(ma pionek na nim). Liczba
    nieujemna. */
    struct field *rep; /**<Reprezentant pola. Początkowo każde pole jest
    swoim reprezentantem.*/
    uint64_t rank; /**<Stopień pola, tj. liczba jego dzieci w drzewie
    reprezentantów. */
} field_t;

//typedef struct field field_t;

/**
 * Funkcja inicjalizująca pole wskazywane przez @p field. Ustawia pole x, y,
 * player na wartości równe odpowiednim argumentom funkcji. Ustawia pole rank
 * struktury @ref field_t na 0 i wartość pola rep na wartość adresu w pamięci
 * danego pola [field].
 * @param field : Wskaźnik na pole na planszy.
 * @param x : Numer kolumny pola.
 * @param y : Numer wiersza pola.
 * @param player : Numer gracza, który będzie właścicielem pola.
 */
void initialize_field(field_t *field, uint32_t x, uint32_t y, uint32_t player);

/**
 * Funkcja alokująca pamięć rozmiaru @p size. W razie niepowodzenia ustawia
 * wartość zmiennej wskazywanej przez @p error na wartość true i ustawia
 * errno na ENOMEM.
 * @param size - Rozmiar segmentu pamięci, który chcemy zaalokować.
 * @param error - Wskaźnik na zmienną, która trzyma status wystąpienia błędu.
 * @return  Wskaźnik na nowo zaalokowany segment pamięcci lub wartość NULL w
 * przypadku błędu(brak pamięci).
 */
void* allocate_memory(size_t size, bool *error);

/**
 * Funkcja znajdująca korzeń drzewa reprezentantów pola wskazywanego na przez
 * @p field. Używana w celu rozróżnienia obszarów, do których pola planszy
 * należą.
 * @param field : Wskaźnik na pole planszy.
 * @return Wskaźnik na pole, które jest korzeniem drzewa reprezentantów, do
 * którego @p field należy.
 */
field_t* find_root(field_t *field);

/**
 * Funkcja złączająca w jeden obszar pola @p first i @p second na zasadzie
 * algorytmu Union-Find, czyli połączeniu drzew reprezentatów tych pól.
 * @param first : Pierwsze pole planszy.
 * @param second : Drugie pole planszy.
 */
void unite(field_t first, field_t second);

/**
 * Funkcja ustawiająca wartośći dla planszy wskazywanej na przez @p field.
 * Różni się od @ref initialize_field tym, że ustawia tylko pola x, y, player.
 * @param field : Wskaźnik na pole planszy, którego pola chcemy zmienić.
 * @param x : Numer kolumny pola.
 * @param y : Numer wiersza pola.
 * @param player : Numer gracza, którego ustawiamy na właśiciela pola.
 */
void set_field(field_t *field, uint32_t x, uint32_t y, uint32_t player);


#endif //GAMMA_BOARD_FIELD_TYPE_H
