.PHONY: clean run #Tell make that 'clean' and 'run' are not associated with building files

#Compile source files and produce executable
all : connect.o
	cc -o connect connect.c
#Remove object and temp files
clean :
	rm connect *.o
#Enter word to search dict for with : make run arg1=___ arg2=___
#Cannot contain flags/spaces on the arguments : make run arg1=ls -a arg2=sort -r  ->  NO
#                                               make run arg1=ls arg2=sort        ->  YES
#                                               make run arg1=ls                  ->  YES
#                                               (read in arg2 must contain no flags/spaces)
run :
	./connect $(arg1) : $(arg2)

