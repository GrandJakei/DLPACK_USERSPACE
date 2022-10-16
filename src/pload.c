#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#define PATH_LOAD "/sys/kernel/security/dlpack/loadRules"
#define PATH_CLEAN "/sys/kernel/security/dlpack/changeRules"
#define PATH_POLICY "result"
#define PATH_SERVICE_PID "/sbin/dlpack/pid"
//#define PATH_POLICY "result"
#define CHAR_MAX_LENGTH 256
char to_deal[CHAR_MAX_LENGTH];  // 待处理的一行文本

int main() {

	clock_t start,stop;
	start = clock();
    FILE *f_policy = fopen(PATH_POLICY, "r");
    FILE *f_load = fopen(PATH_LOAD, "w");
    FILE *f_clean = fopen(PATH_CLEAN, "w");

    if(f_policy == NULL){
        printf("error : fail to open profile f_policy in %s! please run pcheck first.\n", PATH_POLICY);
        goto error;
    }
    if(f_load == NULL){
        printf("error : fail to write in %s! may not have enough permission.\n", PATH_LOAD);
        goto error;
    }
    if(f_clean == NULL){
        printf("error : fail to write in %s! may not have enough permission.\n", PATH_CLEAN);
        goto error;
    }

    // 清空内核策略
    int index, pid;
    fputs("clean", f_clean);
    fclose(f_clean);

    // 
    int i  = 0;
    while (fgets(to_deal, CHAR_MAX_LENGTH, f_policy) != NULL){
        to_deal[strlen(to_deal) - 1] = '\0';
    	fputs(to_deal, f_load);
    	fflush(f_load);
    	printf("to_kernel %d: %s \n", ++i, to_deal);
    }
    printf("policy loading done successfully! \n");
    fclose(f_load);
    fclose(f_policy);
    stop = clock();
    printf("duration is : %f \n",((double)(stop-start))/CLOCKS_PER_SEC);
error:
    return 0;
}

