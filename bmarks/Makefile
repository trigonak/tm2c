all:
	$(MAKE) -C .. benchmarks

clean:
	$(MAKE) -C .. clean_bmarks

# build any target by forwarding to the main makefile
% : forward_ 
	@echo the call is for $@
	$(MAKE) -C .. bmarks/$@

.PHONY : forward_ clean
