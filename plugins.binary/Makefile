PREFIX=/usr/local

BINDIR=$(PREFIX)/bin

make:
	for f in *.c ; do gcc -O2 -s $$f -o $${f%c}bin ; done

clean:
	for f in *.bin ; do rm $$f ; done

install:
	cp -p *.bin $(BINDIR)/ ; chmod 755 $(BINDIR)/*.bin ; chown root:root $(BINDIR)/*bin

uninstall:
	rm -f $(BINDIR)/*.bin

