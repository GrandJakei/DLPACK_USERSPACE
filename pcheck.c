/* 简单的文本解析器*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "read_policy.h"

typedef struct sub_state sub_state;
const int ATOMIC_NUM = 8;   // 原子场景数量（有n个原子场景，state的数值就要n个bit）

char to_deal[CHAR_MAX_LENGTH];  // 待处理的一行文本
char cut_str[CHAR_MAX_LENGTH];  // to_deal的部分文本
//char bk_str[CHAR_MAX_LENGTH];   // 备用文本

struct state_info *states_head;         // 现存的所有state （头节点保存数值）
struct state_info *state_edit_now;      // 在解析策略文件时正在编辑的state结构体
struct permission_info *per_head;       // 现存的所有Permission结构体 （头节点保存数值）
struct permission_info *per_edit_now;   // 在解析策略文件时正在编辑的Permission结构体
struct sub_state *sub_state_head;       //（空 头节点）

int state_num = 0;          // 读取的state数量
int permission_num = 0;     // 读取的permission数量

// 初始化变量和链表头
void init(){
    states_head = (struct state_info*)malloc(sizeof(struct state_info));
    states_head->next = NULL;

    per_head = (struct permission_info*)malloc(sizeof(struct permission_info));
    per_head->rule_head = (struct rule_info*)malloc(sizeof(struct rule_info));
    per_head->rule_head->next = NULL;
    per_head->next = NULL;

    sub_state_head = (struct sub_state*)malloc(sizeof(struct sub_state));
    sub_state_head->next = NULL;

    state_edit_now = states_head;
    per_edit_now = per_head;
}

// 释放所有占用的内存
void free_all_mem(){
    struct state_info *state_free = states_head;
    struct permission_info *per_free = per_head;
    struct rule_info *rule_free, *rule_head_tmp;
    struct sub_state *sub_free = sub_state_head;

    states_head = states_head->next;
    per_head = per_head->next;
    sub_state_head = sub_state_head->next;
    free(state_free);
    free(per_free);
    free(sub_free);

    while(states_head != NULL){
        free(states_head->state_name);
        free(states_head->state_num);
        free(states_head->permissions);
        state_free = states_head;
        states_head = states_head->next;
        free(state_free);
    }
    while(per_head != NULL){
        free(per_head->permission_name);
        rule_free = per_head->rule_head;
        rule_head_tmp = per_head->rule_head->next;
        free(rule_free);
        while(rule_head_tmp != NULL){
            free(rule_head_tmp->object_type);
            free(rule_head_tmp->keyword);
            free(rule_head_tmp->file_path);
            free(rule_head_tmp->file_flags);
//            free(rule_head_tmp->cap_num);
            rule_free = rule_head_tmp;
            rule_head_tmp = rule_head_tmp->next;
            free(rule_free);
        }
        per_free = per_head;
        per_head = per_head->next;
        free(per_free);
    }
    sub_state_head = sub_state_head->next;
    while(sub_state_head != NULL){
        free(sub_state_head->sub_num);
        sub_free = sub_state_head;
        sub_state_head = sub_state_head->next;
        free(sub_free);
    }
}

// 
int check_length_and_modify(int n){
    int index = 0;
        while (to_deal[index] != '\0'){
            if (to_deal[index] == '#')
                return 0;
            if (index != CHAR_MAX_LENGTH -  1){
                if (to_deal[index] == '\r' || to_deal[index] == '\n'){
                    to_deal[index] = '\0';
                    break;
                }
                index ++;
            } else {
                switch (n){
                    case 1:
                        printf("error : there is a line longer than %d character in file: .permissions\n", CHAR_MAX_LENGTH);
                        break;
                    case 2:
                        printf("error : there is a line longer than %d character in file: per_rule\n", CHAR_MAX_LENGTH);
                        break;
                    case 3:
                        printf("error : there is a line longer than %d character in file: states\n", CHAR_MAX_LENGTH);
                        break;
                    case 4:
                        printf("error : there is a line longer than %d character in file: state_per\n", CHAR_MAX_LENGTH);
                        break;
                }
                return -1;
            }
        }
}

/* 检查两个permission集合里面是否有冲突 */
int is_conflict(const int* set1, const int* set2){
    for(int i = 0; i < permission_num; ++i){
        if(set1[i] != set2[i])
            return 1;
    }
    return 0;
}

