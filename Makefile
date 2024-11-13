

SRC = main.cpp serialPort.c plot.cpp 

LIBS = -lglfw3 -lglew32 -lopengl32 -lglu32
all:
	g++ -o main $(SRC) $(LIBS)

test:
	g++ test.cpp -o sinewave $(LIBS)


# -lgdi32