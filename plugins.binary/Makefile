PREFIX=/usr/local

BINDIR=$(PREFIX)/bin

make:
	for f in *.c ; do gcc -O2 -s $$f -o $${f%c}bin ; done

install:
	chmod 755 *.bin ; chown root:root *.bin ; cp -p *.bin $(BINDIR)/

uninstall:
	rm -f $(BINDIR)/*.bin

clean:
	for f in *.bin ; do rm $$f ; done