/* 检查state1是否是state2的子集 */
int is_subset(const char* state1, const char* state2){
    for(int i = 0; i < ATOMIC_NUM; ++i){
        if(state1[i] != state2[i] && state2[i] != '*')
            return 0;
    }
    return 1;
}

/* 检查两个state是否存在交集 */
int is_intersections(const char* state1, const char* state2, int len){
    for (int i = 0; i < len; ++i){
        if (state1[i] != state2[i] && state1[i] != '*' && state2[i] != '*')
            return 0;
    }
    return 1;
}

/* 检查两个state是否存在交集，并且交集内存在不可调和的冲突; 如存在则终止程序 */
int check_intersections(){
    struct state_info *state1 = states_head->next, *state2;
    while (state1 != NULL){
        state2 = state1->next;
        while (state2 != NULL){
            if (is_intersections(state1->state_num, state2->state_num, ATOMIC_NUM) &&
               is_conflict(state1->permissions, state2->permissions) &&
               !is_subset(state1->state_num, state2->state_num) &&
               !is_subset(state2->state_num, state1->state_num)){
                // 产生了冲突，并且不是父子集合，则提示管理员冲突的地方在哪里
                printf("error : conflict between state: %s and state: %s, better make specific config for their intersection part.\n", state1->state_name, state2->state_name);
                exit(0);
            }
            state2 = state2->next;
        }
        state1 = state1->next;
    }
    return 0;
}

/* 检查是否早就存在一个拆解后的原子场景组合，如果存在就判断原先的父state和现在的父state谁是谁的子集，取更具体的为准 */
int sub_already_exist(struct state_info *father, char* state_in_edit){
    struct sub_state *on_chain = sub_state_head->next;
    while (on_chain != NULL){
        if (strcmp(on_chain->sub_num, state_in_edit) == 0){
            if (is_subset(father->state_num, on_chain->father->state_num))
                on_chain->father = father;
            return 1;
        }
        on_chain = on_chain->next;
    }
    return 0;
}

// 拆分某一个单独的state
int single_state_split(struct state_info *father, char* state_in_edit, int index, struct sub_state *sub){
    if(index == ATOMIC_NUM){
        if(sub_already_exist(father, state_in_edit))
            return 0;
        struct sub_state *temp = (struct sub_state *) malloc(sizeof(sub_state));
        temp->father = father;
        temp->sub_num = (char *) malloc(sizeof(char) * (ATOMIC_NUM + 1));
        strcpy(temp->sub_num, state_in_edit);
        temp->next = sub->next;
        sub->next = temp;
    } else if (state_in_edit[index] == '*'){
        state_in_edit[index] = '0';
        single_state_split(father, state_in_edit, index + 1, sub);
        state_in_edit[index] = '1';
        single_state_split(father, state_in_edit,index + 1, sub);
        state_in_edit[index] = '*';
    } else {
        single_state_split(father, state_in_edit,index + 1, sub);
    }
    return 0;
}

// 拆分所有的state，并且处理好子集问题
int state_split(){
    char * temp_char = (char *) malloc(sizeof(char) * ATOMIC_NUM + 1);
    char * to_free = temp_char;
    state_edit_now = states_head->next;
    while (state_edit_now != NULL){
        strcpy(temp_char, state_edit_now->state_num);
        single_state_split(state_edit_now, temp_char, 0, sub_state_head);
        state_edit_now = state_edit_now->next;
    }
    free(to_free);
    return 0;
}


