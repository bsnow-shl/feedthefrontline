default: html/index.html manual.pdf

html/index.html: manual.xml
	rm -rf html
	db2html -d stylesheet.dsl manual.xml 
	cp stylesheet.css manual
	mv manual html
	for i in html/*.html; do tidy 2>/dev/null $$i >x; mv x $$i; done
	perl -pi -e "s#/usr/share/sgml/docbook/stylesheet/dsssl/modular/images/#/docbook-icons/#g" `find html -type f`
	perl -pi -e "s/@@/&#64;&#64;/g" `find html -type f`


manual.pdf: manual.xml
	db2pdf manual.xml
	rm -rf *.junk *.out

manual.xml: db.xml forms.xml  tutorial.xml  users.xml extensions.xml  intro.xml  templating.xml  useful.xml
	touch manual.xml

check: manual.xml
	nsgmls -s $<

clean:
	rm -rf html
	rm -rf *.pdf
