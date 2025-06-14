SRC = cnn.cpp datagen/bmp.cpp datagen/cam.cpp datagen/surface.cpp main.cpp
	  
OBJ=$(SRC:.cpp=.o)

OPTS=-g -ggdb
#OPTS+=-O3

cnn: $(OBJ)
	g++ -I datagen/ $(OPTS) $^ -o cnn

%.o: %.cpp
	g++ $(OPTS) -I datagen/ -c $< -o $@

clean:
	rm cnn *.o -rf