run: my_wget
	./my_wget -r -e -o log.out http://www.cs.stir.ac.uk/~kjt/index.html
build: my_wget
	g++ my_wget.cpp -o my_wget
clean: 
	rm -rf my_wget *.*.* Makefile~ *.out
