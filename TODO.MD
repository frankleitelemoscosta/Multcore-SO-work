- [x] Pthread pass var to each thread
- [x] Timer de execução
- [x] Quantum de cada processo e interrupção
- [x] PCB - pronto, bloqueado e executando
- [x] Base do escalonador, preempção e troca de processos
- [ ] Refatoração das informaçoes da unidade de controle
- [ ] Fazer o I/O ser oculpado por apenas um processo por vez (mutex compartilhado entre eles) e fazer a troca
de processos enquanto o I/O requisitado está bloqueado
	Delay artificial + Bloquear e sair do processo + Gerenciador de recurso olhar o mutex e desbloquear o proximo
- [ ] Criar instrução read


"O ciclo de vida do sistema operacional será implementado utilizando múltiplas
threads, permitindo que tarefas como verificação de solicitações de recursos
e gerenciamento de processos prontos/executando sejam realizadas de forma
independente e em paralelo."
