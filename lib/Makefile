# ------------  dummy default --------------------------------------------------
.DEFAULT_GOAL: default
.PHONY: default

default:
	echo "no default target"

# ------------  uncrustify -----------------------------------------------------
.PHONY: uncrustify

uncrustify:
	@for f in $$(find . -type f -regex '.*\.\([ch]\(pp\)?\|inl\)') ; do \
		uncrustify -c .uncrustify.cfg --replace --no-backup $$f ; \
	done


