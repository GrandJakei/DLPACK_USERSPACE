#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#define PATH_LOAD "/sys/kernel/security/dlpack/loadRules"
#define PATH_POLICY "result"
#define PATH_SERVICE_PID "/sbin/dlpack/pid"
//#define PATH_POLICY "result"
#define CHAR_MAX_LENGTH 256
int to_kernel_max_len = 512, to_kernel_now_len = 0;
char to_deal[CHAR_MAX_LENGTH];  // 待处理的一行文本
char *to_kernel;

int append(){
    int len = strlen(to_deal);
    if (len + to_kernel_now_len >= to_kernel_max_len){
    	to_kernel_max_len = to_kernel_max_len * 2;
    	char *tmp = malloc(sizeof(char) * to_kernel_max_len);
    	strcpy(tmp, to_kernel);
    	free(to_kernel);
    	to_kernel = tmp;
    }
    to_kernel_now_len += len;
    strcat(to_kernel, to_deal);
}
int main() {
    FILE *f_policy = fopen(PATH_POLICY, "r");
    FILE *f_load = fopen(PATH_LOAD, "w");
    if(f_policy == NULL){
        printf("error : fail to open profile f_policy in %s! please run pcheck first.\n", PATH_POLICY);
        goto error;
    }
    if(f_load == NULL){
        printf("error : fail to write in %s! may not have enough permission.\n", PATH_LOAD);
        goto error;
    }
    int index, pid;
    to_kernel = malloc(sizeof(char) * to_kernel_max_len);
    while (fgets(to_deal, CHAR_MAX_LENGTH, f_policy) != NULL){
    	append();
    }
    to_kernel[strlen(to_kernel) - 1] = '\0';
    printf("to_kernel : %s \n", to_kernel);
    fputs(to_kernel, f_load);
    printf("policy loading done successfully! \n");
    fclose(f_load);
    fclose(f_policy);
error:
    return 0;
}

/*
//上面的是单纯输入策略的版本，下面注释掉的是可以与场景收集服务互动的版本

int main() {
    FILE *f_policy = fopen(PATH_POLICY, "r");
    FILE *f_load = fopen(PATH_LOAD, "w");
    //FILE *f_service = fopen(PATH_SERVICE_PID, "r");
    if(f_policy == NULL){
        printf("error : fail to open profile f_policy in %s! please run pcheck first.\n", PATH_POLICY);
        goto error;
    }
    if(f_load == NULL){
        printf("error : fail to write in %s! may not have enough permission.\n", PATH_LOAD);
        goto error;
    }
    //if(f_service == NULL){
        //printf("error : fail to communicate with state collection service, pls make sure it's working normally.\n");
        //goto error;
    //}


    int index, pid;
    if (fgets(to_deal, CHAR_MAX_LENGTH, f_service) != NULL){
    	pid = atoi(to_deal);
    } else {
    	printf("error : fail to communicate with state collection service, pls make sure it's working normally.\n");
        goto error;
    }
    kill(pid, SIGUSR1);
    while (fgets(to_deal, CHAR_MAX_LENGTH, f_policy) != NULL){
        index = 0;
        rewind(f_load);
        while (to_deal[index] != '\0'){
            if (to_deal[index] == '\n' || to_deal[index] == '\r'){
                to_deal[index] = '\0';
                break;
            }
            index ++;
        }
        if(index != 0){
            fputs(to_deal, f_load);
        }
    }
    //system("dmesg | grep dlpack");
    printf("policy loading done successfully!");
    //kill(pid, SIGUSR2);
    
error:
    fclose(f_load);
    fclose(f_policy);
    //fclose(f_service);
    return 0;
}
*/