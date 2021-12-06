SRC := $(shell /usr/bin/find . -mindepth 2 -name "*.Rmd")

README.md index.Rmd: $(SRC)
	LC_CTYPE=German_Germany.1252 RScript --vanilla tools/build_docs.R