///* 十六进制读取辅助函数 */
//int getIndexOfSigns(char ch){
//    if (ch >= '0' && ch <= '9')
//        return ch - '0';
//    if (ch >= 'A' && ch <='F')
//        return ch - 'A' + 10;
//    if (ch >= 'a' && ch <= 'f')
//        return ch - 'a' + 10;
//    return -1;
//}

///* 十六进制读取 */
//long hexToDec(char *source){
//    long sum = 0;
//    long t = 1;
//    int i, len;
//    len = strlen(source);
//    for(i=len-1; i>=0; i--){
//        sum += t * getIndexOfSigns(*(source + i));
//        t *= 16;
//    }
//    return sum;
//}

int find_state(char *name){
    state_edit_now = states_head->next;
    while(state_edit_now != NULL){
        if(strcmp(state_edit_now->state_name, name) == 0)
            return 0;
        state_edit_now = state_edit_now->next;
    }
    return -1;
}

int find_permission(char *name){
    per_edit_now = per_head->next;
    while(per_edit_now != NULL){
        if(strcmp(name, per_edit_now->permission_name) == 0){
            return 0;
        }
        per_edit_now = per_edit_now->next;
    }
    return -1;
}

void set0(int * set){
    for(int i = 0; i < permission_num; ++i){
        *(set + i) = 0;
    }
}

/* 解析单行的state名称，存储为标号 */
int read_single_state(int line_num){
    int index, len, start = 0;
    struct state_info *state_tmp;
    if(to_deal[0] == '$' && to_deal[1] == 'S'){ // 待优化，现在只是去识别第一个字符是不是 $
        index = 3;
        while(to_deal[index] != '\0'){      
            if(to_deal[index] == ' '){  // 硬规定不能出现多余的空格
                if (index >= 27){
                    printf("error : State name should not longer than 30 characters in file: states line %d!\n", line_num);
                    return -1;
                }
                start = index + 1;
                strncpy(cut_str, to_deal+3, index - 3);
                cut_str[index - 3] = '\0';
                for (int i = 0; i < index - 3; ++i){
                    if (cut_str[i] != '_' && (cut_str[i] < 48 || (cut_str[i] > 57 && cut_str[i] < 65) ||  cut_str[i] > 90)){
                        printf("error : wrong State name in file: states line %d!\n", line_num);
                        return -1;
                    }
                }
                state_tmp = (struct state_info *) malloc(sizeof(struct state_info));
                state_tmp->state_name = (char *)malloc(sizeof(char) * (index - 3 + 1));
                strcpy(state_tmp->state_name, cut_str);
                break;
            }
            index ++;
        }
        if(start != 0){
            len = strlen(to_deal + start);
            if(len != ATOMIC_NUM){
                printf("error : wrong state length in file: states line %d, pls set exactly %d atomic bits behind state name\n", line_num, ATOMIC_NUM);
                return -1;
            }
            for (int i = start; i < start + len; ++ i){
            	
                if (to_deal[i] != '*' && to_deal[i] != '0' && to_deal[i] != '1'){
                    printf("error : wrong state value in file: states line %d\n", line_num);
                    return -1;
                }
            }
            state_tmp->state_num = (char *)malloc(sizeof(char) * (len + 1));
            strcpy(state_tmp->state_num, to_deal + start);
            state_tmp->permissions = (int *)malloc(sizeof(int) * permission_num);
            set0(state_tmp->permissions);
            state_tmp->next = states_head->next;
            states_head->next = state_tmp;

            state_num ++;
            return 0;
        }
        printf("error : state format in file: states line %d \n", line_num);
        return -1;
    } else if (to_deal[0] != '#'){
        printf("error : please use \"$S\" to identify a state name or \"#\" to identify annotation at first character in file states line %d\n", line_num);
        return -1;
    }
    return 0;
}

