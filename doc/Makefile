# the compilers
# everything you need is in package debiandoc-sgml
TXTC=sgml2txt
HTMLC=sgml2html
PSC=sgml2ps
PDFC=sgml2pdf

OPTS=

SRCS=$(wildcard *.sgml)
TXTDOCS=$(SRCS:.sgml=.txt)
HTMLDOCS=$(SRCS:.sgml=.html)
PSDOCS=$(SRCS:.sgml=.ps)
PDFDOCS=$(SRCS:.sgml=.pdf)
DOCS=$(TXTDOCS) $(HTMLDOCS) $(PSDOCS) $(PDFDOCS)

.PHONY:default text html postscript pdf all clean

default:text

text:$(TXTDOCS)

textov:$(TOVDOCS)

latex:$(TEXDOCS)

info:$(TEXIDOCS)

html:$(HTMLDOCS)

postscript:$(PSDOCS)

pdf:$(PDFDOCS)

all:$(DOCS)

%.txt:%.sgml
	$(TXTC) $(OPTS) $<

%.tov:%.sgml
	$(TOVC) $(OPTS) $<

%.tex:%.sgml
	$(TEXC) $(OPTS) $<

%.texinfo:%.sgml
	$(TEXIC) $(OPTS) $<

%.html:%.sgml
	$(HTMLC) $(OPTS) $<

%.ps:%.sgml
	$(PSC) $(OPTS) $<

%.pdf:%.sgml
	$(PDFC) $(OPTS) $<

clean:
	rm -rf $(DOCS)
