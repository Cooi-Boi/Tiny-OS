#include "buildin_cmd.h"
#include "../fs/file.h"
#include "../fs/fs.h"
#include "debug.h"
#include "string.h"
#include "syscall.h"
#include "stdio-kernel.h"
#include "../fs/dir.h"
#include "../fs/fs.h"
#include "stdio.h"
#include "shell.h"

extern char final_path[160];

//在用户态就把路径给解析出来
void wash_path(char* old_abs_path,char* new_abs_path)
{
    ASSERT(old_abs_path[0] == '/');
    char name[MAX_FILE_NAME_LEN] = {0};
    char* sub_path = old_abs_path;
    sub_path = path_parse(sub_path,name);
    
    //如果直接就是根目录 直接返回即可
    if(name[0] == 0)
    {
        new_abs_path[0] = '/';
        new_abs_path[1] = 0;
        return;
    }
    new_abs_path[0] = 0; 
    strcat(new_abs_path,"/");
    while(name[0])
    {
        if(!strcmp("..",name))   //返回上级
        {
            char* slash_ptr = strrchr(new_abs_path,'/'); //等于移动到最偏右的/位置去
            if(slash_ptr != new_abs_path)   //如果为 /aaa 那么移动之后就到/的位置了 如果是/aaa/bbb 那么就会回到/aaa/
                *slash_ptr = 0; // 把/变成0
            else
	        *(slash_ptr+1) = 0; // 如果是 / 或者 /aaa 那么都回到/ 则把最右边+1置零位即可
        }
        else if(strcmp(".",name))  //如果不是到. 增加到后米纳即可 .等于没有作用 继续遍历即可
        {
            if(strcmp(new_abs_path,"/"))  //不是 / 防止出现// 的情况
                strcat(new_abs_path,"/");
            strcat(new_abs_path,name);
        }
        
        memset(name,0,MAX_FILE_NAME_LEN);
        if(sub_path)
            sub_path = path_parse(sub_path,name);
    }
}

//把path处理 . ..去掉 储存在final_path getcwd得到当前工作目录 + 相对路径 即绝对路径
void make_clear_abs_path(char* path,char* final_path)
{
    char abs_path[MAX_PATH_LEN] = {0};
    if(path[0] != '/')  //如果不是绝对路径就弄成绝对路径
    {
        memset(abs_path,0,MAX_PATH_LEN);
        if(getcwd(abs_path,MAX_PATH_LEN) != NULL)
        {
            if(!((abs_path[0] == '/') && abs_path[1] == 0))
                strcat(abs_path,"/");
        }
    }
    //把path 加到工作目录的头上
    strcat(abs_path,path);
    wash_path(abs_path,final_path);
}

// pwd命令中的内建函数 得到当前工作目录
void buildin_pwd(uint32_t argc,char** argv)
{
    //没有参数才可以
    if(argc != 1)
        printf("pwd: no argument support!\n");
    else
    {
        if(getcwd(final_path,MAX_PATH_LEN) != NULL)
            printf("%s\n",final_path);
        else
            printf("pwd: get current work directory failed\n");
    }   
}

// 支持一个参数 改变当前工作目录
char* buildin_cd(uint32_t argc,char** argv)
{
    if(argc > 2)
    {
        printf("cd: only support 1 argument!\n");
        return NULL;
    }
    if(argc == 1)
    {
        final_path[0] = '/';
        final_path[1] = 0;
    }
    else    make_clear_abs_path(argv[1],final_path);
    
    if(chdir(final_path) == -1)
    {
        printf("cd: no such directory %s\n",final_path);
        return NULL;
    }
    return final_path;   
}

