#include "CONTROL_UNIT.h"
#include "../PCB.h"
#include <bitset>
#include <memory>
#include <string>
#include <vector>


void Core(MainMemory &ram, PCB &process, vector<unique_ptr<ioRequest>>* ioRequests, bool &printLock, Cache &cache){
    // load register and state from PCB
    auto &registers = process.regBank;
    
    Control_Unit UC;
    Instruction_Data data;
    
    int clk = 0;
    int counterForEnd = 5;
    int counter = 0;
    bool endProgram = false;
    bool endExecution = false;
    
    ControlContext context{registers, ram, *ioRequests, printLock, process, counter, counterForEnd, endProgram, endExecution,cache,clk};
    
    while(context.counterForEnd > 0){
        if(context.counter >= 4 && context.counterForEnd >= 1){
            //chamar a instrução de write back
            UC.Write_Back(UC.data[context.counter - 4],context);
        }
        if(context.counter >= 3 && context.counterForEnd >= 2){
            //chamar a instrução de memory_acess da unidade de controle
            UC.Memory_Acess(UC.data[context.counter - 3],context);
        }
        if(context.counter >= 2 && context.counterForEnd >= 3){
            //chamar a instrução de execução da unidade de controle
            UC.Execute(UC.data[context.counter - 2], context);
        }
        if(context.counter >= 1 && context.counterForEnd >= 4){
            //chamar a instrução de decode da unidade de controle
            UC.Decode(context.registers,UC.data[context.counter-1]);
        }
        if(context.counter >= 0 && context.counterForEnd == 5){
            //chamar a instrução de fetch da unidade de controle
            UC.data.push_back(data);
            UC.Fetch(context);
        }
        context.counter += 1;
        clk += 1;
        process.timestamp += 1;
        
        if(clk >= process.quantum || context.endProgram == true){
            context.endExecution = true;
        }
        
        if(context.endExecution == true){
            context.counterForEnd -= 1;
        }
    }

    if(context.endProgram){
        context.process.state = State::Finished;
    }    
    
    return;
}


using namespace std;

uint32_t ConvertToDecimalValue(uint32_t value){
    string bin_str = to_string(value);
        uint32_t decimal_value = 0;       
        int len = bin_str.length();
        for (int i = 0; i < len; ++i) {
        if (bin_str[i] == '1') {
            decimal_value += std::pow(2, len - 1 - i);
            }
        }
    return decimal_value;
}

//PIPELINE

void Control_Unit::Fetch(ControlContext &context){
    const uint32_t instruction = context.registers.ir.read();
    
    //registers.ir.write(ram.ReadMem(registers.mar.read()));
    if(instruction == 0b11111100000000000000000000000000)
    {
        //cout << "END FOUND" << endl;
        context.endProgram = true;
        return;
    }
    context.registers.mar.write(context.registers.pc.value);
    //chamar a memória com a posição do pc e inserir em um registrador
    //registers.ir.write(aqui tem de ser passado a instrução que estiver na RAM);

    context.registers.ir.write(context.ram.ReadMem(context.registers.mar.read()));
    //cout << "IR: " << bitset<32>(registers.ir.read()) << endl;
    context.registers.pc.write(context.registers.pc.value += 1);//incrementando o pc 
}

void Control_Unit::Decode(REGISTER_BANK &registers, Instruction_Data &data){

    const uint32_t instruction = registers.ir.read();
    data.rawInstrucion = instruction;
    data.op = Identificacao_instrucao(instruction,registers);
    //cout << data.op << endl;

    //cout <<instruction << endl;

    if(data.op == "ADD" || data.op == "SUB" || data.op == "MULT" || data.op == "DIV"){
        // se entrar aqui é porque tem de carregar registradores, que estão especificados na instrução
        data.source_register = Get_source_Register(instruction);
        data.target_register = Get_target_Register(instruction);
        data.destination_register = Get_destination_Register(instruction);

    }else if(data.op == "LI" || data.op == "LW" || data.op == "LA" || data.op == "SW")
    {
        data.target_register = Get_target_Register(instruction);
        data.addressRAMResult = Get_immediate(instruction);
        //cout << "source register: " <<data.source_register << endl;
        //cout << "segundo registrador:" << data.addressRAMResult << endl;

    }else if(data.op == "BLT" || data.op == "BGT" || data.op == "BGTI" || data.op == "BLTI"){
        data.source_register = Get_source_Register(instruction);
        data.target_register = Get_target_Register(instruction);
        data.addressRAMResult = Get_immediate(instruction);   
    }
    else if(data.op == "J"){
        data.addressRAMResult = Get_immediate(instruction);
    }
    else if(data.op == "PRINT"){
        string instrucao = to_string(instruction);
        if(Get_immediate(instruction) == "0000000000000000"){  //se for zero, então é um registrador
            data.target_register = Get_target_Register(instruction);
        }else{  //se não for zero, então é um valor imediato
            data.addressRAMResult = Get_immediate(instruction); 
        }
    }

    return;
}

