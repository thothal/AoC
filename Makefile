SRC := $(shell /usr/bin/find . -mindepth 2 -name "*.Rmd")

README.md index.Rmd: $(SRC)
	RScript --vanilla tools/build_docs.R