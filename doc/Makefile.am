TARGETS	= nss-pgsql.html

all html: $(TARGETS)

install-data-local: $(TARGETS)
	$(mkinstalldirs) $(DESTDIR)$(docdir)
	$(INSTALL_DATA) nss-pgsql.html $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/caution.png $(DESTDIR)$(docdir)

clean-local:
	rm -f $(TARGETS)

nss-pgsql.html: config.xsl nss-pgsql.xml
	xmlto xhtml-nochunks -m $(srcdir)/config.xsl $(srcdir)/nss-pgsql.xml