/* 将某state下的某permission记录下来 */
int add_state_permission(char *per_name){
    if(find_permission(per_name) == -1)
        return -1;
    state_edit_now->permissions[per_edit_now->permission_num] = 1;
//    printf("state: %s; %d; \n", state_edit_now->state_num, per_edit_now->permission_num);
    return 0;

//    struct rule_info *rule_head = per_edit_now->rule_head;
//    while(rule_head != NULL){
//        if(strcmp(rule_head->object_type, "file") == 0){
//            printf("state: %x; %s; %s; %s; %s; \n", state_edit_now->state_num, rule_head->keyword, rule_head->object_type, rule_head->file_path, rule_head->file_flags);
//        }else{
//            printf("state: %x; %s; %s; %s; \n", state_edit_now->state_num, rule_head->keyword, rule_head->object_type, rule_head->cap_num);
//        }
//        rule_head = rule_head->next;
//    }
//    return 0;
}

/* 解析单行的state-permission配置 */
int read_single_state_permission(int line_num){
    int index = 0, start = 0, end;
    if(to_deal[0] == '$' && to_deal[1] == 'S'){ // 如果是某一块State的配置开始
        start = 3;
        while(to_deal[index] != '\0'){
            if(to_deal[index] == ':'){
                end = index;
                strncpy(cut_str, to_deal+start, end - start);
                cut_str[end - start] = '\0';
                if(find_state(cut_str) == -1){
                    printf("error : wrong state name in state_permission line %d \n", line_num);
                    return -1;
                }else return 0;
            }
            index++;
        }
        printf("error : lack of ':' in state-permission line %d \n", line_num);
        return -1;
    }
    while(to_deal[index] != '\0'){
        if(to_deal[index] == '#')
            return 0;
        if(to_deal[index] == '+'){
            if(state_edit_now->state_name == NULL){
                printf("error : no state to set in state-permission line %d \n", line_num);
                return -1;
            }
            if(to_deal[index+1]==' ' && to_deal[index+2]=='$' && to_deal[index+3]=='P' && to_deal[index+4]==' '){
                index += 5;
                start = index;
            } else {
                printf("error : wrong Permission format in file: state-permission line %d, it should be like \"+$P PERMISSION_1\" \n", line_num);
                return -1;
            }
            if(start != 0){
                if(add_state_permission(to_deal + start) == -1){
                    printf("error : wrong permission name in state-permission line %d \n", line_num);
                    return -1;
                }
            }
            return 0;
        }
        if (to_deal[index] != ' '){
            printf("error : wrong format in file: state_per line %d \n", line_num);
            return -1;
        }
        index ++;
    }
    return 0;
}