void Control_Unit::Execute(Instruction_Data &data, ControlContext &context){
    /*Daqui tem de ser chamado o que tiver de ser chamado*/

    if(data.op == "ADD" ||  data.op == "SUB" || data.op == "MUL" || data.op == "DIV"){
        Execute_Aritmetic_Operation(context.registers, data);
    }else if(data.op == "BEQ" || data.op == "J" || data.op == "BNE" || data.op == "BGT" || data.op == "BGTI" || data.op == "BLT" || data.op == "BLTI"){
        Execute_Loop_Operation(context.registers, data, context.counter,context.counterForEnd,context.endProgram,context.ram);
    }
    else if( data.op == "PRINT" ){
        Execute_Operation(data,context);
    }

    //cout<<"entrou"<<endl;
    //cout << counter << endl;

    // demais operações realizadas no memory acess
}

void Control_Unit::Memory_Acess(Instruction_Data &data, ControlContext &context){

    
    string nameregister = this->map.mp[data.target_register];

    if(data.op == "LW"){
        int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));
        
        cout << context.cache.find(decimal_value) << endl;
        if(context.cache.find(decimal_value)){
            context.registers.acessoEscritaRegistradores[nameregister](context.cache.access(decimal_value,context.clock));
        }
        else{
            context.registers.acessoEscritaRegistradores[nameregister](context.ram.ReadMem(decimal_value));
            context.cache.write(decimal_value,context.ram.ReadMem(decimal_value),context.clock);
        }
    }
    if(data.op == "LA" || data.op == "LI"){
        int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));
        
        context.registers.acessoEscritaRegistradores[nameregister](decimal_value);
    }
    else if(data.op == "PRINT" && data.target_register == ""){
        int decimalAddr = ConvertToDecimalValue(stoul(data.addressRAMResult));
        int value = 0;

        cout << context.cache.find(decimalAddr) << endl;
        if(context.cache.find(decimalAddr)){
            value = context.cache.access(decimalAddr,context.clock);
        }
        else{
            value = context.ram.ReadMem(decimalAddr);            
            context.cache.write(decimalAddr,value,context.clock);
        }
        
        
        auto req = make_unique<ioRequest>();
        req->msg = to_string(value);
        req->process = &context.process;
        context.ioRequests.push_back(move(req));
        
        if(context.printLock){
            context.process.state = State::Blocked;
            context.endExecution = true;
        }
    }

    return;
}

void Control_Unit::Write_Back(Instruction_Data &data, ControlContext &context){

    string nameregister = this->map.mp[data.target_register];

    if(data.op == "SW"){
        int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));

        cout << context.cache.find(decimal_value) << endl;
        if (context.cache.find(decimal_value)){
            context.cache.write(decimal_value,context.registers.acessoLeituraRegistradores[nameregister](),context.clock);
        }
        else{
            context.ram.WriteMem(decimal_value, context.registers.acessoLeituraRegistradores[nameregister]());
            context.cache.write(decimal_value,context.registers.acessoLeituraRegistradores[nameregister](),context.clock);
        }        
    }

    return;

}

