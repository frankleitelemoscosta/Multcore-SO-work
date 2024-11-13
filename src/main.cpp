#include"./cpu/REGISTER_BANK.h"
#include"./cpu/CONTROL_UNIT.h"
#include"./memory/MAINMEMORY.h"
#include"./loader.h"

using namespace std;

int main(int argc, char* argv[]){

    if (argc < 2) {
      std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
      return 1;
    }

    MainMemory ram = MainMemory(2048,2048);				//  1º  Cria-se uma variável do tipo MainMemory com 2048 linhas e 2048 colunas.
    int initProgram[argc];
    initProgram[0]=0; //onde se inicia o primeiro programa
    Control_Unit UC;

    for(int i = 1; i < argc; i++){
        initProgram[i] = loadProgram(argv[i],ram);
    }

    UC.Pipeline(ram);
      
       
    return 0;
}
