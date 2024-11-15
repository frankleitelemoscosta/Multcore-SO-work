#include"./cpu/REGISTER_BANK.h"
#include"./cpu/CONTROL_UNIT.h"
#include"./memory/MAINMEMORY.h"
#include"./loader.h"
#include<pthread.h>

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

    
      
    pthread_t threads[4];
    int inputs[4] = {1, 2, 3, 4};

    // Criação das threads
    for (int i = 0; i < 4; i++) {
        if (pthread_create(&threads[i], nullptr, UC.Pipeline(ram), &inputs[i]) != 0) {
            std::cerr << "Erro ao criar thread " << i << std::endl;
            return 1;
        }
    }

    // Join para esperar todas as threads finalizarem
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], nullptr);
    }

    std::cout << "Todas as entradas foram processadas." << std::endl;
    return 0;
}
