#include <stdio.h>
#include "iter.h"

unsigned __assertions_total;
unsigned __assertions_passed;
unsigned __tests_total = 0;
unsigned __tests_passed = 0;

void assert_eq(const char* as, const char* bs, size_t a, size_t b) {
	if(a != b) {
		fprintf(stderr,
				"\33[31;1massertion failed:\33[0m "
				"%s \33[35m%zu\33[0m != %s \33[35m%zu\33[0m\n",
				as, a, bs, b);
	} else {
		__assertions_passed++;
	}
	__assertions_total++;
}
#define assert_eq(a, b) assert_eq(#a, #b, a, b)

void __test_results(const char* name) {
	const bool passed = __assertions_total == __assertions_passed;
	fprintf(passed ? stdout : stderr,
			passed
				? "\33[42;30;1m Passed \33[0m"
				: "\33[41;30;1m Failed \33[0m");
	fprintf(passed ? stdout : stderr,
			"\33[1m %s \33[0;90m%d / %d passed\33[0m\n",
			name, __assertions_passed, __assertions_total);
	__tests_total++;
	__tests_passed += passed;
}
#define test(name) \
	for(bool _ = !(__assertions_total = __assertions_passed = 0); _; \
			_ = (__test_results(name), false))

[[gnu::destructor]]
void __final_message() {
	const bool passed = __tests_passed == __tests_total;
	fprintf(passed ? stdout : stderr,
			"%s%d / %d tests passed\33[0m\n",
			passed ? "\33[32;1m" : "\33[31;1m",
			__tests_passed, __tests_total);
}

void consumer1(void* item) {
	static int prev_number = 0;
	assert_eq(*(int*) item, ++prev_number);
}

void* mapfn1(void* item) {
	static int value;
	value = *(int*) item * 10;
	return &value;
}

bool filter1(void* item) {
	return *(int*) item % 2;
}

bool search1(void* item) {
	return *(int*) item == 4;
}

bool cond1(void* item) {
	return *(int*) item < 3;
}

int main() {
	int nums[] = { 1, 2, 3, 4, 5 };

	test("foreach & creation with liter() and foreach()") {
		Iter iter = liter(nums, 5, sizeof(int));

		foreach(iter, &consumer1);

		size_t i = 0;
		foreachinl(&iter, int* item) {
			assert_eq(*item, nums[i++]);
		}
	}

	test("mapping with map()") {
		Iter original = liter(nums, 5, sizeof(int));
		Iter mapped = map(&original, &mapfn1);

		int value = 0;
		foreachinl(&mapped, int* item) {
			assert_eq(*item, value += 10);
		}
	}

	test("counting iterator items with count()") {
		Iter iter = liter(nums, 5, sizeof(int));
		assert_eq(count(iter), 5);
	}

	test("indexing iterator with nth()") {
		for(int i = 0; i < 5; i++) {
			Iter iter = liter(nums, 5, sizeof(int));
			assert_eq(*(int*) nth(iter, i), nums[i]);
		}
	}

	test("chaining two iterators with chain()") {
		Iter first = liter(nums, 5, sizeof(int));

		int second_nums[] = { 6, 7, 8, 9, 10 };
		Iter second = liter(second_nums, 5, sizeof(int));

		Iter chained = chain(&first, &second);
		assert_eq(count(chained), 10);

		first = liter(nums, 5, sizeof(int));
		second = liter(second_nums, 5, sizeof(int));
		chained = chain(&first, &second);

		int value = 0;
		foreachinl(&chained, int* item) {
			assert_eq(*item, ++value);
		}
	}

	test("filtering with filter()") {
		Iter iter = liter(nums, 5, sizeof(int));
		Iter filtered = filter(&iter, &filter1);

		assert_eq(count(filtered), 3);

		iter = liter(nums, 5, sizeof(int));
		filtered = filter(&iter, &filter1);

		int value = 1;
		foreachinl(&filtered, int* item) {
			assert_eq(*item, value);
			value += 2;
		}
	}

	test("find a value in an iterator with find()") {
		Iter iter = liter(nums, 5, sizeof(int));
		int* four = find(iter, &search1);
		assert_eq(*four, 4);
	}

	test("peekable iterators with peekable() and next_if()") {
		Iter iter = liter(nums, 5, sizeof(int));
		Peekable piter = peekable(&iter);

		int value = 0;
		foreachinl(&piter, int* item) {
			assert_eq(*item, ++value);
			if(piter.peeked) {
				assert_eq(*(int*) piter.peeked, value + 1);
			}
		}

		iter = liter(nums, 5, sizeof(int));
		piter = peekable(&iter);

		int count = 0;
		while(next_if(&piter, &cond1)) count++;
		assert_eq(count, 2);
	}

	test("stress test with foreach()") {
		int* big_nums = malloc(sizeof(int) * 100000);
		for(int i = 0; i < 100000; i++) {
			big_nums[i] = i + 6;
		}

		Iter iter = liter(big_nums, 100000, sizeof(int));
		foreach(iter, &consumer1);

		free(big_nums);
	}
}