/* 解析单行的permission-rule配置，完善Permission的链表 */
int read_single_permission_rule(int line_num){
    int index = 0, start, end = 0, len;
    struct rule_info *temp_rule;
    char *record;
    if(to_deal[0] == '$' && to_deal[1] == 'P'){ // 如果是某一块Permission的配置开始
        start = 3;
        while(to_deal[index] != '\0'){
            if(to_deal[index] == ':'){
                end = index;
            }
            if(to_deal[index] == '\r' || to_deal[index] == '\n'){
                to_deal[index] = '\0';
                break;
            }
            index ++;
        }
        strncpy(cut_str, to_deal+start, end - start);
        cut_str[end - start] = '\0';
        if(find_permission(cut_str) == -1){
            printf("error : wrong permission name in permission_rule line %d \n", line_num);
            return -1;
        }else return 0;
//        printf("error : lack of ':' in permission_rule line %d \n", line_num);
//        return -1;
    }
    while(to_deal[index] != '\0'){
        if(to_deal[index] == '#')
            return 0;
        if(to_deal[index] == '+'){
            if(per_edit_now->permission_name == NULL){
                printf("error : no permission to set in in file: permission_rule line %d \n", line_num);
                return -1;
            }
            temp_rule = (struct rule_info*) malloc(sizeof(struct rule_info));
            temp_rule->object_type = NULL;
            temp_rule->keyword = NULL;
            temp_rule->next = NULL;
//            temp_rule->cap_num = NULL;
            temp_rule->file_flags = NULL;
            temp_rule->file_path = NULL;

            len = (int)strlen(to_deal);
            strncpy(cut_str, to_deal+index + 1,  len - index);
            cut_str[len - index] = '\0';
            /* 获取第一个子字符串 */
            record = strtok(cut_str, " ");
            if (record == NULL || (strcmp(record, "allow") != 0 && strcmp(record, "audit") != 0)){
                printf("error : wrong key word in file: permission_rule line %d, it should be \"allow\" or \"audit\" \n", line_num);
                return -1;
            }
            temp_rule->keyword = (char *)malloc(sizeof(char) * (strlen(record) + 1));
            strcpy( temp_rule->keyword, record);


            /* 继续获取其他的子字符串 */
            record = strtok(NULL, " ");
            if (record == NULL || (strcmp(record, "file") != 0 && strcmp(record, "cap") != 0)){
                printf("error : wrong object type in file: permission_rule line %d, it should be \"file\" or \"cap\" \n", line_num);
                return -1;
            }
            temp_rule->object_type = (char *)malloc(sizeof(char) * (strlen(record) + 1));
            strcpy( temp_rule->object_type, record);
            if (strcmp(temp_rule->object_type, "file") == 0){
                record = strtok(NULL, " ");
                if (record == NULL){
                    printf("error : pls write file path after object type in file: permission_rule line %d \n", line_num);
                    return -1;
                }
                temp_rule->file_path = (char *)malloc(sizeof(char) * (strlen(record) + 1));
                strcpy( temp_rule->file_path, record);

                record = strtok(NULL, " ");
                if (record == NULL){
                    printf("error : pls write file limits after file path in file: permission_rule line %d \n", line_num);
                    return -1;
                }
                if (strlen(record) > 3){
                    printf("error : wrong file limits format in file: permission_rule line %d \n", line_num);
                    return -1;
                }
                for (int i = 0; i < strlen(record); i++){
                    if (record[i] != 'r' && record[i] != 'w' && record[i] != 'x'){
                        printf("error : wrong file limits format in permission_rule line %d \n", line_num);
                        return -1;
                    }
                }
                temp_rule->file_flags = (char *)malloc(sizeof(char) * (strlen(record) + 1));
                strcpy( temp_rule->file_flags, record);
                record = strtok(NULL, " ");
                if (record != NULL){
                    printf("error : pls dont add other things after file limits in file: permission_rule line %d \n", line_num);
                    return -1;
                }
            } else {
                record = strtok(NULL, " ");
                if (record == NULL){
                    printf("error : pls write capability number in file: permission_rule line %d \n", line_num);
                    return -1;
                }
                record[strlen(record)] = '\0'; // fix
                for (int i = 0; i < strlen(record); i++){
                    if (record[i] < 48 || record[i] > 57){
                        printf("error : wrong cap num format in file: permission_rule line %d \n", line_num);
                        return -1;
                    }
                }
                int capnum = atoi(record);
                if (capnum < 0 || capnum > 40){
                    printf("error : wrong cap num in file: permission_rule line %d, it should be in 0-40 \n", line_num);
                    return -1;
                }
                temp_rule->cap_num = capnum;
//                temp_rule->cap_num = (char *)malloc(sizeof(char) * (strlen(record) + 1));
//                strcpy( temp_rule->cap_num, record);
            }
            temp_rule->next = per_edit_now->rule_head->next;
            per_edit_now->rule_head->next = temp_rule;
            return 0;
        }
        if (to_deal[index] != ' '){
            printf("error : wrong format in file: permission_rule line %d \n", line_num);
            return -1;
        }
        index ++;
    }
    return 0;
}

