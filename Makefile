all: pidplus

pidplus: CFLAGS=-g -O0 --coverage -Wpedantic -Werror

.PHONY: test

test: pidplus
	$(RM) pidplus.gcda
	sudo setcap cap_checkpoint_restore=eip ./pidplus
	./pidplus && false || true
	./pidplus -- && false || true
	./pidplus echo && false || true
	./pidplus -- echo && false || true
	./pidplus sh -c 'echo "My pid is $$$$ and my parent is $$PPID"' && false || true
	./pidplus -- sh -c 'echo "My pid is $$$$ and my parent is $$PPID"' && false || true
	./pidplus 10000 sh -c 'echo "My pid is $$$$ and my parent is $$PPID"'
	./pidplus 10001 -- sh -c 'echo "My pid is $$$$ and my parent is $$PPID"' && false || true
	./pidplus 10002 && false || true
	./pidplus 0 echo && false || true
	./pidplus -1 echo && false || true
	./pidplus 1 echo && false || true
	./pidplus -9223372036854775808 echo && false || true
	./pidplus -9223372036854775809 echo && false || true
	./pidplus 9223372036854775807 echo && false || true
	./pidplus 9223372036854775808 echo && false || true
	./pidplus badpid echo && false || true
	./pidplus 1badpid echo && false || true
	./pidplus 10003 badprog && false || true
	gcov pidplus.c
