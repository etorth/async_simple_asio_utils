all:
	g++-14 -std=c++23 f.cpp g.cpp main.cpp -g -O0 -fsanitize=address
commit: clean
	git add .
	git commit -m f
	git push
clean:
	rm -f *.o a.out
