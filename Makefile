CC=gcc
SRCPATH=./src/
OUTPATH=./out/
INCPATH=./inc/
CFLAGS= -std=c99 -c
.PHONY: clean
app.exe: $(OUTPATH) $(OUTPATH)threadpool.o  $(OUTPATH)main.o
	mkdir -p $(OUTPATH)
	$(CC) -o  app.exe $(OUTPATH)threadpool.o $(OUTPATH)main.o -lpthread
$(OUTPATH)main.o: $(SRCPATH)main.c $(INCPATH)threadpool.h
	 $(CC) $(CFLAGS) -o $(OUTPATH)main.o $(SRCPATH)main.c
$(OUTPATH)threadpool.o: $(SRCPATH)threadpool.c $(INCPATH)threadpool.h
	 $(CC) $(CFLAGS) -o $(OUTPATH)threadpool.o  $(SRCPATH)threadpool.c
clean:
	rm -fv *gcno
	rm -fv *exe
	rm -fv $(OUTPATH)*o