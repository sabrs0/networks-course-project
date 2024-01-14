CC=gcc
SRCPATH=./src/
OUTPATH=./out/
INCPATH=./inc/
CFLAGS= -std=gnu99 -c
.PHONY: clean
app.exe: $(OUTPATH) $(OUTPATH)logger.o $(OUTPATH)http.o $(OUTPATH)server.o $(OUTPATH)threadpool.o  $(OUTPATH)main.o
	mkdir -p $(OUTPATH)
	$(CC) -o  app.exe $(OUTPATH)logger.o $(OUTPATH)http.o $(OUTPATH)server.o $(OUTPATH)threadpool.o $(OUTPATH)main.o -lpthread
$(OUTPATH)main.o: $(SRCPATH)main.c $(INCPATH)server.h
	 $(CC) $(CFLAGS) -o $(OUTPATH)main.o $(SRCPATH)main.c
$(OUTPATH)threadpool.o: $(SRCPATH)threadpool.c $(INCPATH)threadpool.h $(INCPATH)logger.h
	 $(CC) $(CFLAGS) -o $(OUTPATH)threadpool.o  $(SRCPATH)threadpool.c
$(OUTPATH)server.o: $(SRCPATH)server.c $(INCPATH)server.h $(INCPATH)http.h
	 $(CC) $(CFLAGS) -o $(OUTPATH)server.o  $(SRCPATH)server.c
$(OUTPATH)http.o: $(SRCPATH)http.c $(INCPATH)http.h $(INCPATH)logger.h
	 $(CC) $(CFLAGS) -o $(OUTPATH)http.o  $(SRCPATH)http.c
$(OUTPATH)logger.o: $(SRCPATH)logger.c $(INCPATH)logger.h
	 $(CC) $(CFLAGS) -o $(OUTPATH)logger.o  $(SRCPATH)logger.c

clean:
	rm -fv *gcno
	rm -fv *exe
	rm -fv $(OUTPATH)*o