all:
	g++-14 -std=c++23 *.cpp -g -O0 -fsanitize=address -fcoroutines
commit: clean
	git add .
	git commit -m f
	git push
clean:
	rm -f *.o a.out
