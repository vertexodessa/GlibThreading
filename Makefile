all:
	g++ main.cc -I. -std=c++11 `pkg-config --cflags --libs glib-2.0` -O0  -g -o test
