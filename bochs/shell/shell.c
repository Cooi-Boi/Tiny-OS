#include "shell.h"
#include "global.h"
#include "stdint.h"
#include "string.h"
#include "syscall.h"
#include "stdio.h"
#include "../fs/file.h"
#include "../kernel/debug.h"
#include "buildin_cmd.h"
#include "../fs/fs.h"
#include "../userprog/exec.h"

#define cmd_len    128 //最大支持128个字符
#define MAX_ARG_NR 16  //命令名外支持15个参数

char cmd_line[cmd_len] = {0};
char cwd_cache[64] = {0}; //目录的缓存 执行cd则移动到其他目录去
char final_path[MAX_PATH_LEN];
char* argv[MAX_ARG_NR];   //参数

//固定输出提示副
void print_prompt(void)
{
    printf("[Love 6@localhost %s]$ ",cwd_cache);
}

//最多读入count字节到buf
void readline(char* buf,int32_t count)
{
    ASSERT(buf != NULL && count > 0);
    char* pos = buf;
    //默认没有到回车就不停止 、一个一个字节读
    while(read(stdin_no,pos,1) != -1 && (pos - buf) < count)
    {
        switch(*pos)
        {
            //清屏
            case 'l'-'a':
                *pos = 0;
                clear();
                print_prompt();
                printf("%s",buf);  //把刚刚键入的字符打印出开
                break;
            
            //清除输入
            case 'u'-'a':
                while(buf != pos)
                {
                    putchar('\b');
                    *(pos--) = 0;
                }
                break;
                
            //和下面的回车一起
            case '\n':
            case '\r':
                *pos = 0;
                putchar('\n');
                return;
            
            case '\b':
                if(buf[0] != '\b') //阻止删除不是本次输出的信息
                {
                    --pos;
                    putchar('\b');
                }
                break;
            
            default:
                putchar(*pos);
                ++pos;
        }
    }
    printf("readline: cant fine entry_key in the cmd_line,max num of char is 128\n");
}

//解析键入的字符 token为分割符号
int32_t cmd_parse(char* cmd_str,char** argv,char token)
{
    ASSERT(cmd_str != NULL);
    int32_t arg_idx = 0;
    
    //初始化指针数组
    while(arg_idx < MAX_ARG_NR)
    {
        argv[arg_idx] = NULL;
        arg_idx++;
    }
    char* next = cmd_str;
    int32_t argc = 0;
    while(*next)
    {
        while(*next == token)	++next;
        if(*next == 0)	break;	//如果最后一个参数后面是空格 则之后就是结束符号了
        argv[argc] = next;     //无论怎么样 到这里都是有字符的地方 且argc表示目前存在第几个参数字符串
        
        while(*next && *next != token)  //要不结束了为0 或者到了分割符号
            ++next;
            
        if(*next)
            *(next++) = 0; //到最后就是设置字符串的末尾0 分割符号位置刚好处理为0 这样字符串有结束末尾了
                        //最后一个参数后面自然有'\0'
        if(argc > MAX_ARG_NR)
            return -1;
        
        ++argc;        
    }
    return argc;        //多少个参数
}

void my_shell(void)
{
    cwd_cache[0] = '/';
    cwd_cache[1] = 0;
    int argc = -1;
    while(1)
    {
        print_prompt();
        memset(cmd_line,0,cmd_len);
        memset(final_path,0,MAX_PATH_LEN);
        memset(argv,0,sizeof(char*) * MAX_ARG_NR);
        readline(cmd_line,cmd_len);
        if(cmd_line[0] == 0)
            continue;
            
        argc = -1;  
        argc = cmd_parse(cmd_line,argv,' ');
        if(argc == -1)
        {
            printf("num of arguments exceed %d\n",MAX_ARG_NR);
            continue;
        }
        if(!strcmp("ls",argv[0]))
            buildin_ls(argc,argv);
        else if(!strcmp("pwd",argv[0]))
            buildin_pwd(argc,argv);
        else if(!strcmp("ps",argv[0]))
            buildin_ps(argc,argv);
        else if(!strcmp("cd",argv[0]))
        {
            if(buildin_cd(argc,argv) != NULL)
            {
                memset(cwd_cache,0,MAX_PATH_LEN);
                strcpy(cwd_cache,final_path);
            }
        }
        else if(!strcmp("clear",argv[0]))
            buildin_clear(argc,argv);
        else if(!strcmp("mkdir",argv[0]))
            buildin_mkdir(argc,argv);
        else if(!strcmp("rmdir",argv[0]))
            buildin_rmdir(argc,argv);   
        else if(!strcmp("rm",argv[0]))
            buildin_rm(argc,argv); 
        else
        {
            printf("external command\n");
        }
    }
    PANIC("my_shell: should not be here");
}
