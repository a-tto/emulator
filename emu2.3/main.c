#include <stdio.h>	//printf()
#include <string.h>	//memset(),
#include <stdlib.h>	//malloc(),free()

#define MEMORY_SIZE (1024*1024)

enum Register {EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI, REGISTERS_COUNT};

char *registers_name[] = {
	"EAX", "ECX", "EDX", "EBX", "ESP", "EBP", "ESI", "EDI"};

typedef struct {
	//汎用レジスタ
	uint32_t registers[REGISTERS_COUNT];
	//efragsレジスタ
	uint32_t efrags;
	//メモリのアドレスを指すポインタ
	//1バイトずつ読み込むのでuint8_t型
	uint8_t	 *memory;
	//プログラムカウンタ
	//なぜレジスタの一部として配列になっていないのか不明
	uint32_t eip;

} Emulator;

//エミュレータを作成する関数
//メモリのサイズ，eip,espレジスタのアドレスを引数にとる
static Emulator *create_emu(size_t size, uint32_t eip, uint32_t esp){

	//メモリ領域を割り当てる
	Emulator *emu = malloc(sizeof(Emulator));
	emu->memory = malloc(size);

	//レジスタの値を全て０にセットする
	memset(emu->registers, 0, sizeof(emu->registers));

	//指定されたレジスタの値をセットする
	emu->eip = eip;
	emu->registers[ESP] = esp;

	return emu;
}

//エミュレータを破棄する
static void destroy_emu(Emulator *emu){
	free(emu->memory);
	free(emu);
}

static void dump_registers(Emulator *emu){
	int i;

	for ( i = 0; i < REGISTERS_COUNT; i++) {
		printf("%s = %08x\n",registers_name[i],emu->registers[i]);
	}

	printf("EIP = %08x\n",emu->eip);
}

uint32_t get_code8(Emulator *emu, int index){
	return emu->memory[emu->eip + index];
}

int32_t get_sign_code8(Emulator *emu, int index){
	return (int8_t)emu->memory[emu->eip + index];
}

uint32_t get_code32(Emulator *emu, int index){
	int i;
	uint32_t ret = 0;

	for ( i = 0; i < 4; i++) {
		ret |= get_code8(emu,i + index) << (i * 8);
	}

	return ret;
}

void mov_r32_imm32(Emulator *emu){
	uint8_t reg = get_code8(emu,0) - 0xB8;
	uint32_t value = get_code32(emu,1);

	emu->registers[reg] = value;
	emu->eip += 5;
}

void short_jump(Emulator *emu){
	uint8_t diff = get_code8(emu,1);
	emu->eip = diff + 2;
}

typedef void instruction_func(Emulator *);
instruction_func* instructions[256];

void init_instructions(void){
	int i;
	memset(instructions, 0, sizeof(instructions));

	for ( i = 0; i < 8; i++) {
		instructions[i + 0xB8] = mov_r32_imm32;
	}
	instructions[0xEB] = short_jump;
}

int main(int argc, char *argv[]){
	FILE *binary;
	Emulator *emu;

	if (argc != 2) {
		printf("usage: px86 <filename>\n");
		return 1;
	}

	emu = create_emu(MEMORY_SIZE,0,0x7c00);

	binary = fopen(argv[1], "rb");

	if (binary == NULL) {
		printf("file can't opened");
		return 1;
	}

	fread(emu->memory, 1, 0x200, binary);
	fclose(binary);

	init_instructions();

	while (emu->eip < MEMORY_SIZE) {
		uint8_t code = get_code8(emu,0);
		printf("EIP = %08x, CODE = %02x\n",emu->eip,code);

		if (instructions[code] == NULL) {
			printf("\n\nNot Implemented: %x\n",code);
			break;
		}
		
		instructions[code](emu);

		if (emu->eip == 0x00) {
			printf("\n\nend of program.\n\n");
			break;
		}
	}
	dump_registers(emu);
	destroy_emu(emu);

	return 0;
}
