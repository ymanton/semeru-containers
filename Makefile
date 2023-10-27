all: pidplus

pidplus: CFLAGS=-g -O0 --coverage -Wpedantic -Werror

.PHONY: test

test: pidplus
	$(RM) pidplus.gcda
	sudo setcap cap_checkpoint_restore=eip ./pidplus
	./pidplus
	./pidplus --
	./pidplus -h
	./pidplus echo
	./pidplus -- echo
	./pidplus sh -c 'echo "My pid is $$$$ and my parent is $$PPID"'
	./pidplus -- sh -c 'echo "My pid is $$$$ and my parent is $$PPID"'
	./pidplus -p 10000 sh -c 'echo "My pid is $$$$ and my parent is $$PPID"'
	./pidplus -p 10001 -- sh -c 'echo "My pid is $$$$ and my parent is $$PPID"'
	./pidplus -p 0 echo && false || true
	./pidplus -p -1 echo && false || true
	./pidplus -p 1 echo && false || true
	./pidplus -p -9223372036854775808 echo && false || true
	./pidplus -p -9223372036854775809 echo && false || true
	./pidplus -p 9223372036854775807 echo && false || true
	./pidplus -p 9223372036854775808 echo && false || true
	./pidplus -p badpid echo && false || true
	./pidplus -p 1badpid echo && false || true
	./pidplus -x echo && false || true
	./pidplus -x 10000 echo && false || true
	./pidplus badprog && false || true
	gcov pidplus.c