// ls内建函数 仅支持-l -h -h等于不支持 哈哈
void buildin_ls(uint32_t argc,char** argv)
{
    char* pathname = NULL;
    struct stat file_stat;
    memset(&file_stat,0,sizeof(struct stat));
    bool long_info = false;
    uint32_t arg_path_nr = 0;  
    uint32_t arg_idx = 1;    //第一个字符串是 ls 跳过
    while(arg_idx < argc)    //仅仅支持 ls 或者 ls -l 或者 ls -l path的形式
    {
        if(argv[arg_idx][0] == '-')
        {
            if(!strcmp("-l",argv[arg_idx]))
                long_info = true;
            else if(!strcmp("-h",argv[arg_idx]))
            {
                printf("usage: -l list all all information about the file.\nnot support -h now sry - -\n");
                return;
            }
            else
            {
                printf("ls: invaild option %s\nTry 'ls -l' u can get what u want\n",argv[arg_idx]);
                return; 
            }
        }
        else
        {
            if(arg_path_nr == 0)
            {
                pathname = argv[arg_idx];
                arg_path_nr = 1;
            }
            else
            {
                printf("ls: only support one path\n");
                return;
            }
        }
        ++arg_idx;
    }
    
    if(pathname == NULL) //ls 或者 ls -l
    {
        //得到工作目录
        if(getcwd(final_path,MAX_PATH_LEN) != NULL)
	    pathname = final_path;
	else
	{
	    printf("ls: getcwd for default path failed\n");
	    return;
	}
    }
    else
    {
        make_clear_abs_path(pathname,final_path);
        pathname = final_path;
    }
    
    //目录下的文件
    if(stat(pathname,&file_stat) == -1)
    {
        printf("ls: cannot access %s: No such file or directory\n",pathname);
        return;
    }
    
    if(file_stat.st_filetype == FT_DIRECTORY)  //得到目录文件才继续
    {
        struct dir* dir = opendir(pathname); //得到目录指针
        struct dir_entry* dir_e = NULL;
        char sub_pathname[MAX_PATH_LEN] = {0};
        uint32_t pathname_len   = strlen(pathname);
        uint32_t last_char_idx  = pathname_len - 1;
        memcpy(sub_pathname,pathname,pathname_len);
        
        //方便后面得到当前目录下的文件stat信息 
        //加个/ 之后每个文件加文件名stat即可
        if(sub_pathname[last_char_idx] != '/') 
        {
            sub_pathname[pathname_len] = '/'; 
            ++pathname_len;
        }
        
        rewinddir(dir);  //目录指针指向0  方便readdir遍历目录项
        if(long_info)    // ls -l 这里是目录的ls
        {
            char ftype;
            printf("total: %d\n",file_stat.st_size);
            while((dir_e = readdir(dir)))    //通过readdir来遍历目录项 我还专门回去看了看这个函数
            {
                ftype = 'd';
                if(dir_e->f_type == FT_REGULAR)
                    ftype = '-';
                sub_pathname[pathname_len] = 0; //把字符串末尾设0 方便strcat函数
                strcat(sub_pathname,dir_e->filename);
                memset(&file_stat,0,sizeof(struct stat));
                if(stat(sub_pathname,&file_stat) == -1)
                {
                    printf("ls: cannot access %s:No such file or directory\n",dir_e->filename);
                    return;
                }
                printf("%c    %d    %d    %s\n",ftype,dir_e->i_no,file_stat.st_size,dir_e->filename);
            }
        }
        else  //仅仅是ls 把文件名写出来即可
        {
            while((dir_e = readdir(dir)))
                printf("%s  ",dir_e->filename);
            printf("\n");
        }
        closedir(dir);
    }
    else
    {
        if(long_info)
             printf("-    %d    %d    %s\n",file_stat.st_ino,file_stat.st_size,pathname);
	else
	    printf("%s\n",pathname);
    }
}

void buildin_ps(uint32_t argc,char** argv)
{
     //不应该有参数
    if(argc != 1)
    {
        printf("ps: no argument support!\n");
        return;
    }
    ps();
}

void buildin_clear(uint32_t argc,char** argv)
{
    if(argc != 1)
    {
        printf("clear: no argument support!\n");
        return;
    }
    clear();
}

int32_t buildin_mkdir(uint32_t argc,char** argv)
{
    //必须要有一个 安装路径参数
    int32_t ret = -1;
    if(argc != 2)
        printf("mkdir: only support 1 argument!\n");
    else
    {
        make_clear_abs_path(argv[1],final_path);
        if(strcmp("/",final_path))  //不是根目录 根目录一直都在
        {
            if(mkdir(final_path) == 0)
                ret = 0;
            else
                printf("mkdir: create directory %s failed.\n",argv[1]);
        }
    }
    return ret;
}

int32_t buildin_rmdir(uint32_t argc,char** argv)
{
    int32_t ret = -1;
    if(argc != 2)
        printf("rmdir: only support 1 argument!\n");
    else
    {
        make_clear_abs_path(argv[1],final_path);
        if(strcmp("/",final_path)) // 不能删除根目录
        {
            if(rmdir(final_path) == 0)
                ret = 0;
            else    printf("rmdir: remove %s failed\n");
        }
    }
    return ret;
}

int32_t buildin_rm(uint32_t argc,char** argv)
{
    int32_t ret = -1;
    if(argc != 2)
        printf("rm: only support 1 argument!\n");
    else
    {
        make_clear_abs_path(argv[1],final_path);
        if(strcmp("/",final_path))
        {
            if(unlink(final_path) == 0)
                ret = 0;
            else    printf("rm: delete %s failed\n",argv[1]); 
        }
    }
    return ret;
}