string Control_Unit::Identificacao_instrucao(uint32_t instruction, REGISTER_BANK &registers){

    string instrucao = bitset<32>(instruction).to_string();
    string string_instruction = instrucao;
    string opcode = "";
    string instruction_type = "";

    for(int i = 0; i < 6; i++){
        opcode += string_instruction[i];
    }
    //cout << string_instruction << endl;
    //instrução do tipo I
    if (opcode == this->instructionMap.at("la")) {              // LOAD from vector
        instruction_type = "LA";
    } else if (opcode == this->instructionMap.at("lw")) {       // LOAD
        instruction_type = "LW";
    } else if (opcode == this->instructionMap.at("j")) {       // JUMP
        instruction_type = "J";
    } else if (opcode == this->instructionMap.at("sw")) {       // STORE
        instruction_type = "SW";
    } else if (opcode == this->instructionMap.at("beq")) {      // EQUAL
        instruction_type = "BEQ";
    } else if (opcode == this->instructionMap.at("blt")) {      // LESS THAN
        instruction_type = "BLT";
    } else if (opcode == this->instructionMap.at("blti")) {     // LESS THAN OR EQUAL
        instruction_type = "BLTI";
    } else if (opcode == this->instructionMap.at("bgt")) {      // GREATER THAN
        instruction_type = "BGT";
    } else if (opcode == this->instructionMap.at("bgti")) {     // GREATER THAN OR EQUAL
        instruction_type = "BGTI";
    }
    else if (opcode == this->instructionMap.at("print")) {    
        instruction_type = "PRINT";
    }
    else if (opcode == this->instructionMap.at("li")) {    
        instruction_type = "LI"; // LOAD IMMEDIATE
    }

    // instruções do tipo R

    if (opcode == this->instructionMap.at("add")) {              
        instruction_type = "ADD";
            //cout << this->instructionMap.at("add")<< "opcode soma:" << opcode << endl;

    } else if (opcode == this->instructionMap.at("sub")) {       
        instruction_type = "SUB";
    } else if (opcode == this->instructionMap.at("mult")) {       
        instruction_type = "MULT";
    } else if (opcode == this->instructionMap.at("div")) {      
        instruction_type = "DIV";
    }

    return instruction_type;
} 

string Control_Unit::Get_immediate(const uint32_t instruction)
{
    string instrucao = bitset<32>(instruction).to_string();
    string copia_instrucao = instrucao;
    string code;
    for(int i = 16; i < 32; i++){
        code += copia_instrucao[i];
    }

    return code;
}

string Control_Unit::Get_destination_Register(const uint32_t instruction){
    string instrucao = bitset<32>(instruction).to_string();
    string copia_instrucao = instrucao;
    string code;
    for(int i = 16; i < 21; i++){
        code += copia_instrucao[i];
    }

    return code;
}

string Control_Unit::Get_target_Register(const uint32_t instruction){
    string instrucao = bitset<32>(instruction).to_string();
    string copia_instrucao = instrucao;
    string code;
    for(int i = 11; i < 16; i++){
        code += copia_instrucao[i];
    }

    return code;
}

string Control_Unit::Get_source_Register(const uint32_t instruction){
    string instrucao = bitset<32>(instruction).to_string();
    string copia_instrucao = instrucao;
    string code;
    for(int i = 6; i < 11; i++){
        code += copia_instrucao[i];
    }

    return code;
}

void Control_Unit::Execute_Aritmetic_Operation(REGISTER_BANK &registers,Instruction_Data &data){

        string nameregistersource = this->map.mp[data.source_register];
        string nametargetregister = this->map.mp[data.target_register];
        string nameregisterdestination = this->map.mp[data.destination_register]; 
        ALU alu;
        if(data.op == "ADD"){
            alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
            alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
            alu.op = ADD;
            alu.calculate();
            //cout << "valor de A:" << alu.A << endl;
            //cout << "valor de B:" << alu.B << endl;
            registers.acessoEscritaRegistradores[nameregisterdestination](alu.result);
            //cout<< "resultado soma:" <<registers.acessoLeituraRegistradores[nameregisterdestination]() << endl;
        }else if(data.op == "SUB"){
            alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
            alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
            alu.op = SUB;
            //cout << "valor de A:" << alu.A << endl;
            //cout << "valor de B:" << alu.B << endl;
            alu.calculate();
            registers.acessoEscritaRegistradores[nameregisterdestination](alu.result);
            //cout << "resultado subtração:" << registers.acessoLeituraRegistradores[nameregisterdestination]() << endl;
        }else if(data.op == "MUL"){
            alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
            alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
            alu.op = MUL;
            alu.calculate();
            registers.acessoEscritaRegistradores[nameregisterdestination](alu.result);
            //cout << "resultado multiplicação:" << registers.acessoLeituraRegistradores[nameregisterdestination]() << endl;
        }else if(data.op == "DIV"){
            alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
            alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
            alu.op = DIV;
            alu.calculate();
            registers.acessoEscritaRegistradores[nameregisterdestination](alu.result);
        }

        return;
}

