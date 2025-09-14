test:
	mkdir -p out
	$(CC) test.c -DITER_IMPL -o out/test
	./out/test
