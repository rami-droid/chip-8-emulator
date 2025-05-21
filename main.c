#include <stdio.h>
#include <stdbool.h>

struct mainMemory {
    unsigned char memory[4096]; // 4KB of memory
    unsigned char V[16];       // 16 registers
    unsigned short I;          // Index register
    unsigned short PC;         // Program counter
    unsigned char delay_timer; // Delay timer
    unsigned char sound_timer; // Sound timer
    unsigned short stack[16];  // Stack
    unsigned short SP;         // Stack pointer
};

int romLoader(const char *filename, struct mainMemory *chip8) {
    FILE *romFile = fopen(filename, "rb");
    if (romFile == NULL) {
        perror("Failed to open ROM file");
        return -1;
    }
    fseek(romFile, 0, SEEK_END);
    long fileSize = ftell(romFile);
    fseek(romFile, 0, SEEK_SET);
    fread(&chip8->memory[0x200], 1, fileSize, romFile);
    printf("Loaded %ld bytes.\n", fileSize);
    return 0;
}

int interpreter (int currByte, struct mainMemory *chip8) {
    int opcode = chip8->memory[currByte] << 8 | chip8->memory[currByte + 1];
    short x;
    short y;
    unsigned short NNN;
    unsigned char NN;

    switch (opcode & 0xF000) {
    case 0x0000:
        switch (opcode)
        {
        case 0x00E0:
            //clear the screen
            break;
        
        case 0x00EE:
            chip8->SP--;
            
            //return from subroutine
            break;
        default:
            
            //Jump to subroutine at NNN
            break;
        }
        break;
    case 0x1000:
        NNN = opcode & 0x0FFF;
        chip8->PC = NNN;
        //jump to address NNN
        break;
    case 0x2000:
        NNN = opcode & 0x0FFF; 

            chip8->stack[chip8->SP] = chip8->PC;
            chip8->SP++;

            chip8->PC = NNN;
        //call subroutine at NNN
        break;

    case 0x3000:
        //skip next instruction if Vx == NN
        unsigned short NN = opcode & 0x00FF;
        if (NN == chip8->V[(opcode & 0x0F00) >> 8]) {
            chip8->PC += 2;
        }        
        break;
    
    case 0x4000:
        //skip next instruction if Vx != NN
        NN = opcode & 0x00FF;
        if (NN != chip8->V[(opcode & 0x0F00) >> 8]) {
            chip8->PC += 2;
        }   
        break;

    case 0x5000:
        y = (opcode & 0x00F0) >> 4;
        x = (opcode & 0x0F00) >> 8;
        //skip next instruction if Vx == Vy
        if(chip8->V[x] == chip8->V[y]) {
            chip8->PC += 2;
        }
        break;

    case 0x6000:
        //set Vx to NN
        x = (opcode & 0x0F00) >> 8;
        NN = opcode & 0x00FF;
        chip8->V[x] = NN;
        break;
    
    case 0x7000:
        x = (opcode & 0x0F00) >> 8;
        NN = opcode & 0x00FF;
        //add NN to Vx

        chip8->V[x] += NN;
        break;

    case 0x8000:
        switch (opcode & 0x000F) {
        case 0x0000:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            chip8->V[x] = chip8->V[y];
            break;
        case 0x0001:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;

            chip8->V[x] |= chip8->V[y];
            //set Vx to Vx OR Vy 
            //Bitwise OR
            break;
        
        case 0x0002:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;

            chip8->V[x] &= chip8->V[y];
            //set Vx to Vx AND Vy
            //Bitwise AND
            break;

        case 0x0003:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;

            chip8->V[x] ^= chip8->V[y];
            //set Vx to Vx XOR Vy
            //Bitwise XOR
            break;

        case 0x0004:
            //add Vx to Vy, set Vx to result
            //set VF to 1 if carry, 0 if not
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            int res = chip8->V[x] + chip8->V[y];
            if (res > 255) {
                chip8->V[0xF] = 1; //carry
            } else {
                chip8->V[0xF] = 0; //no carry
            }
            chip8->V[x] += chip8->V[y];

            break;

        case 0x0005:
            //subtract Vy from Vx, set Vx to result
            //set VF to 0 if borrow, 1 if not

            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            if (chip8->V[x] >= chip8->V[y]) {
                chip8->V[0xF] = 1; //no borrow
            } else {
                chip8->V[0xF] = 0; //borrow
            }
            chip8->V[x] -= chip8->V[y];
            break;
        
        case 0x0006:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;

            chip8->V[0xF] = chip8->V[x] & 0x1;            
            chip8->V[x] >>= 1;
            //shift Vx right by 1, set Vx to result
            //set VF to least significant bit before shift
            break;

        case 0x0007:
            //set Vx to Vy - Vx
            //set VF to 0 if borrow, 1 if not
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            if (chip8->V[y] > chip8->V[x]) {
                chip8->V[0xF] = 1; //no borrow
            } else {
                chip8->V[0xF] = 0; //borrow
            }
            chip8->V[x] = chip8->V[y] - chip8->V[x];

            break;
        case 0x000E:
            //shift Vx left by 1, set Vx to result
            //set VF to most significant bit before shift

            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;

            chip8->V[0xF] = (chip8->V[x] & 0x80) >> 7;
            chip8->V[x] <<= 1;
            break;
        default:
            break;
        }
        break;

    case 0x9000:
        //skip next instruction if Vx != Vy
        break;

    case 0xA000:
        //set I to NNN
        break;
    
    case 0xB000:
        //set PC to NNN + V0
        break;
    
    case 0xC000:
        //set Vx to random number(0,255) AND NN
        break;
    
    case 0xD000:
        //draw sprite at coordinate (Vx, Vy) with height of N
        //set VF to 1 if any set pixels are unset, 0 if not
        break;

    case 0xE000:
        switch (opcode & 0x00FF) {
        case 0x009E:
            //skip next instruction if key with value of Vx is pressed
            break;
        
        case 0x00A1:
            //skip next instruction if key with value of Vx is not pressed
            break;
        default:
            break;
        }
        break;

    case 0xF000:
        switch (opcode & 0x00FF) {
        case 0x0007:
            //set Vx to delay timer value
            break;
        case 0x000A:
            //wait for key press, store value in Vx
            break;
        case 0x0015:
            //set delay timer to Vx
            break;
        case 0x0018:
            //set sound timer to Vx
            break;
        case 0x001E:
            //add Vx to I
            break;

        case 0x0029:
            //set I to location of sprite for digit Vx
            break;
        case 0x0033:
            //store BCD representation of Vx in memory locations I, I+1, I+2
            break;
        case 0x0055:
            //store registers V0 to Vx in memory starting at location I
            //I = I + x + 1
            break;
        case 0x0065:
            //read registers V0 to Vx from memory starting at location I
            //I = I + x + 1
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
    
    return opcode;
}

int main() {
    bool running = true;
    struct mainMemory chip8;
    unsigned short pc = chip8.PC;

    printf("Hello, Chip-8 Emulator!\n");
    romLoader("Connect-4.ch8", &chip8);
    printf("Memory[0]: %x\n", chip8.memory[0x200]);
    printf("Opcode: %x\n", interpreter(0x200, &chip8));
    return 0;

    while (running)  // Main loop
    {
        
    }
    
}