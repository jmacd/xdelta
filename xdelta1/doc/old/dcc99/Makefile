all: dcc99.dvi

# mkex mkex2 lcs

# mkex: mkex.c
# 	gcc -o mkex -Wall `glib-config --cflags` `glib-config --libs` -g mkex.c -lglib

# mkex2: mkex2.c
# 	gcc -o mkex2 -Wall `glib-config --cflags` `glib-config --libs` -g mkex2.c -lglib

# lcs: lcs.c
# 	gcc -o lcs -Wall -O lcs.c -lglib

LATEX=latex

dcc99.dvi: dcc99.tex dcc99.bib
	$(LATEX) dcc99.tex
	bibtex dcc99
	$(LATEX) dcc99.tex
