/* bytecode disassembler! */


const char *reg_strs[REG_COUNT] = {
	"r0",
	"r1",
	"r2",
	"r3",
	"r4",
	"r5",
	"r6",
	"r7",
	"r8",
	"r9",
	"r10",
	"r11",
	"r12",
	"r13",
	"r14",
	"r15",

	"rsp",
	"sbp",
};

const char *opc_strs[OPC_COUNT] = {
	"call",
	"enter",
	"leave",
	"ret",
	"c-reset",
	"call-c-void",
	"call-c-int",
	"c-int-arg",
	"add-int",
	"load",
	"push",
	"pop",
	"mov"
};

void disass_proc(struct instruction *istart, struct instruction *iend)
{
	printf("--begin bytecode asm--\n");
	for (struct instruction *i = istart; i < iend; ++i) {
		const char *opc_str = opc_strs[i->opcode];
		printf("%s", opc_str);
		size_t oslen = strlen(opc_str);
		for (int j = 0; j < 16 - oslen; j++) {
				printf(" ");
		}

		int oprc = oprcounts[i->opcode];
		/* printf("oprc: %d\n", oprc); */

		/* if (i->opcode == OPC_MOV) { */
		/* 	printf("AHHAHONEAOTEHNAOTH\n"); */
		/* } */
		for (int j = 0; j < oprc; ++j) {
			struct operand opr = i->operands[j];
			switch (opr.mode) {
			case MODE_REG:
				printf("%s ", reg_strs[opr.reg]);
				break;
			case MODE_MEM:
				if (opr.offset != 0)
					printf("%d", opr.offset);
				printf("(%s) ", reg_strs[opr.reg]);
				break;
			case MODE_DIRECT:
				printf("%ld ", (i64)opr.direct_value);
				break;
			}
		}
		printf("\n");
	}
	printf("--end bytecode asm--\n");
}
