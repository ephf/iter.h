# iter.h

🔄 Iterators in C

## Example

```c
#include <stdio.h>
#include <assert.h>

#define ITER_IMPL
#include "iter.h"

bool filter_odds(int* item) {
    return *item % 2;
}

int main() {
    int numbers[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    // create iterator
    Iter numbers_iter = liter(numbers, 10, sizeof(int));

    // filter only odd numbers
    Iter filtered = filter(&numbers_iter, (void*) &filter_odds);
    assert(count(filtered) == 5);

    // reset both iterators
    numbers_iter = liter(numbers, 10, sizeof(int));
    filtered = filter(&numbers_iter, (void*) &filter_odds);

    // inline loop macro
    foreachinl(&filtered, int* odd_number) {
        assert(*odd_number % 2 == 1);
        printf("odd: %d\n", *odd_number);
    }
}
```

## Extra

All of the source code is in [iter.h](iter.h) and you can also find examples of function use in [test.c](test.c). Feel free to open an issue, request any additions, or open a pr.

Unit tests can be ran using `make`:

```bash
$ make test
```
