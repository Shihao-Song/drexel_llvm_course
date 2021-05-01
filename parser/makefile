ROOT	:= /home/research/Documents/teaching/drexel_llvm_course
SOURCE	:= $(ROOT)/parser/main.cc $(ROOT)/lexer/lexer.cc
CC	:= g++
FLAGS	:= -O3 -std=c++17 -w 
FLAGS	+= -I $(ROOT)
TARGET	:= parser

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(FLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET)
