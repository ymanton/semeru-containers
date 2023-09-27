all: pidplus

.PHONY: test

test: pidplus
	sudo setcap cap_checkpoint_restore=eip ./pidplus
	./pidplus
	./pidplus --
	./pidplus echo
	./pidplus -- echo
	./pidplus sh -c 'echo "My pid is $$$$ and my parent is $$PPID"'
	./pidplus -- sh -c 'echo "My pid is $$$$ and my parent is $$PPID"'
	./pidplus -p 10000 sh -c 'echo "My pid is $$$$ and my parent is $$PPID"'
	./pidplus -p 10000 -- sh -c 'echo "My pid is $$$$ and my parent is $$PPID"'
