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
    unsigned char N;

    switch (opcode & 0xF000) {
    case 0x0000:
        switch (opcode)
        {
        case 0x00E0:
            //clear the screen
            chip8->PC += 2;
            break;
        
        case 0x00EE:
            //return from subroutine
            chip8->SP--;
            chip8->PC = chip8->stack[chip8->SP];
            chip8->PC += 2;
            break;
        default:
            //No operation
            //call RCA 1802 program at address NNN
            chip8->PC += 2;
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
        NN = opcode & 0x00FF;
        if (NN == chip8->V[(opcode & 0x0F00) >> 8]) {
            chip8->PC += 2;
        }
        chip8->PC += 2;
        break;
    
    case 0x4000:
        //skip next instruction if Vx != NN
        NN = opcode & 0x00FF;
        if (NN != chip8->V[(opcode & 0x0F00) >> 8]) {
            chip8->PC += 2;
        }
        chip8->PC += 2;
        break;

    case 0x5000:
        y = (opcode & 0x00F0) >> 4;
        x = (opcode & 0x0F00) >> 8;
        //skip next instruction if Vx == Vy
        if(chip8->V[x] == chip8->V[y]) {
            chip8->PC += 2;
        }
        chip8->PC += 2;
        break;

    case 0x6000:
        //set Vx to NN
        x = (opcode & 0x0F00) >> 8;
        NN = opcode & 0x00FF;
        chip8->V[x] = NN;
        chip8->PC += 2;
        break;
    
    case 0x7000:
        x = (opcode & 0x0F00) >> 8;
        NN = opcode & 0x00FF;
        //add NN to Vx
        chip8->V[x] += NN;
        chip8->PC += 2;
        break;

    case 0x8000:
        switch (opcode & 0x000F) {
        case 0x0000:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            chip8->V[x] = chip8->V[y];
            chip8->PC += 2;
            break;
        case 0x0001:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            chip8->V[x] |= chip8->V[y];
            chip8->PC += 2;
            break;
        
        case 0x0002:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            chip8->V[x] &= chip8->V[y];
            chip8->PC += 2;
            break;

        case 0x0003:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            chip8->V[x] ^= chip8->V[y];
            chip8->PC += 2;
            break;

        case 0x0004:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            int res = chip8->V[x] + chip8->V[y];
            chip8->V[0xF] = (res > 255) ? 1 : 0;
            chip8->V[x] += chip8->V[y];
            chip8->PC += 2;
            break;

        case 0x0005:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            chip8->V[0xF] = (chip8->V[x] >= chip8->V[y]) ? 1 : 0;
            chip8->V[x] -= chip8->V[y];
            chip8->PC += 2;
            break;
        
        case 0x0006:
            x = (opcode & 0x0F00) >> 8;
            chip8->V[0xF] = chip8->V[x] & 0x1;            
            chip8->V[x] >>= 1;
            chip8->PC += 2;
            break;

        case 0x0007:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            chip8->V[0xF] = (chip8->V[y] > chip8->V[x]) ? 1 : 0;
            chip8->V[x] = chip8->V[y] - chip8->V[x];
            chip8->PC += 2;
            break;

        case 0x000E:
            x = (opcode & 0x0F00) >> 8;
            chip8->V[0xF] = (chip8->V[x] & 0x80) >> 7;
            chip8->V[x] <<= 1;
            chip8->PC += 2;
            break;

        default:
            break;
        }
        break;

    case 0x9000:
        x = (opcode & 0x0F00) >> 8;
        y = (opcode & 0x00F0) >> 4;
        if (chip8->V[x] != chip8->V[y]) {
            chip8->PC += 2;
        }
        chip8->PC += 2;
        break;

    case 0xA000:
        NNN = opcode & 0x0FFF;
        chip8->I = NNN;
        chip8->PC += 2;
        break;
    
    case 0xB000:
        NNN = opcode & 0x0FFF;
        chip8->PC = NNN + chip8->V[0];
        break;
    
    case 0xC000:
        x = (opcode & 0x0F00) >> 8;
        NN = opcode & 0x00FF;
        chip8->V[x] = (rand() % 256) & NN;
        chip8->PC += 2;
        break;
    
    case 0xD000:
        x = (opcode & 0x0F00) >> 8;
        y = (opcode & 0x00F0) >> 4;
        N = opcode & 0x000F;
        //draw sprite at coordinate (Vx, Vy) with height of N
        //set VF to 1 if any set pixels are unset, 0 if not
        chip8->PC += 2;
        break;

    case 0xE000:
        switch (opcode & 0x00FF) {
        case 0x009E:
            //skip next instruction if key with value of Vx is pressed
            chip8->PC += 2;
            break;
        
        case 0x00A1:
            //skip next instruction if key with value of Vx is not pressed
            chip8->PC += 2;
            break;
        default:
            break;
        }
        chip8->PC += 2;
        break;

    case 0xF000:
        switch (opcode & 0x00FF) {
        case 0x0007:
            //set Vx to delay timer value
            chip8->PC += 2;
            break;
        case 0x000A:
            //wait for key press, store value in Vx
            chip8->PC += 2;
            break;
        case 0x0015:
            //set delay timer to Vx
            chip8->PC += 2;
            break;
        case 0x0018:
            //set sound timer to Vx
            chip8->PC += 2;
            break;
        case 0x001E:
            x = (opcode & 0x0F00) >> 8;
            chip8->I += chip8->V[x];
            chip8->PC += 2;
            break;

        case 0x0029:
            //set I to location of sprite for digit Vx
            chip8->PC += 2;
            break;
        case 0x0033:
            x = (opcode & 0x0F00) >> 8;
            chip8->memory[chip8->I] = chip8->V[x] / 100; //hundreds
            chip8->memory[chip8->I + 1] = (chip8->V[x] / 10) % 10; //tens
            chip8->memory[chip8->I + 2] = chip8->V[x] % 10; //ones
            chip8->PC += 2;
            break;
        case 0x0055:
            x = (opcode & 0x0F00) >> 8;
            for (int i = 0; i <= x; i++) {
                chip8->memory[chip8->I + i] = chip8->V[i];
            }
            chip8->I += x + 1;
            chip8->PC += 2;
            break;
        case 0x0065:
            x = (opcode & 0x0F00) >> 8;
            for (int i = 0; i <= x; i++) {
                chip8->V[i] = chip8->memory[chip8->I + i];
            }
            chip8->PC += 2;
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
        interpreter(pc, &chip8);
    }
    
}