SUBDIRS := client server
SUBDIRSCLEAN=$(addsuffix _clean,$(SUBDIRS)) 

.PHONY : $(MAKECMDGOALS)
$(MAKECMDGOALS): 
	+$(MAKE) -C $@

.PHONY : clean
clean: $(SUBDIRSCLEAN)
	@echo Cleaned server and client

%_clean: %
	+$(MAKE) -C $< clean
