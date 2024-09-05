#include <u.h>
#include <libc.h>

void load( struct myth_vm*, char *);
void save( struct myth_vm*, char *);

void
load(struct myth_vm *vm, char *fname)
{
        int fdesc;
        fdesc=open(fname, OREAD);
        if(fdesc != -1)
                read(fdesc, vm, sizeof(struct myth_vm));
        else{
                print("Creating missing corestate file '%s'\n", fname);
                myth_reset(vm);
                create(fname, 0, 0666);
                fdesc=open(fname, OWRITE);
                if (fdesc != -1) write(fdesc, vm, sizeof(struct myth_vm));
                else print("Write error\n");
        }
        close(fdesc);
}

void
save(struct myth_vm *vm, char *fname)
{
        int fdesc;
        create(fname, 0, 0666);
        fdesc=open(fname, OWRITE);
        if (fdesc != -1) write(fdesc, vm, sizeof(struct myth_vm));
        else print("Write error\n");
        close(fdesc);
}
