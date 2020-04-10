# ------------  dummy default --------------------------------------------------
.DEFAULT_GOAL: default
.PHONY: default

default:
	echo "no default target"

# ------------  uncrustify -----------------------------------------------------
.PHONY: uncrustify

uncrustify:
	@for f in $$(find . -type f -regex '.*\.\([ch]\(pp\)?\|inl\)') ; do \
		uncrustify -l CPP -c .uncrustify.cfg --replace --no-backup $$f ; \
	done
	@for f in $$(find . -type f -regex '.*\.c') ; do \
		uncrustify -l C -c .uncrustify.cfg --replace --no-backup $$f ; \
	done


