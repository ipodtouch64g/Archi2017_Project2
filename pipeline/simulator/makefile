CC = g++ -std=c++11
SRCS = ./*.cpp
OBS = ./*.o

pipeline: SRC
	$(CC) -o $@ $(OBS)

SRC: $(SRCS)
	$(CC) -c $(SRCS)

clean: $(OBS)
	rm -f $(OBS)
