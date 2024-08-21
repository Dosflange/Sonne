
#include <u.h>
#include <libc.h>

struct myth_vm
{
        uchar pagebyte[256][256];

        uchar e;    /*Device ENABLE register */

        uchar sclk; /*Serial clock state bit*/
        uchar miso; /*Serial input line state bit*/
        uchar mosi; /*Serial output line state bit*/
        uchar sir;  /*Serial input register, modelled after HC595*/
        uchar sor;  /*Serial output register, modelled after HC165*/

        uchar pir;  /*Parallel input register*/
        uchar por;  /*Parallel output register*/

        uchar j;    /*JUMP register (program counter)*/
        uchar o;    /*ORIGIN*/
        uchar r;    /*RESULT*/

        uchar c;    /*CODE page register*/
        uchar d;    /*DATA page register*/
        uchar l;    /*LOCAL page register*/

        uchar g;    /*GLOBAL*/
        uchar b;    /*BROADCAST*/
        uchar i;    /*INNER*/
};

void myth_reset(struct myth_vm *vm);
uchar myth_fetch(struct myth_vm *vm);
void myth_step(struct myth_vm *vm);

void myth_exec_pair(struct myth_vm *vm, uchar opcode);
void myth_exec_gput(struct myth_vm *vm, uchar opcode);
void myth_exec_trap(struct myth_vm *vm, uchar opcode);
void myth_exec_alu(struct myth_vm *vm, uchar opcode);
void myth_exec_adj(struct myth_vm *vm, uchar opcode);
void myth_exec_sys(struct myth_vm *vm, uchar opcode);

void myth_call(struct myth_vm *vm, uchar dstpage);
void myth_ret(struct myth_vm *vm);


/*  The 'REGx' notation means: REG into (something)
    i.e. REG is a source
*/

#define Nx 0 /*from code literal (NUMBER)*/
#define DMx 1 /*from MEMORY via DATA page index*/
#define LMx 2 /*from MEMORY via LOCAL page index*/
#define Gx 3 /*from DATA page register*/
#define Rx 4 /*from RESULT register*/
#define Ix 5 /*from INNER register*/
#define Sx 6 /*from SERIAL input*/
#define Px 7 /*from PARALLEL input*/

/*  The 'xREG' notation means: (something) into REG
    i.e. REG is a destination
*/

#define xO 0 /*to ORIGIN register*/
#define xDM 1 /*to MEMORY via DATA page index*/
#define xLM 2 /*to MEMORY via LOCAL page index*/
#define xG 3 /*to GLOBAL page register*/
#define xR 4 /*to RESULT register*/
#define xI 5 /*to INNER register*/
#define xS 6 /*to SERIAL output*/
#define xP 7 /*to PARALLEL output*/

#define xE 8 /*to ENABLE register*/
#define xB 9 /*to BROADCAST register*/
#define xD 10 /*to DATA register*/
#define xJ 11 /*to JUMP page index*/
#define xW 12 /*to WHILE page index*/
#define xT 13 /*to TRUE page index*/
#define xF 14 /*to FALSE page index*/
#define xC 15 /*to CALL page index*/

#define IDR 0 /*Identity R*/
#define IDO 1 /*Identity O*/
#define OCR 2 /*Ones' complement of R*/
#define OCO 3 /*Ones' complement of O*/
#define SLR 4 /*Shift left R*/
#define SLO 5 /*Shift left O*/
#define SRR 6 /*Shift right R*/
#define SRO 7 /*Shift right O*/
#define AND 8 /*R AND O*/
#define IOR 9 /*R OR O*/
#define EOR 10 /*R XOR O*/
#define ADD 11 /*R + O*/
#define CAR 12 /*Carry of R + O (0 or 1)*/
#define RLO 13 /*255 if R<O else 0*/
#define REO 14 /*255 if R=O else 0*/
#define RGO 15 /*255 if R>O else 0*/

#define NOP 0 /*No Operation*/
#define SSI 1 /*Serial Shift In*/
#define SSO 2 /*Serial Shift Out*/
#define SCL 3 /*Set serial Clock Low*/
#define SCH 4 /*Set serial Clock High*/
#define RET 5 /*Return from CALL*/
#define FAR 6 /*Wide BRANCH*/
#define ORG 7 /*Wide PULL*/


