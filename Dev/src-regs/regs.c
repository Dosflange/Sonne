/*
        Myth Project
    Dump LOX registers for debugging.
    It expects (or creates) the file "corestate.myst" in the working directory.

    Edit nettle, then run nettle, lox and regs in succession.

    Author: mim@ok-schalter.de (Michael/Dosflange@github)

    Requires a Plan 9 build environment:
    https://github.com/9fans/plan9port

    Build using:
    9c <progname>.c
    9l <progname>.o
    
    Run:
    ./a.out
*/

#include <u.h>
#include <libc.h>
#include "../src-myst/myth.h"
#include "../src-myst/myst.h"

char* fname="corestate.myst";
int i;
int n;

void
main( int argc, char *argv[])
{
        struct myth_vm vm;
        load( &vm, fname);

        print( "LOX regs: ", vm.l);

        print( "r:%.02Xh(%d)(%b) ", vm.r, vm.r, vm.r);
        print( "o:%.02Xh(%d)(%b) ", vm.o, vm.o, vm.o);
        print( "pc:%.02Xh(%d)(%b) ", vm.pc, vm.pc, vm.pc);
        print( "c:%.02Xh(%d)(%b) ", vm.c, vm.c, vm.c);
        print( "d:%.02Xh(%d)(%b) ", vm.d, vm.d, vm.d);
        print( "g:%.02Xh(%d)(%b) ", vm.g, vm.g, vm.g);
        print( "i:%.02Xh(%d)(%b)\n", vm.i, vm.i, vm.i);

        print( "\te:%.02Xh(%d)(%b) ", vm.e, vm.e, vm.e);
        print( "sclk:%d ", vm.sclk ? 1:0);
        print( "miso:%d ", vm.miso ? 1:0);
        print( "mosi:%d ", vm.mosi ? 1:0);
        print( "sir:%.02Xh(%d)(%b) ", vm.d, vm.d, vm.d);
        print( "sor:%.02Xh(%d)(%b) ", vm.g, vm.g, vm.g);
        print( "pir:%.02Xh(%d)(%b) ", vm.i, vm.i, vm.i);
        print( "por:%.02Xh(%d)(%b)\n", vm.i, vm.i, vm.i);

        print( "Locals @l%.02X: ", vm.l);
        for( i=0; i<8; i++){
                n = vm.pagebyte[vm.l][0xF8+i];
                print( "L%d:%X(%d)(%b) ", i, n, n, n);
        }

        print( "\n");
        exits( "");
}


 