void Control_Unit::Execute_Loop_Operation(REGISTER_BANK &registers,Instruction_Data &data, int &counter, int& counterForEnd, bool& programEnd, MainMemory& ram){
    
    string nameregistersource = this->map.mp[data.source_register];
    string nametargetregister = this->map.mp[data.target_register];
    string nameregisterdestination = this->map.mp[data.destination_register];
    ALU alu;
    if(data.op == "BEQ"){
        alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
        alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
        alu.op = BEQ;
        alu.calculate();
        if(alu.result == 1){
            int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));
            registers.pc.write(decimal_value);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0;
            counterForEnd = 5;
            programEnd = false;
        }
    }else if(data.op == "BNE"){
        alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
        alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
        alu.op = BNE;
        alu.calculate();
        if(alu.result == 1){
            int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));
            registers.pc.write(decimal_value);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0;
            counterForEnd = 5;
            programEnd = false;
        }
    }else if(data.op == "J"){
        int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));
        registers.pc.write(decimal_value);
        registers.ir.write(ram.ReadMem(registers.pc.read()));

        //cout << "Jump to: " << registers.pc.read();
        counter = 0;
        counterForEnd = 5;
        programEnd = false;
    }else if(data.op == "BLT"){
        alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
        alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
        alu.op = BLT;
        alu.calculate();
        if(alu.result == 1){
            int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));
            registers.pc.write(decimal_value);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0;
            counterForEnd = 5;
            programEnd = false;
        }
    }else if(data.op == "BLTI"){
        alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
        alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
        alu.op = BLTI;
        alu.calculate();
        if(alu.result == 1){
            int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));
            registers.pc.write(decimal_value);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0;
            counterForEnd = 5;
            programEnd = false;
        }
    }else if(data.op == "BGT"){
        alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
        alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
        alu.op = BGT;
        alu.calculate();
        if(alu.result == 1){
            int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));
            registers.pc.write(decimal_value);
            registers.ir.write(ram.ReadMem(registers.pc.read()));
            counter = 0;
            counterForEnd = 5;
            programEnd = false;
        }
    }else if(data.op == "BGTI"){
        alu.A = registers.acessoLeituraRegistradores[nameregistersource]();
        alu.B = registers.acessoLeituraRegistradores[nametargetregister]();
        alu.op = BGTI;
        alu.calculate();
        if(alu.result == 1){
            int decimal_value = ConvertToDecimalValue(stoul(data.addressRAMResult));
            registers.pc.write(decimal_value);
            registers.ir.write(ram.ReadMem(registers.pc.read()));

            counter = 0;
            counterForEnd = 5;
            programEnd = false;
        }
    }
    return;
}

void Control_Unit::Execute_Operation(Instruction_Data &data, ControlContext &context){
    string nameregister = this->map.mp[data.target_register];

    if(data.op == "PRINT" && data.target_register != ""){
        auto value = context.registers.acessoLeituraRegistradores[nameregister]();
        auto req = make_unique<ioRequest>();
        req->msg = to_string(value);
        req->process = &context.process;
        context.ioRequests.push_back(move(req));
        
        if(context.printLock){
            context.process.state = State::Blocked;
            context.endExecution = true;
        }
    }
}

string Control_Unit::Pick_Code_Register_Load(const uint32_t instruction){
    string instrucao = bitset<32>(instruction).to_string();
    string copia_instrucao = instrucao;
    string code;
    for(int i = 11; i < 16; i++){
        code += copia_instrucao[i];
    }

    return code;
}