void
myth_reset(struct myth_vm *vm)
{
        memset(vm->pagebyte, 0, 256*256);

        vm->e = 0;

        vm->sclk = 0;
        vm->miso = 0;
        vm->mosi = 0;
        vm->sir = 0;
        vm->sor = 0;

        vm->pir = 0;
        vm->por = 0;

        vm->j = 0;
        vm->o = 0;
        vm->r = 0;

        vm->c = 0;
        vm->d = 0;
        vm->l = 0;

        vm->g = 0;
        vm->b = 0;
        vm->i = 0;
}


uchar
myth_fetch(struct myth_vm *vm) /*Fetch next byte in CODE stream, increment PC*/
{
        uchar val = vm->pagebyte[ vm->c][ vm->j];
        (vm->j)++;
        return val;
}


void
myth_cycle(struct myth_vm *vm)
{

        uchar opcode = myth_fetch(vm);

                /*Decode priority encoded opcode*/
                /*Execute decoded instruction*/

                if (opcode&0x80) myth_exec_pair(vm, opcode);
                else if (opcode&0x40) myth_exec_gput(vm, opcode);
                else if (opcode&0x20) myth_exec_trap(vm, opcode);
                else if (opcode&0x10) myth_exec_alu(vm, opcode);
                else if (opcode&0x08) myth_exec_adj(vm, opcode);
                else myth_exec_sys(vm, opcode);
}


void
myth_call(struct myth_vm *vm, uchar dstpage)
{
        vm->o = vm->j;
        vm->j = 0;
        vm->d = vm->c;
        vm->c = dstpage;
        vm->l -= 1;
}


uchar
myth_exec_pair_srcval(struct myth_vm *vm, uchar srcreg)
{
        switch(srcreg){
                case Nx: return myth_fetch(vm); /*pseudo reg*/
                case DMx: return vm->pagebyte[ vm->d][ vm->o]; /*pseudo reg*/
                case LMx: return vm->pagebyte[ vm->l][ vm->o]; /*pseudo reg*/
                case Gx: return vm->g;
                case Rx: return vm->r;
                case Ix: return vm->i;
                case Sx: return vm->sir;
                default /*Px*/: return vm->pir;
        }
}


void
myth_exec_pair(struct myth_vm *vm, uchar opcode)
{
                uchar srcreg = (opcode >> 4) & 7; /*Zero except bits 4-6 at LSB*/
                uchar dstreg = opcode & 15; /* Zero except bits 0-3 at LSB*/

                /* SCROUNGING
                   Remap ("scrounge") memory-to-memory opcodes to NOP. */

                if ((srcreg==Nx || srcreg==DMx || srcreg==LMx)
                        && (dstreg==xDM || dstreg==xLM))
                                return;

        uchar srcval = myth_exec_pair_srcval(vm, srcreg);
        switch(dstreg){
                case xO: vm->o = srcval; break;
                case xDM: /*pseudo reg*/
                        vm->pagebyte[ vm->d][ vm->o] = srcval;
                        break;
                case xLM: /*pseudo reg*/
                        vm->pagebyte[ vm->l][ vm->o] = srcval;
                        break;
                case xG: vm->g = srcval; break;
                case xR: vm->r = srcval; break;
                case xI: vm->i = srcval; break;
                case xS: vm->sor = srcval; break;
                case xP: vm->por = srcval; break;

                case xE: vm->e = srcval; break;
                case xB: vm->b = srcval; break;
                case xD: vm->d = srcval; break;
                case xJ: /*pseudo reg*/
                        vm->j += (signed char) srcval;
                        break;
                case xW: /*pseudo reg*/
                        if (vm->i) vm->j += (signed char) srcval;
                        (vm->i)--; /*Post decrement always*/
                        break; 
                case xT: /*pseudo reg*/
                        if (vm->r) vm->j += (signed char) srcval;
                        break;
                case xF: /*pseudo reg*/
                        if (!vm->r) vm->j = (signed char) srcval;
                        break;
                case xC: myth_call(vm, srcval); break; /*pseudo reg*/
        }
}


