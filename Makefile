CC=gcc

.PHONY: build install uninstall clean

build: ssmi.c
	$(CC) -D_POSIX_C_SOURCE=2 -std=c99 -Wall -g -s ssmi.c -o ssmi -lm


install:
	@echo Installing ssmi ...
	@install -d /usr/bin
	@install -m 755 ssmi /usr/bin/
	@install -d /usr/share/man/man1
	@install -m 644 ssmi.1 /usr/share/man/man1/
	@echo Successfully installed

uninstall:
	@echo Uninstalling ssmi ...
	@rm -f /usr/bin/ssmi
	@rm -f /usr/share/man/man1/ssmi.1
	@echo Successfully uninstalled

clean:
	@rm  ssmi
