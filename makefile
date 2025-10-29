#variaveis
CXX = gcc										#compilador
CXXFLAGS = -g 									#flags para o compilador

SRC1 = servidor.c 								#arquivos para compilar
OBJ1 = $(SRC1:.c=.o)							#arquivos objetos que vão ser gerados
EXEC1 = server									#nome do arquivo executável

SRC2 = cliente.c 								#arquivos para compilar
OBJ2 = $(SRC2:.c=.o)							#arquivos objetos que vão ser gerados
EXEC2 = mandioca

#regras
all: $(OBJ1) $(OBJ2)					#gera o executável a partir dos programas objetos
	$(CXX) $(OBJ1) -o $(EXEC1)
	$(CXX) $(OBJ2) -o $(EXEC2)

%.o: %.c								#cria os programas objetos
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f $(OBJ1) $(EXEC1) $(OBJ2) $(EXEC2)

.PHONY: all clean						#específica que são regras e não nome de arquivos