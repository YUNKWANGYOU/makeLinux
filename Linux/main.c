#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#define MAX_ARGV 256
#define MAX_CMD 64
#define MAX_NAME 24
#define MAX_PASSWD 24
#define MAX_DIR 64
typedef struct rtime{
    int year,month,day,hour,min,sec;
}rtime;
typedef struct fnode *fptr;
typedef struct fnode{
    int id; // node id(address value) (ex:1382842368)
    char permission[9]; // permission (ex:rw-rwxrwx)
    bool hide; // Whether it's a hidden file or not.
    char type; // 'd' is directory. '-' is file.
    char owner[MAX_NAME]; // File or directory owner
    char group[MAX_NAME]; // File or directory ownership group
    char name[MAX_NAME]; // Dir/file name
    rtime recent_time;
    fptr upper,lower,sbling;
}fnode;
typedef struct group *gptr;
typedef struct group{
    int gid;
    char name[MAX_NAME];
}group;
typedef struct user *uptr;
typedef struct user{
    int uid,gid;
    fptr user_root;
    char name[MAX_NAME],passwd[MAX_PASSWD],home_path[MAX_DIR];
    rtime recent_time;
    uptr link;
}user;
typedef struct qnode *qptr;
typedef struct qnode{
    fptr data;
    qptr link;
}qnode;
typedef struct qtype{
    qptr front, rear;
}qtype;
typedef struct snode *sptr;
typedef struct snode{
    fptr data;
    sptr link;
}snode;
typedef struct stack{
    sptr top;
}stacktype;
fptr make_fd(char type,char *fdname,uptr cur_user,fptr *upper,int mode);
fptr get_fd(fptr *mydir, uptr cur_user, char type, char *newname, int mode);
fptr change_directory(fptr *cur,fptr *_cur,uptr cur_user,char dirname[],char mode[]);
int check_arg(char *argv[], int max);
void get_permission(fptr *cur,int mode);
fptr chdir(fptr *curr,uptr cur_user,char *path[],char mode[]);
fptr root=NULL; uptr user_head=NULL;
char current_ver[]="0.1 (Test)", os_name[]="DGU-OS";


int check_permission(fptr cur,uptr cur_user,char access_permission)
{
    if(cur_user->uid==0&&cur_user->gid==0) return 1;
    if(access_permission=='r'){ // 접근하려는 권한이 읽기 권한인 경우(ex:cat(read only), vi(read only)...)
        if(!strcmp(cur->owner,cur_user->name)){ // 대상이 유저 소유라면 rwxrwxrwx 중 제일 앞의 3자리를 본다.
            if(cur->permission[0]=='r') return 1;
            else return 0;
        }
        else{ // 유저 소유의 폴더가 아니므로 rwxrwxrwx 중 맨 뒤 3자리를 본다.
            if(cur->permission[6]=='r') return 1;
            else return 0;
        }
    }
    else if(access_permission=='w'){ // 접근하려는 권한이 쓰기 권한인 경우(ex:mkdir, cat(read and write), chmod ...)
        if(!strcmp(cur->owner,cur_user->name)){ // 대상이 유저 소유라면 rwxrwxrwx 중 제일 앞의 3자리를 본다.
            if(cur->permission[1]=='w') return 1;
            else return 0;
        }
        else{ // 유저 소유의 폴더가 아니므로 rwxrwxrwx 중 맨 뒤 3자리를 본다.
            if(cur->permission[7]=='w') return 1;
            else return 0;
        }
    }
    else if(access_permission=='x'){ // 접근하려는 권한이 접근 권한인 경우(ex:ls, cd)
        if(!strcmp(cur->owner,cur_user->name)){ // 대상이 유저 소유라면 rwxrwxrwx 중 제일 앞의 3자리를 본다.
            if(cur->permission[3]=='x') return 1;
            else return 0;
        }
        else{ // 유저 소유의 폴더가 아니므로 rwxrwxrwx 중 맨 뒤 3자리를 본다.
            if(cur->permission[8]=='x') return 1;
            else return 0;
        }
    }
    else return 0;
}

int get_childsize(fptr t)
{
    int size=0;
    if (t!=NULL){
        size+=sizeof(*t);
        size+=get_childsize(t->lower);
        size+=get_childsize(t->sbling);
    }
    return size;
}

int get_filesize(fptr t)
{
    return sizeof(*t)+get_childsize(t->lower); // I've set the file size to the size of the structure. need to review it.
}