/* 初始化 permission 的链表 */
int permission_name_init(){
    struct permission_info *per_tmp;
    int linenum = 0;
    FILE *file = fopen(PATH_PERMISSION, "r");
    if(file == NULL){
        printf("error : fail to open state profile in %s!\n", PATH_STATE);
        return -1;
    }
    while (fgets(to_deal, CHAR_MAX_LENGTH, file) != NULL){
        linenum ++;
        if (check_length_and_modify(1) == -1){
            fclose(file);
            return -1;
        }
        if (to_deal[0] == '#')
            continue;

        int index = 0;
        while (to_deal[index] != '\0'){
            if (index >= 30){
                printf("error : Permission name should not longer than 30 characters in .permission line %d \n", linenum);
                return -1;
            }
            if (to_deal[index] != '_' && (to_deal[index] < 48 || (to_deal[index] > 57 && to_deal[index] < 65) ||  to_deal[index] > 90)){
                printf("error : wrong Permission name in .permission line %d \n", linenum);
                return -1;
            }
            index ++;
        }
        per_tmp = (struct permission_info *) malloc(sizeof(struct permission_info));

        per_tmp->permission_name = (char *)malloc(sizeof(char) * (index + 1));
        strncpy(per_tmp->permission_name, to_deal, index + 1);
        per_tmp->permission_num = permission_num++;
        per_tmp->rule_head = (struct rule_info*)malloc(sizeof(struct rule_info));
        per_tmp->rule_head->next = NULL;

        per_tmp->next = per_head->next;
        per_head->next = per_tmp;
    }
    fclose(file);
    printf("%d permissions loaded \n", permission_num);
    return 0;
}

int read_state(){
    FILE *file;
    int line_num;
    // *** 读取state定义，将所有的state存表
    line_num = 0;
    file = fopen(PATH_STATE, "r");
    if(file == NULL){
        printf("error : fail to open state profile in %s!\n", PATH_STATE);
        return -1;
    }
    while (fgets(to_deal, CHAR_MAX_LENGTH, file) != NULL){
        if (check_length_and_modify(3) == -1){
            fclose(file);
            return -1;
        }
        if(read_single_state(++ line_num) == -1){
            printf("error : fail to load state profile in %s!\n", PATH_STATE);
            return -1;
        }
    }
    fclose(file);
    printf("%d states loaded \n", state_num);
    return 0;
}

int read_permission_rule(){
    FILE *file;
    int line_num;
    // *** 读取permission-rule表，将Permission与rule的关系存起来
    line_num = 0;
    if(permission_name_init() == -1){
        return -1;
    }
    file = fopen(PATH_PERMISSION_RULE, "r");
    if(file == NULL){
        printf("error : fail to open state profile in %s!\n", PATH_PERMISSION_RULE);
        return -1;
    }
    while (fgets(to_deal, CHAR_MAX_LENGTH, file) != NULL){
        if (check_length_and_modify(2) == -1){
            fclose(file);
            return -1;
        }
        if(read_single_permission_rule(++ line_num) == -1){
            fclose(file);
            printf("error : fail to load permission_rule profile in %s!\n", PATH_PERMISSION_RULE);
            return -1;
        }
    }
    fclose(file);
    printf("succeed in load permission_rule profile \n");
    return 0;
}

int read_state_permission(){
    FILE *file;
    int line_num;
    // *** 读取state-permission表，将state-permission转换成state-rule
    line_num = 0;
    file = fopen(PATH_STATE_PERMISSION, "r");
    if(file == NULL){
        printf("error : fail to open state profile in %s!\n", PATH_STATE_PERMISSION);
        return -1;
    }
    while (fgets(to_deal, CHAR_MAX_LENGTH, file) != NULL){
        if (check_length_and_modify(4) == -1){
            fclose(file);
            return -1;
        }    
        if(read_single_state_permission(++ line_num) == -1){
            fclose(file);
            printf("error : fail to load state_permission profile in %s!\n", PATH_STATE_PERMISSION);
            return -1;
        }
    }
    fclose(file);
    return 0;
}

