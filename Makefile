SUBDIRS := client server
SUBDIRSCLEAN=$(addsuffix _clean,$(SUBDIRS)) 

.PHONY : $(MAKECMDGOALS)
$(MAKECMDGOALS): 
	$(MAKE) -C $@


.PHONY : clean all

all:
	$(foreach d, $(SUBDIRS), $(MAKE) -C $(d);)

clean: $(SUBDIRSCLEAN)
	@echo Cleaned server and client

%_clean: %
	$(MAKE) -C $< clean
