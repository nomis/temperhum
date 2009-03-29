all: README.html
clean:
	rm -f README.html

README.html: README Makefile
	sed \
		-e 's@\(http://[^ ]*\)@<a href="\1">\1</a>@' \
		-e 's@$$@<br>@' \
		-e 's@^= \(.*\) =<br>$$@<h1>\1</h1>@' \
		-e 's@^== \(.*\) ==<br>$$@<h2>\1</h2>@' \
		-e 's@^[\t]<br>$$@@' \
		-e 's@^ \(.*\)$$@<code>\1</code>@' \
		< README > README.html