long b2i(const char *ps, int size) {
    /**
     * 二进制转数值
     */
    long n = 0;
    for (int i = 0; i < size; ++i) {
        n = n * 2 + (ps[i] - '0');
    }
    return n;
}
int keyword2i(const char *keyword){
    if (strcmp(keyword, "deny") == 0)
        return 0;
    if (strcmp(keyword, "allow") == 0)
        return 1;
    if (strcmp(keyword, "audit") == 0)
        return 2;
}
int limit2i(const char *limit){
    int len = strlen(limit), result = 0;
    for (int i = 0; i < len; ++i){
        switch (limit[i]) {
            case 'r':
                result += 4;
                break;
            case 'w':
                result += 2;
                break;
            case 'x':
                result += 1;
                break;
        }
    }
    return result;
}

void print_state_rule(){
    struct sub_state *sub;
    struct rule_info *rule;
    per_edit_now = per_head->next;
    int allow;
    while (per_edit_now != NULL){
        printf("\n** Permission: %s \n", per_edit_now->permission_name);
        sub = sub_state_head->next;
        while (sub != NULL){
            allow = sub->father->permissions[per_edit_now->permission_num];
            if(allow){
                rule = per_edit_now->rule_head->next;
                while (rule != NULL){
                    if(strcmp(rule->object_type, "file") == 0){
                        printf("state: %s; %s; %s; %s; %s; \n", sub->sub_num, rule->keyword, rule->object_type, rule->file_path, rule->file_flags);
                    }else{
                        printf("state: %s; %s; %s; %d; \n", sub->sub_num, rule->keyword, rule->object_type, rule->cap_num);
                    }
                    rule = rule->next;
                }
            }
            sub = sub->next;
        }
        per_edit_now = per_edit_now->next;
    }
}

void generate_result(){
    remove("result");
    FILE *result = fopen( "result", "w" );
    struct sub_state *sub;
    struct rule_info *rule;
    per_edit_now = per_head->next;
    int allow;
    while (per_edit_now != NULL){
        sub = sub_state_head->next;
        while (sub != NULL){
            allow = sub->father->permissions[per_edit_now->permission_num];
            if(allow){
                rule = per_edit_now->rule_head->next;
                while (rule != NULL){
                    if(strcmp(rule->object_type, "file") == 0){
                        /* state（整数） level（整数） keyword（整数，0、1、2代表deny、allow、audit） 1（整数，代表“file”） path（字符串） limit（整数） */
                        sprintf(to_deal, "%ld 0 %d 1 %s %d \n", b2i(sub->sub_num, ATOMIC_NUM), keyword2i(rule->keyword), rule->file_path,
                               limit2i(rule->file_flags));
                        fputs(to_deal, result);
                    }else{
                        /* state（整数） level（整数） keyword（整数，0、1、2代表deny、allow、audit） 0（代表“cap”） capnum（整数） */
                        sprintf(to_deal, "%ld 0 %d 0 %d \n", b2i(sub->sub_num, ATOMIC_NUM), keyword2i(rule->keyword), rule->cap_num);
                        fputs(to_deal, result);
                    } 
                    rule = rule->next;
                }
            }
            sub = sub->next;
        }
        per_edit_now = per_edit_now->next;
    }
    fclose(result);
}

int main(){
    init();

    // 读取策略文件
    if (read_permission_rule() == -1)
        exit(0);
    if (read_state() == -1)
        exit(0);
    if (read_state_permission() == -1)
        exit(0);

    // 检查冲突
    check_intersections();
    // 拆分State中的 '*'
    state_split();
    // 生成最终策略文件
    generate_result();
    // 打印传入内核的state-rule
    print_state_rule();

    // 释放所有使用过的内存
    free_all_mem();
    return 0;
}