char* get_month(int m)
{
    static char monthlist[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    return monthlist[m];
}

stacktype make_stack()
{
    stacktype stack;
    stack.top=NULL;
    return stack;
}

void push(stacktype *stack,fptr item)
{
    sptr newp=(sptr)malloc(sizeof(snode));
    newp->data=item; newp->link=NULL;
    newp->link=(*stack).top;
    (*stack).top=newp;
}

fptr pop(stacktype *stack)
{
    if((*stack).top==NULL) return NULL;
    sptr delete_node=(*stack).top;
    fptr return_val=delete_node->data;
    (*stack).top=(*stack).top->link;
    free(delete_node);
    return return_val;
}

void free_stack(stacktype *stack)
{
    while(pop(stack)!=NULL){

    }
}

qtype make_queue()
{
    qtype queue;
    queue.front=queue.rear=NULL;
    return queue;
}

int optchk(char remain[],char funcname[])
{
    int i=0;
    while(remain[i]!='\0'){
        if(remain[i]=='-'){
            while(remain[i]=='-'){
                i++;
            }
            printf("bash: %s: -%c invalid option\n",funcname,remain[i]);
            return 0;
        } i++;
    }
    return 1;
}

void pwd(fptr curtemp,char remain[])
{
    if(!optchk(remain,"pwd")) return;
    fptr t=curtemp;
    stacktype stack=make_stack();
    if(t->upper==t){
        printf("/\n"); return;
    }
    while(t!=root){
        push(&stack,t);
        t=t->upper;
    }
    push(&stack,t); // It's means push(root)
    while((t=pop(&stack))!=NULL){
        if(t==root) printf("/");
        else if(t==curtemp) printf("%s",t->name);
        else printf("%s/",t->name);
    }
    printf("\n");
}

void insert_queue(qtype *queue,fptr item)
{
    qptr newp=(qptr)malloc(sizeof(qnode));
    newp->data=item; newp->link=NULL;
    if((*queue).front==NULL){
        (*queue).front=(*queue).rear=newp;
    }
    else{
        (*queue).rear->link=newp;
        (*queue).rear=newp;
    }
}

fptr delete_queue(qtype *queue)
{
    if((*queue).front==NULL){
        (*queue).rear=NULL;
        return NULL;
    }
    qptr delete_node=(*queue).front;
    fptr return_val=delete_node->data;
    (*queue).front=(*queue).front->link;
    free(delete_node);
    return return_val;
}

void free_queue(qtype *queue)
{
    while((*queue).front!=NULL){
        delete_queue(queue);
    }
}

fptr ls_getcur(fptr cur,uptr cur_user,char path[])
{
    fptr t=cur;
    if(path==NULL) return t;
    if(!strncmp(path,"/",1)) t=root;
    char *token=strtok(path,"/");
    while(token!=NULL){
        if(!strncmp(token,"~",1)) t=root;
        else if(!strncmp(token,".",1)) t=cur;
        else t=change_directory(&cur,&t,cur_user,token,"ls");
        token=strtok(NULL,"/");
    }
    return t;
}

int ls_getlinknum(fptr cur)
{
    int num=2;
    fptr t=cur->lower;
    while(t!=NULL){
        num++;
        t=t->sbling;
    }
    return num;
}

void ls(fptr cur,uptr cur_user,char argv[])
{
    char *path=NULL;
    int a=0, l=0, i;
    char *token=strtok(argv," ");
    if(strlen(argv)!=0)
    {
        while(token!=NULL){
            if (token[0]=='-'){
                i=0;
                while(token[i]=='-'){
                    i++;
                }
                while(token[i]!='\0')
                {
                    if(token[i]=='a')
                        a=1;
                    else if(token[i]=='l')
                        l=1;
                    else{
                        printf("ls: invalid option -- \'%c\'\n",token[i]);
                        return;
                    }
                    i++;
                }
            }
            else{
                path=(char*)malloc(sizeof(char)*strlen(token));
                strcpy(path,token);
            }
            token=strtok(NULL," ");
        }
    }
    qtype queue=make_queue();
    fptr curtemp=ls_getcur(cur,cur_user,path);
    if(!check_permission(curtemp,cur_user,'x')){
        printf("ls: cannot open directory \'%s\': Permission denied\n",curtemp->name);
    }
    fptr t=curtemp->lower;
    if(a==1){
        insert_queue(&queue,curtemp);
        insert_queue(&queue,curtemp->upper);
    }
    while(t!=NULL){
        //kprintf("insert q:%s\s",t->name);
        if(a==1)
            insert_queue(&queue,t);
        else if(a==0&&(t->hide==false))
            insert_queue(&queue,t);
        t=t->sbling;
    }
    t=delete_queue(&queue); i=0;
    char list_string[]="%c%s%4d %5s%5s  %5d %5s %d %d:%d %s\n";
    while(t!=NULL){
        if(l==1){
            if(i==0&&t==curtemp) printf(list_string,t->type,t->permission,ls_getlinknum(t),t->owner,t->group,get_filesize(t),get_month(t->recent_time.month),t->recent_time.day,t->recent_time.hour,t->recent_time.min,".");
            else if(i==1&&t==curtemp->upper) printf(list_string,t->type,t->permission,ls_getlinknum(t),t->owner,t->group,get_filesize(t),get_month(t->recent_time.month),t->recent_time.day,t->recent_time.hour,t->recent_time.min,"..");
            else printf(list_string,t->type,t->permission,ls_getlinknum(t),t->owner,t->group,get_filesize(t),get_month(t->recent_time.month),t->recent_time.day,t->recent_time.hour,t->recent_time.min,t->name);
        }
        else{
            if(i==0&&t==curtemp) printf(".   ");
            else if(i==1&&t==curtemp->upper) printf("..  ");
            else printf("%8s",t->name);
        } i++;
        t=delete_queue(&queue);
    }
    if(l==0) printf("\n");
    free_queue(&queue);
    free(path);
}

rtime make_fd_refresh_time()
{
    rtime recent_time;
    time_t make_time=time(NULL);
    struct tm *t=localtime(&make_time);
    recent_time.year=t->tm_year;
    recent_time.month=t->tm_mon;
    recent_time.day=t->tm_mday;
    recent_time.hour=t->tm_hour;
    recent_time.min=t->tm_min;
    recent_time.sec=t->tm_sec;
    return recent_time;
}

fptr make_fd(char type,char *fdname,uptr cur_user,fptr *upper,int mode)
{
    fptr fd=(fptr)malloc(sizeof(fnode));
    strcpy(fd->name,fdname); fd->type=type;
    fd->lower=fd->sbling=NULL; fd->id=fd; fd->hide=false;
    get_permission(&fd,mode);
    strcpy(fd->owner,cur_user->name); // User 기능이 없으므로 일단 root로 소유권 둠.
    strcpy(fd->group,cur_user->name);
    fd->recent_time=make_fd_refresh_time();
    if (*upper==NULL){
        fd->upper=fd;
    }
    else fd->upper=*upper;
    return fd;
}

void dirsave_method(FILE *fp,fptr t)
{
    if(t!=NULL){
        fwrite(t,sizeof(fnode),1,fp);
        dirsave_method(fp,t->sbling);
        dirsave_method(fp,t->lower);
    }
}

void save_fd(char dir[])
{
    FILE *fp;
    if((fp=fopen(dir,"wb"))==NULL){
        printf("File open error!\n"); return;
    }
    dirsave_method(fp,root);
    fclose(fp);
}

int isoctal(char c[])
{
    int i=0;
    while(c[i]!='\0'){
        if(c[i]<48||c[i]>55) return 0; // 48:0, 55:7
        i++;
    }
    return 1; // all is didit!
}

void get_permission(fptr *fd,int mode) // 2 algorithm edit!
{
    int i=0,j=0,searchset[]={4,2,1},step=0,len=0;
    char mode_s[4],pset[]={'r','w','x'},temp[]="000\0";
    itoa(mode,mode_s,10);
    len=strlen(mode_s);
    if(len==1){
        temp[2]=mode_s[0];
        strcpy(mode_s,temp);
    }
    else if(len==2){
        temp[1]=mode_s[0]; temp[2]=mode_s[1];
        strcpy(mode_s,temp);
    }
    for(i=0;i<3;i++){
        int mode_i=mode_s[i]-48;
        for(j=0;j<3;j++,step++){
            if(mode_i<searchset[j]){
                (*fd)->permission[step]='-';
            }
            else{
                mode_i%=searchset[j];
                (*fd)->permission[step]=pset[j];
            }
        }
    }
    (*fd)->permission[step]='\0';
}

fptr fwddir(fptr *cur,fptr *curtemp,char upper_dir[],char lower_dir[])
{
    if(!strncmp(upper_dir,"~",1)) return root;
    else if(!strncmp(upper_dir,".",1)) return *cur;
    if((*curtemp)->lower==NULL){
        if(lower_dir==NULL) return *curtemp;
        else return NULL;
    }
    fptr t=(*curtemp)->lower;
    while(t!=NULL&&strcmp(t->name,upper_dir)){
        t=t->sbling;
    }
    if(t==NULL){ //upper dir is not exist.
        if(lower_dir==NULL) return *curtemp;
        else return NULL;
    }
    else{ // upper_dir is already exist.
        if(lower_dir==NULL) return -1;
        else return t;
    }
}

fptr chkdir(fptr *cur,uptr cur_user,char path[],int mode)
{
    fptr curtemp=*cur;
    char *token, *next_token;
    if(!strncmp(path,"/",1)) curtemp=root;
    token=strtok(path,"/");
    next_token=strtok(NULL,"/");
    while(next_token!=NULL){
        if((curtemp=fwddir(cur,&curtemp,token,next_token))==NULL){
            printf("mkdir: cannot create directory \'%s\': No such file or directory\n",token);
            return NULL;
        }
        else if(curtemp==-1){
            printf("mkdir: cannot create directory \'%s\': File or directory exists.\n",token);
            return NULL;
        }
        token=next_token;
        next_token=strtok(NULL,"/");
    }
    if(!check_permission(curtemp,cur_user,'w')){
        printf("ls: cannot open directory \'%s\': Permission denied\n",(*cur)->name);
        return NULL;
    }
    if(next_token==NULL) get_fd(&curtemp,cur_user,'d',token,mode); // token=='TT', curtemp=test2
    else get_fd(&curtemp,cur_user,'d',next_token,mode);
    return curtemp;
}

fptr fwddir_p(fptr *cur,uptr cur_user,fptr *curtemp,char dirn[],int mode)
{
    if(!strncmp(dirn,"~",1)) return root;
    else if(!strncmp(dirn,".",1)) return *cur;
    if(!check_permission(*cur,cur_user,'w')||!check_permission(*cur,cur_user,'x')){
        printf("mkdir: cannot open directory \'%s\': Permission denied\n",(*cur)->name);
        return NULL;
    }
    if((*curtemp)->lower==NULL){
        return get_fd(curtemp,cur_user,'d',dirn,mode);
    }
    fptr t=(*curtemp)->lower;
    while(t!=NULL&&strcmp(t->name,dirn)){
        t=t->sbling;
    }
    if(t==NULL){
        return get_fd(curtemp,cur_user,'d',dirn,mode);
    }
    else return t;
}

void chkdir_p(fptr *cur,uptr cur_user,char path[],int mode)
{
    fptr curtemp=*cur;
    char *token;
    if(!strncmp(path,"/",1)) curtemp=root;
    token=strtok(path,"/");
    while(token!=NULL){
        curtemp=fwddir_p(cur,cur_user,&curtemp,token,mode);
        token=strtok(NULL,"/");
    }
}

void mkdir(fptr *cur,uptr cur_user,char *argv[])
{
    int pathnum=0, p=0, m=-1, i, mode=-1;
    char **path_list;
    char *token=strtok(argv," ");
    while(token!=NULL){
        if (token[0]=='-'){
            i=0;//-m --p option processing
            while(token[i]=='-'){
                i++;
            } // token[i] == option character
            while(token[i]!='\0'){
                if(token[i]=='m')
                    m=1; // option m is on (Need a [mode] such as 700)
                else if(token[i]=='p')
                    p=1; // option p is on
                else{
                    printf("mkdir: invalid option -- \'%c\'\n",token[i]);
                    return;
                }
                i++;
            }
        }
        else if(m==1&&mode==-1){
            //printf("m==1\n");
            if(strlen(token)>3||!isoctal(token)){
                printf("mkdir: invalid mode -- \'%s\'\n",token);
                return;
            }
            mode=atoi(token);
        }
        else{
            if(pathnum++==0)
                path_list=(char**)malloc(sizeof(char*));
            else
                realloc(path_list,sizeof(char*)*pathnum);
            path_list[pathnum-1]=(char*)malloc(sizeof(char)*strlen(token));
            strcpy(path_list[pathnum-1],token);
            // path processing
        }
        token=strtok(NULL," ");
    } // end of while
    if(m==1){
        if(mode==-1){
            printf("mkdir: option requires an argument -- 'm'\n");
            return;
        }
    }
    //for(i=0;i<pathnum;i++)
      //  printf("path:%s\n",path_list[i]);
    if(m<=-1) mode=755; // default is 755
    // Directory Creation part
    if(p==0){
        for(i=0;i<pathnum;i++){
            if(chkdir(cur,cur_user,path_list[i],mode)==NULL) // Need the fork() p==0 case
                return NULL;
        }
    }
    else if(p==1){ // sequence generate part
        for(i=0;i<pathnum;i++){
            chkdir_p(cur,cur_user,path_list[i],mode); // Need the fork() p==1 case
        }
    }
    for(i=0;i<pathnum;i++)
        free(path_list[i]);
    free(path_list);
}

fptr get_fd(fptr *mydir, uptr cur_user, char type, char *newname, int mode)
{
    if (*mydir==NULL){
        *mydir=make_fd('d',"root",cur_user,mydir,mode);
        root=*mydir;
        return *mydir;
    }
    else{
        fptr temp, fd=make_fd(type,newname,cur_user,mydir,mode);
        if((*mydir)->lower==NULL)
            return (*mydir)->lower=fd;
        temp=(*mydir)->lower;
        while(temp->sbling!=NULL){
            temp=temp->sbling;
        }
        return temp->sbling=fd;
    }
} // mkdir -p /abc/cde/fgd <->mkdir abc dddd tmp/test

void id_reset(fptr *t)
{
    if(*t!=NULL){
        (*t)->id=*t;
        id_reset(&((*t)->sbling));
        id_reset(&((*t)->lower));
    }
}

void dirget_relation(fptr *curr,fptr *n)
{
    if((*curr)!=NULL){
        if((*curr)->id==(*n)->upper&&(*curr)->type=='d'){
            fptr t=(*curr)->lower;
            while(t!=NULL&&t->sbling!=NULL){
                t=t->sbling;
            }
            if(t==NULL){
                (*curr)->lower=*n;
            }
            else t->sbling=*n;
            (*n)->upper=*curr; // Refresh parent
        }
        else{
            dirget_relation(&((*curr)->sbling),n);
            dirget_relation(&((*curr)->lower),n);
        }
    }
}

void dirload_method(fptr *curr,FILE *fp)
{
    while(!feof(fp)){
        fptr n=(fptr)malloc(sizeof(fnode));
        fread(n,sizeof(fnode),1,fp);
        n->lower=NULL; n->sbling=NULL;
        if(!strlen(n->name)){
            free(n);
            return;
        }
        else if(root==NULL){
            root=*curr=n;
            n->upper=root;
        }
        else{
            dirget_relation(curr,&n);
        }
    }
}

void load_mydir(fptr *curr,char *dir)
{
    FILE *fp;
    if ((fp=fopen(dir,"rb"))==NULL){
        printf("Directory loading errer!\n");
        return NULL;
    }
    dirload_method(curr,fp);
    id_reset(curr);
    fclose(fp);
}

fptr chdir(fptr *curr,uptr cur_user,char *path[],char mode[])
{
    fptr curtemp=*curr;
    char *s=(char*)malloc(strlen(path));strcpy(s,path);
    if(!strncmp(path,"/",1)){
        if((curtemp=change_directory(curr,&curtemp,cur_user,"/",mode))==NULL)
            return NULL;
    }
    char *dir=strtok(s,"/");
    while(dir!=NULL)
    {
        if((curtemp=change_directory(curr,&curtemp,cur_user,dir,mode))==NULL){
            return NULL;
        }
        dir=strtok(NULL,"/");
    }
    free(s);
    return curtemp;
}

int check_arg(char *argv[], int max)
{
    int count=0;
    char *t=strtok(argv," ");
    while(t!=NULL){
        count++;
        t=strtok(NULL," ");
    }
    if(count>max) return 1;
    return 0;
}

int cd(fptr *curr,uptr cur_user,char *argv[])
{
    if(check_arg(argv,1)){
        printf("bash: cd: too many arguments\n");
        return 0;
    }
    fptr curtemp;
    if ((curtemp=chdir(curr,cur_user,argv,"cd"))!=NULL){
        if(!check_permission(curtemp,cur_user,'x')){
            printf("cd: cannot open directory \'%s\': Permission denied\n",curtemp->name);
            return 0;
        }
        *curr=curtemp;
    }
    else return 0;
    return 1;
}

fptr change_directory(fptr *cur,fptr *_cur,uptr cur_user,char dirname[],char mode[])
{
    if(!strcmp(dirname,"/")) return root; //dirname = "~/test/starcraft
    else if(!strcmp(dirname,"~")) return cur_user->user_root;
    else if(!strcmp(dirname,"..")) return (*_cur)->upper;
    else if(!strcmp(dirname,".")) return *cur;
    fptr t=(*_cur)->lower;
    while(t!=NULL&&(strcmp(t->name,dirname)||t->type!='d')){
        t=t->sbling;
    }
    if(t==NULL){
        printf("bash: %s: %s: No such file or directory\n",mode,dirname);
        return NULL;
    } // t!=NULL
    else{
        if(t->type=='d') *_cur=t;
        else{
            printf("bash: %s: %s: Not a directory\n",mode,dirname);
            return NULL;
        }
    }
    return *_cur;
}

fptr preorder(fptr t)
{
    if(t!=NULL){
        printf("%s ",t->name);
        //preorder(t->lower);
        preorder(t->sbling);
        preorder(t->lower);
    }
}

fptr freeall(fptr t)
{
    if(t!=NULL){
        freeall(t->lower);
        freeall(t->sbling);
        free(t);
    }
}

void split_cmd(char argv[],char cmd[],char remain[])
{
    int cmdlen=0,remainlen=0,i=0;
    while(argv[i]!=' '&&argv[i]!='\0'){
        cmd[cmdlen++]=argv[i++];
    }
    i++; cmd[cmdlen]='\0';
    while(argv[i]!='\0'){
        remain[remainlen++]=argv[i++];
    }
    remain[remainlen]='\0';
    for(i=0;i<MAX_ARGV;i++)
        argv[i]='\0';
}

void user_freeall()
{
    uptr lead=user_head, prev=lead;
    while(lead!=NULL){
        prev=lead;
        lead=lead->link;
        free(prev);
    }
}

uptr user_reset_userlist()
{
    user_head=(uptr)malloc(sizeof(user));
    user_head->gid=user_head->uid=0;
    user_head->link=NULL;
    user_head->user_root=root;
    user_head->recent_time=make_fd_refresh_time();
    strcpy(user_head->name,"root");
    strcpy(user_head->passwd,"root");
    strcpy(user_head->home_path,"/");
    return user_head;
}

void user_save_userlist(char *save_dir)
{
    FILE *fp;
    if((fp=fopen(save_dir,"wb"))==NULL){
        printf("File open error!\n"); return;
    }
    uptr lead=user_head;
    while(lead!=NULL){
        fwrite(lead,sizeof(user),1,fp);
        lead=lead->link;
    }
    fclose(fp);
}

void user_load_deletelst()
{
    uptr prev,lead=user_head,delete_node;
    while(lead->link!=NULL){
        prev=lead;
        lead=lead->link;
    }
    prev->link=NULL;
    delete_node=lead;
    free(delete_node);
}

void user_load_userlist(char *load_dir)
{
    FILE *fp;
    if((fp=fopen(load_dir,"rb"))==NULL){
        printf("File open error!\n"); return;
    }
    while(!feof(fp)){
        fptr curtemp=root;
        uptr newp=(uptr)malloc(sizeof(user));
        fread(newp,sizeof(user),1,fp);
        newp->link=NULL;
        cd(&curtemp,newp,newp->home_path);
        newp->user_root=curtemp;
        if(user_head==NULL) user_head=newp;
        else{
            uptr lead=user_head;
            while(lead->link!=NULL){
                lead=lead->link;
            }
            lead->link=newp;
        }
    }
    user_load_deletelst();
    fclose(fp);
}

int is_root(uptr cur_user)
{
    if(cur_user->uid==0&&cur_user->gid==0) return 1;
    return 0;
}

void user_get_password(uptr *newuser)
{
    char Retype[MAX_PASSWD];
    while(1){
        printf("Enter new UNIX password:");
        gets((*newuser)->passwd);
        //scanf("%s",(*newuser)->passwd);
        printf("Retype new UNIX password:");
        gets(Retype);
        //scanf("%s",Retype);
        if(strcmp((*newuser)->passwd,Retype)){
            char yesorno;
            printf("Sorry, passwords do not match\n Try again? [y/n]\n");
            scanf("%c",yesorno);
            //scanf("%c",&yesorno);
            if(yesorno=='y') continue;
            else return;
        }
        else break;
    }
}

void get_homedir(char dir[],char username[])
{
    printf("username:%s\n",username);
    strcpy(dir,"/home/");
    int dirlen=strlen(dir), i=0;
    while(username[i]!='\0'){
        dir[dirlen]=username[i];
        dirlen++; i++;
    }
    dir[dirlen]='\0';
}

uptr user_get_new_user(uptr cur_user,char username[])
{
    printf("Adding user \'%s\' ...\n",username);
    uptr newuser=(uptr)malloc(sizeof(user));
    newuser->link=NULL;
    strcpy(newuser->name,username);
    uptr prev=user_head, lead=user_head;
    while(lead->link!=NULL){
        prev=lead;
        lead=lead->link;
    }
    newuser->gid=prev->gid+1;
    newuser->uid=prev->uid+1;
    printf("Adding new group \'%s\' (%d) ...\n",newuser->name,newuser->gid);
    printf("Adding new group \'%s\' (%d) with group \'%s\' ...\n",newuser->name,newuser->gid,username);
    lead->link=newuser;
    fptr curtemp=root;
    char home_dir[MAX_DIR];
    get_homedir(home_dir,username);
    mkdir(&root,cur_user,home_dir);
    printf("Creating home directory \'%s\' ...\n",home_dir);
    strcpy(newuser->home_path,home_dir);
    cd(&curtemp,newuser,newuser->home_path); // get home_path pointer address at curtemp
    newuser->user_root=curtemp;
    user_get_password(&newuser);
    newuser->recent_time=make_fd_refresh_time();
}

void adduser(uptr cur_user,char remain[])
{
    if(!is_root(cur_user)){
        printf("adduser: Only root may add a user or group to the system.\n");
        return;
    }
    else if(!optchk(remain,"adduser"))
        return;
    uptr newuser=user_get_new_user(cur_user,remain);
}

uptr welcome_compareID(char *account,char *passwd)
{
    int fail=0;
    uptr lead=user_head;
    while(lead!=NULL&&strcmp(lead->name,account)){
        lead=lead->link;
    }
    if(lead==NULL) return NULL;
    else if(strcmp(lead->passwd,passwd)) return NULL;
    else return lead;
}

uptr welcome_linux(fptr *cur)
{
    printf("%s release %s\n",os_name,current_ver);
    char account[MAX_NAME],passwd[MAX_PASSWD];
    uptr login_user=NULL;
    while(1){
        printf("localhost login: ");
        gets(account);
        printf("Password : ");
        gets(passwd);
        if((login_user=welcome_compareID(account,passwd))!=NULL) break;
        else printf("Login incorrect\n\n");
    } // month day year
    printf("Login Success\n User->home_path:%s\n",login_user->home_path);
    printf("Last login: %s %d %d %d:%d:%d\n",get_month(login_user->recent_time.month),login_user->recent_time.day,login_user->recent_time.year+1900,login_user->recent_time.hour,login_user->recent_time.min,login_user->recent_time.sec);
    login_user->recent_time=make_fd_refresh_time();
    cd(cur,login_user,login_user->home_path);
    return login_user;
}

void su(fptr *cur,uptr *cur_user,char username[])
{
    if(!optchk(username,"su")) return;
    uptr lead=user_head;
    while(lead!=NULL&&strcmp(lead->name,username)){
        lead=lead->link;
    }
    if(lead==NULL) printf("No passwd entry for user \'%s\'\n",username);
    else{
        char passwd[MAX_PASSWD];
        printf("Password: "); gets(passwd);
        if(strcmp(lead->passwd,passwd)) printf("su: Authentication failure\n");
        else{
            *cur_user=lead;
            (*cur_user)->recent_time=make_fd_refresh_time();
        }
    }
}

int dec_to_octal(int n)
{
    if(n<=7)
        return n;
    return dec_to_octal(n/8)*10+n%8;
}

void permisstion_to_octal(fptr *cur,char mode[])
{
    char octal[4];
    int i=0,j=0,searchset[]={0,3,6},valueset[]={4,2,1};
    while(i<3){
    int sum=0;;
    for(j=searchset[i];j<searchset[i]+3;j++){
        if((*cur)->permission[j]!='-'){
            sum+=valueset[j%3];
        }
    }
    octal[i]=sum+48;
    i++;
    }
    octal[i]='\0';
    get_permission(cur,dec_to_octal(strtoul(octal, NULL, 8)+strtoul(mode, NULL, 8)));
}

void chmod(fptr *cur,uptr cur_user,char remain[])
{
    if(!optchk(remain,"chmod")) return;
    char *token=strtok(remain," "),*path,mode[4]={'\0'};
    int i=0;
    while(token!=NULL){
        if(i==0){
            if(isoctal(token)) strcpy(mode,token);
            else{
                printf("chmod: invalid mode: \'%s\'\n",token);
                return;
            }
        }
        else if(i==1){
            path=(char*)malloc(sizeof(char)*strlen(token));
            strcpy(path,token);
            break;
        }
        i++;
        token=strtok(NULL," ");
    }
    fptr curtemp=*cur;
    token=strtok(path,"/");
    while(token!=NULL){
        if((curtemp=change_directory(cur,&curtemp,cur_user,token,"chmod"))==NULL) return;
        token=strtok(NULL, "/");
    }
    if(!check_permission(curtemp,cur_user,'w')){
        printf("chmod: cannot open directory \'%s\': Permission denied\n",curtemp->name);
        return;
    }
    permisstion_to_octal(&curtemp,mode);
    curtemp->recent_time=make_fd_refresh_time();
}

int rm(fptr *cur,uptr *cur_user,char* remain)
{
	int state = 1;
	int option = 1;
	fptr delNode = NULL;
	fptr searchNode = NULL;
	fptr prevNode = NULL;
	prevNode = searchNode = (*cur)->lower;

	if(prevNode == NULL)
	{
		printf("[Error] %s 는 존재하지 않습니다.\n",remain);
		return 0;
	}
	else if(strcmp(searchNode->name, remain)==0)
	{
		(*cur)->lower= searchNode->sbling;
		delNode = searchNode;
	}
	else
	{
		searchNode = searchNode->sbling;
		while( searchNode != NULL )
		{
			if( strcmp(searchNode -> name , remain) ==0 )
			{
				delNode = searchNode;
				break;
			}
			else
			{
				prevNode = searchNode;
				searchNode = searchNode -> sbling;
			}
		}
	}
	if(delNode != NULL)
	{
		if( delNode->lower != NULL )
		{
			printf("rmdir: %s 디렉토리가 비어 있지 않음 \n",delNode->name);
			return 0;
		}

		// ----------------------- 해당 위치에서의 허가권 검사 -----------------------
		if( (state = check_permission(delNode,cur_user,'w')) == 0 )
		{
			printf("rmdir: '%s' 을(를) 삭제 할 수 없습니다.: 허가가 거부됨 \n",remain);
			return 0;
		}
		// ----------------------- @해당 위치에서의 허가권 검사 ----------------------

		prevNode->sbling = delNode->sbling;
		if(option == 1)		// 옵션이 1일경우 디렉토리 삭제 ..
		{
			freeall(delNode->lower);
		}
		free(delNode);
		return 1;
	}
	else
	{
		printf("[Error] %s 는 존재하지 않습니다. \n",remain);
		return 0;
	}
}

int main()
{
    fptr cur=NULL;
    //get_fd(&cur,'d',"root",777); // load_mydir 쓸꺼면 주석처리하고, load 안헐꺼면 이거 활성화
    //get_fd(&cur,'d',"home",755);
    load_mydir(&cur,"directory.bin"); // if you want to load previous directory, use this.
    //uptr cur_user=user_reset_userlist();
    //cur_user=welcome_linux(&cur);
    user_load_userlist("userlist.bin");
    uptr cur_user=welcome_linux(&cur);
    preorder(root), printf("\n");
    char argv[MAX_ARGV]={'\0'};
    printf("*------------Welcome to the DGU OS project system------------*\n[Notice]If you want to exit, enter \"exit\".\n");
    while(printf("%s:%s$ ",cur_user->name,cur->name)&&gets(argv)!=EOF&&strncmp(argv,"exit",4)){
        if(!strncmp(argv," ",1)||strlen(argv)<=0) continue;
        char cmd[MAX_CMD], remain[MAX_ARGV-MAX_CMD];
        split_cmd(argv,cmd,remain);
        if(!strcmp(cmd,"cd")) cd(&cur,cur_user,&remain);
        else if(!strcmp(cmd,"mkdir")) mkdir(&cur,cur_user,&remain);
        else if(!strcmp(cmd,"ls")) ls(cur,cur_user,&remain);
        else if(!strcmp(cmd,"pwd")) pwd(cur,remain);
        else if(!strcmp(cmd,"adduser")) adduser(cur_user,remain);
        else if(!strcmp(cmd,"su")) su(&cur,&cur_user,remain);
        else if(!strcmp(cmd,"chmod")) chmod(&cur,cur_user,remain);
        else if(!strcmp(cmd,"rm")) rm(&cur,cur_user,remain);
        else printf("Command \'%s\' not found.\n",cmd);
        save_fd("directory.bin");
        user_save_userlist("userlist.bin");
        cur_user->recent_time=make_fd_refresh_time();
    }
    save_fd("directory.bin");
    user_save_userlist("userlist.bin");
    user_freeall();
    freeall(root);
    return 0;
}