void
myth_exec_gput(struct myth_vm *vm, uchar opcode) /*Execute GETPUT instruction*/
{
        /* OPCODE
            BITS 0-1 encode registers RODG
            BIT 2 encodes GET/PUT mode
            BITS 3-5 encode address offset
        */

        #define BIT2 4

        uchar *mptr;
        uchar offs = (opcode >> 3) & 7; /*Zero except bits 3-5 at LSB*/
        
        mptr = &(vm->pagebyte[vm->l][0xFC + offs]);
        if(opcode & BIT2)
                switch(opcode & 3){ /*Zero except low order 2 bits*/
                        case 0: *mptr = vm->r;
                        case 1: *mptr = vm->o;
                        case 2: *mptr = vm->d;
                        case 3: *mptr = vm->g;
                }
        else
                switch(opcode & 3){ /*Zero except low order 2 bits*/
                        case 0: vm->r = *mptr;
                        case 1: vm->o = *mptr;
                        case 2: vm->d = *mptr;
                        case 3: vm->g = *mptr;
                }
}


void
myth_exec_trap(struct myth_vm *vm, uchar opcode)
{
        uchar dstpage = opcode & 31; /*Zero except low order 5 bits*/
        myth_call(vm, dstpage);
}


void
myth_exec_alu(struct myth_vm *vm, uchar opcode)
{
        switch(opcode & 15){/* Zero except low order 4 bits*/
                case IDR: vm->r = vm->r; break;
                case IDO: vm->r = vm->o; break;
                case OCR: vm->r = ~vm->r; break;
                case OCO: vm->r = ~vm->o; break;
                case SLR: vm->r = vm->r << 1; break;
                case SLO: vm->r = vm->o << 1; break;
                case SRR: vm->r = vm->r >> 1; break;
                case SRO: vm->r = vm->o >> 1; break;
                case AND: vm->r = vm->r & vm->o; break;
                case IOR: vm->r = vm->r | vm->o; break;
                case EOR: vm->r = vm->r ^ vm->o; break;
                case ADD: vm->r = vm->r + vm->o; break;
                case CAR:
                        vm->r = (int) vm->r + (int) vm->o > 255 ? 1 : 0;
                        break;
                case RLO: vm->r = (vm->r < vm->o) ? 255 : 0; break;
                case REO: vm->r = (vm->r == vm->o) ? 255 : 0; break;
                case RGO: vm->r = (vm->r > vm->o) ? 255 : 0; break;
        }
}


void /*Adjust R by sign-extended offset*/
myth_exec_adj(struct myth_vm *vm, uchar opcode)
{
        switch(opcode & 7){ /*Zero except low order 3 bits*/
                case 0: vm->r += 4; break;
                case 1: vm->r += 1; break;
                case 2: vm->r += 2; break;
                case 3: vm->r += 3; break;
                case 4: vm->r -= 4; break;
                case 5: vm->r -= 3; break;
                case 6: vm->r -= 2; break;
                case 7: vm->r -= 1; break;
        }
}


void
myth_exec_sys(struct myth_vm *vm, uchar opcode)
{
        switch(opcode & 7){ /*Zero except low order 3 bits*/
                
                case NOP:
                        break;
                case SSI:
                        /*Clocks in MISO line bit into LSB*/
                        vm->sir = ((vm->sir)<<1) + vm->miso;
                        break;
                case SSO:
                        vm->mosi = (vm->sor)&0x80 ? 1:0;
                        vm->sor <<= 1; /*Clocks out MSB first*/
                        break;
                case SCL:
                        vm->sclk = 0;
                        break;
                case SCH:
                        vm->sclk = 1;
                        break;
                
                case RET: vm->l += 1; /*FALL THROUGH*/
                case FAR:
                        vm->c = vm->d;
                        vm->j = vm->o;
                        break;
                
                case ORG:
                        vm->o = vm->j;
                        vm->d = vm->c;
                        break;
        }
}

