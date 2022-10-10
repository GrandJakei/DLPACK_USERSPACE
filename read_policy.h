// 保存state名称及编号的文件
#define PATH_STATE "./policy/states"
//#define PATH_STATE "e:\\states.txt"

// 保存state_permission的文件
#define PATH_STATE_PERMISSION  "./policy/state_per"
//#define PATH_STATE_PERMISSION  "e:\\state_per.txt"

// 保存permission_rule的文件
#define PATH_PERMISSION_RULE "./policy/per_rule"
//#define PATH_PERMISSION_RULE "e:\\per_rule.txt"

// 保存所有permission名称的文件
#define PATH_PERMISSION  "./policy/.permission"
//#define PATH_PERMISSION  "e:\\permission.txt"

#define CHAR_MAX_LENGTH 1024        // 一行字符串最多允许的字符数

#define DL_MAX_RULE_NUM 256         // 策略允许下，每个state下允许最多的rule数量


struct rule_info{
    char *keyword;      // rule的关键词
    char *object_type;  // 客体类型，"file" 或者 "cap"
    int cap_num;
//    char *cap_num;      // cap的编号
    char *file_path;    // 文件路径
    char *file_flags;   // 涉及到的 文件操作
    struct rule_info *next;
};

struct permission_info{
    char *permission_name;            // permission名称
    int permission_num;
    struct rule_info *rule_head;       // 这个permission所包含的rules
    struct permission_info *next;
};

struct state_info{
    char *state_name;   // state名称
    char *state_num;      // state编号
    int *permissions;
    struct state_info *next;
};

struct sub_state{
    struct state_info *father;      // 父state编号
    char *sub_num;                  // 拆分后的state编号
    struct sub_state *next;
};
