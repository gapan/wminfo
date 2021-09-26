LIBDIR = -L/usr/X11R6/lib
LIBS   = -lXpm -lXext -lX11
FLAGS = -O2
OBJS =	wminfo.o \
		../wmgeneral/wmgeneral.o \
		../wmgeneral/misc.o \
		../wmgeneral/list.o

.c.o:
	cc -I/usr/X11R6/share/include $(FLAGS) -c -Wall $< -o $*.o

wminfo: $(OBJS)
	cc $(FLAGS) -o wminfo $^ -lXext $(LIBDIR) $(LIBS)

all:: wminfo

clean::
	for i in $(OBJS) ; do \
		rm -f $$i ; \
	done
	rm -f wminfo
	rm -f *~
	rm -f core

install:: wminfo
	cp -f wminfo /usr/local/bin/
	chmod 755 /usr/local/bin/wminfo
	chown root:root /usr/local/bin/wminfo
	echo "wmInfo installed in /usr/local/bin"
