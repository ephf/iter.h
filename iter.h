/* iter.h - v1.0.0 - https://github.com/ephf/iter.h

	Iterators in C

	# Example

	```c
	#include <stdio.h>
	#include <assert.h>

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
*/

#ifndef ITER_H
#define ITER_H

#ifndef ITERDEF
#define ITERDEF
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Complete union of all iterator types. Call `.next()` to advance
// the iterator.
typedef union Iter Iter;
// Union of all peekable iterators. Saves the current value which can
// be accessed at `.peeked`
typedef union Peekable Peekable;

// Linear iterator. Goes along an array linearly until it runs out of
// items.
typedef struct LinIter {
	void* (*next)(struct LinIter* self);
	size_t item_size;
	uint8_t* bottom;
	size_t items_left;
} LinIter;

// Map iterator. Maps each value using a the `.mapfn()`.
typedef struct MapIter {
	void* (*next)(struct MapIter* self);
	Iter* child;
	void* (*mapfn)(void* item);
} MapIter;

// Chain iterator. Chains two iterators together.
typedef struct ChainIter {
	void* (*next)(struct ChainIter* self);
	Iter* first;
	Iter* second;
} ChainIter;

// Filter iterator. Only includes values from iterator if `.filter()`
// returns `true`.
typedef struct FilterIter {
	void* (*next)(struct FilterIter* self);
	Iter* child;
	bool (*filter)(void* item);
} FilterIter;

// Union of all peekable iterators. Saves the current value which can
// be accessed at `.peeked`
union Peekable {
	struct {
		void* (*next)(Peekable* self);
		void* peeked;
		Iter* child;
		void* _padding[1];
	};
};

// Complete union of all iterator types. Call `.next()` to advance
// the iterator.
union Iter {
	struct {
		void* (*next)(Iter* self);
		void* _padding[3];
	};

	LinIter _lin;
	MapIter _map;
	ChainIter _chain;
	FilterIter _filter;

	Peekable _peekable;
};

// Advances `struct LinIter`
ITERDEF static inline void* LinIter_next(LinIter* self) {
	if(!self->items_left) return NULL;
	self->items_left--;
	void* item = self->bottom;
	self->bottom += self->item_size;
	return item;
}

// Advances `struct MapIter`
ITERDEF static inline void* MapIter_next(MapIter* self) {
	void* item = self->child->next(self->child);
	return item ? self->mapfn(item) : NULL;
}

// Advances `struct ChainIter`
ITERDEF static inline void* ChainIter_next(ChainIter* self) {
	if(self->first) {
		void* item = self->first->next(self->first);
		if(item) return item;
		else self->first = NULL;
	}
	return self->second->next(self->second);
}

// Advances `struct FilterIter`
ITERDEF static inline void* FilterIter_next(FilterIter* self) {
	void* item;
	while((item = self->child->next(self->child))
			&& !self->filter(item));
	return item;
}

// Advances `union Peekable`
ITERDEF static inline void* Peekable_next(Peekable* self) {
	void* item = self->peeked;
	self->peeked = self->child->next(self->child);
	return item;
}

// Creates a new linear iterator `struct LinIter`.
ITERDEF static inline Iter liter(void* data, size_t count, size_t item_size) {
	return (Iter) { ._lin = {
		&LinIter_next, item_size, (uint8_t*) data, count,
	}};
}

// Consumes every item in `iter`.
ITERDEF static inline void foreach(Iter iter, void (*consumer)(void* item)) {
	for(void* item; (item = iter.next(&iter));) {
		consumer(item);
	}
}

// Creates a mapped iterator `struct MapIter` with `mapfn`.
ITERDEF static inline Iter map(Iter* child, void* (*mapfn)(void* item)) {
	return (Iter) { ._map = {
		&MapIter_next, child, mapfn,
	}};
}

// Counts the number of items in `iter`.
ITERDEF static inline size_t count(Iter iter) {
	size_t count = 0;
	while(iter.next(&iter)) count++;
	return count;
}

// Gets the `n`th item in `iter`.
ITERDEF static inline void* nth(Iter iter, size_t n) {
	for(void* item; (item = iter.next(&iter));) {
		if(!n--) return item;
	}
	return NULL;
}

// Combines two iterators together into a chained iterator
// `struct ChainIter`.
ITERDEF static inline Iter chain(Iter* first, Iter* second) {
	return (Iter) { ._chain = {
		&ChainIter_next, first, second,
	}};
}

// Filters the items in `child` with the `filter` function and creates
// a filtered iterator `struct FilterIter`.
ITERDEF static inline Iter filter(Iter* child, bool (*filter)(void* item)) {
	return (Iter) { ._filter = {
		&FilterIter_next, child, filter,
	}};
}

// Finds the first next in `iter` that matches the search criteria
// in `search`.
ITERDEF static inline void* find(Iter iter, bool (*search)(void* item)) {
	for(void* item; (item = iter.next(&iter));) {
		if(search(item)) return item;
	}
	return NULL;
}

// Creates a new peekable iterator `union Peekable`
ITERDEF static inline Peekable peekable(Iter* child) {
	Peekable peekable = {
		&Peekable_next, NULL, child,
	};
	(void) Peekable_next(&peekable);
	return peekable;
}

// Advances `peekable` if the condition `cond` is met.
ITERDEF static inline void* next_if(Peekable* peekable, bool (*cond)(void* item)) {
	return cond(peekable->peeked)
		? Peekable_next(peekable)
		: NULL;
}

// Inline `foreach()` alternative. Used like a `for`-loop.
#define foreachinl(constexpr_iter, arg_decl) \
	for(void* __item; (__item = \
				(constexpr_iter)->next(constexpr_iter));) \
	for(arg_decl = __item; __item; __item = NULL)

#endif
