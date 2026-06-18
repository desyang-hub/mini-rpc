.PHONY: all dev release test clean

all: dev

dev:
	bash scripts/build.sh dev

release:
	bash scripts/build.sh release

test:
	bash scripts/build.sh test && bash scripts/test.sh

clean:
	bash scripts/clean.sh
