#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

void *thread(void *x) { open("x/blockme", 0); return 0; }

int main() {
    pthread_t t;
    pthread_create(&t, 0, thread, 0);
    sleep(1);

    puts("spawned thread");

    FILE *mapsfile = fopen("/proc/self/maps", "r");

    struct m { long start, end; };
    static struct m maps[128]; // oughta be enough for anyone
    int i = 0;

    while (fscanf(mapsfile, "%lx-%lx %*[^\n]", &maps[i].start, &maps[i].end) != EOF) i++;
    puts("got all the maps");

    // - copy all the data we'll need in a new map
    // - add this new map's address at the end of the struct
    // - jit some code that does this:
    //   void f() { struct m *x = f-2048; for (i in 0..n) munmap(m[i].start...); }
    //   ^ will segfault when returning from the final munmap
    void *newmap = mmap(0, 4096, 7, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    maps[i].start = (long)newmap;
    maps[i].end = (long)newmap + 4096;

    memcpy(newmap, maps, sizeof maps);

#if 0
static inline long munmap(long a, long b) {
    long ret;
    asm volatile("syscall" : "=a"(ret) : "a"(11), "D"(a), "S"(b) : "cc", "r11", "rcx");
    return ret;
}

void f() {
    struct m { long start, end; } *p = (void *)((char *) f - 2048);
    while (1) {
        munmap(p->start, p->end - p->start);
        p++;
    }
}
#endif

    unsigned char bytes[] = {
/* 0000000000000000 <f>:                                                                                  */
/*    0:  */  0x48, 0xba, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    /*  movabs $0x0,%rdx         */
/*                        ^^^^^^^^^^^^^^^^ f's address ^^^^^^^^^^^^^^^^^^                                 */
/*    a:  */  0x41, 0xb8, 0x0b, 0x00, 0x00, 0x00,                            /*  mov    $0xb,%r8d         */
/*   10:  */  0x48, 0x8b, 0xba, 0x00, 0xf8, 0xff, 0xff,                      /*  mov    -0x800(%rdx),%rdi */
/*   17:  */  0x48, 0x8b, 0xb2, 0x08, 0xf8, 0xff, 0xff,                      /*  mov    -0x7f8(%rdx),%rsi */
/*   1e:  */  0x44, 0x89, 0xc0,                                              /*  mov    %r8d,%eax         */
/*   21:  */  0x48, 0x29, 0xfe,                                              /*  sub    %rdi,%rsi         */
/*   24:  */  0x0f, 0x05,                                                    /*  syscall                  */
/*   26:  */  0x48, 0x83, 0xc2, 0x10,                                        /*  add    $0x10,%rdx        */
/*   2a:  */  0xeb, 0xe4                                                     /*  jmp    10 <f+0x10>       */
    };

    void (*f)(void) = (void *)(char *)newmap + 2048;
    memcpy(bytes+2, &f, 8);
    memcpy(f, bytes, sizeof bytes);

    printf("BTW MY PID IS %d\n", getpid());

    puts("start unmapping like crazy");
    f();
}
