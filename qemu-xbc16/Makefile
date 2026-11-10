CC ?= cc
CFLAGS ?= -std=c11 -O1 -g -Wall -Wextra -Wno-unused-parameter
CORE = overlay/target/xbc16/core.c
core-test: tests/core_test.c $(CORE) overlay/target/xbc16/core.h
	$(CC) $(CFLAGS) -fsanitize=undefined -fno-sanitize-recover=all tests/core_test.c $(CORE) -lm -o $@
text-test: tests/text_test.c overlay/hw/xbc16/text.h
	$(CC) $(CFLAGS) -fsanitize=undefined -fno-sanitize-recover=all tests/text_test.c -o $@
graphics-demo-test: tests/graphics_demo_test.c $(CORE) overlay/target/xbc16/core.h
	$(CC) $(CFLAGS) -fsanitize=undefined -fno-sanitize-recover=all tests/graphics_demo_test.c $(CORE) -lm -o $@
test: core-test text-test graphics-demo-test
	./core-test
	./text-test
	./graphics-demo-test examples/graphics.bin
clean:
	rm -f core-test core-runner text-test graphics-demo-test
.PHONY: test clean
