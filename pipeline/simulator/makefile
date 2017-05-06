CC = g++ -std=c++11 -O3
SRCS = ./*.cpp
OBS = ./*.o

pipeline: SRC
	$(CC) -o $@ $(OBS)

SRC: $(SRCS)
	$(CC) -c $(SRCS)

clean: $(OBS)
	rm -f $(OBS)
