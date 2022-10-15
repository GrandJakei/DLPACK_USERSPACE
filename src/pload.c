#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#define PATH_LOAD "/sys/kernel/security/dlpack/loadRules"
#define PATH_CLEAN "/sys/kernel/security/dlpack/changeRules"
#define PATH_POLICY "result"
#define PATH_SERVICE_PID "/tmp/dlpack_pid"
//#define PATH_POLICY "result"
#define CHAR_MAX_LENGTH 256
char to_deal[CHAR_MAX_LENGTH];  // 待处理的一行文本

int main() {
    int f_policy = open(PATH_POLICY, O_RDONLY);
    int f_load = open(PATH_LOAD, O_RDWR);
    int f_clean = open(PATH_CLEAN, O_RDWR);
    int f_service = open(PATH_SERVICE_PID, O_RDONLY);


    key_t key = 39242235;
	int pid = 0;


    if(!f_policy){
        printf("error : fail to open profile f_policy in %s! please run pcheck first.\n", PATH_POLICY);
        goto error;
    }
    if(!f_load){
        printf("error : fail to write in %s! may not have enough permission.\n", PATH_LOAD);
        goto error;
    }
    if(!f_clean){
        printf("error : fail to write in %s! may not have enough permission.\n", PATH_CLEAN);
        goto error;
    }
     if(!f_service){
        printf("error : fail to communicate with state collection service, pls make sure it's working normally.\n");
    } else {
    	read(f_service, to_deal, CHAR_MAX_LENGTH);
        pid = atoi(to_deal);
    }

    if (pid != 0)
        kill(pid, SIGUSR1);

    // 清空内核策略
    write(f_clean, "clean", 6);

    // 
    while (read(f_policy, to_deal, CHAR_MAX_LENGTH) > 0){
        to_deal[strlen(to_deal) - 1] = '\0';
    	write(f_load, to_deal, strlen(to_deal));
    	printf("to_kernel : %s \n", to_deal);
    }
    printf("policy loading done successfully! \n");

    if (pid != 0)
        kill(pid, SIGUSR2);

    close(f_load);
    close(f_policy);
    close(f_clean);
error:
    return 0;
}
