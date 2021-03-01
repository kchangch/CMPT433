# Makefile to create sorter application
# Date: February 25th, 2021
# Name: Kidjou Chang, Eric Sund

OUTFILE1 = sorter
OUTFILE2 = noworky
OUTDIR = $(HOME)/cmpt433/public/myApps
CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -pthread

all: sorter noworky done

sorter:
	$(CC_C) $(CFLAGS) sorter.c -o $(OUTDIR)/$(OUTFILE1)
	ls -l $(OUTDIR)/$(OUTFILE1)

noworky:
	$(CC_C) $(CFLAGS) noworky.c -o $(OUTDIR)/$(OUTFILE2)
	ls -l $(OUTDIR)/$(OUTFILE2)

done:
	@echo "Finished building application."

clean:
	rm $(OUTDIR)/$(OUTFILE1)
	rm $(OUTDIR)/$(OUTFILE2)