#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#define MAX_ARGV 256
#define MAX_CMD 64
typedef struct rtime{
    int year,month,day,hour,min,sec;
}rtime;
typedef struct fnode *fptr;
typedef struct fnode{
    int id; // node id(address value) (ex:1382842368)
    char permission[9]; // permission (ex:rw-rwxrwx)
    bool hide; // Whether it's a hidden file or not.
    char type; // 'd' is directory. '-' is file.
    char owner[20]; // File or directory owner
    char group[20]; // File or directory ownership group
    char name[20]; // Dir/file name
    rtime recent_time;
    fptr upper,lower,sbling;
}fnode;
typedef struct group *gptr;
typedef struct group{
    char name[20];
    int mode;
}group;
typedef struct user *uptr;
typedef struct user{
    char name[20];
    char pswd[20];
    char group[20];
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
fptr make_fd(char type,char *fdname,fptr *upper,int mode);
fptr get_fd(fptr *mydir, char type, char *newname, int modep);
fptr change_directory(fptr *cur,fptr *_cur,char dirname[],char mode[]);
int check_arg(char *argv[], int max);
void get_permission(fptr *cur,int mode);
fptr chdir(fptr *curr,char *path[],char mode[]);
fptr root=NULL;

/*void rm(fptr *cur,char *argv[])
{

}
*/
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

int pwd_optchk(char remain[])
{
    int i=0;
    while(remain[i]!='\0'){
        if(remain[i]=='-'){
            while(remain[i]=='-'){
                i++;
            }
            printf("bash: pwd: -%c invalid option\n",remain[i]);
            return 0;
        } i++;
    }
    return 1;
}

void pwd(fptr curtemp,char remain[])
{
    if(!pwd_optchk(remain)) return;
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

fptr ls_getcur(fptr cur,char path[])
{
    fptr t=cur;
    if(path==NULL) return t;
    if(!strncmp(path,"/",1)) t=root;
    char *token=strtok(path,"/");
    while(token!=NULL){
        if(!strncmp(token,"~",1)) t=root;
        else if(!strncmp(token,".",1)) t=cur;
        else t=change_directory(&cur,&t,token,"ls");
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

void ls(fptr cur,char *argv[])
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
    fptr curtemp=ls_getcur(cur,path);
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

fptr make_fd(char type,char *fdname,fptr *upper,int mode)
{
    fptr fd=(fptr)malloc(sizeof(fnode));
    strcpy(fd->name,fdname); fd->type=type;
    fd->lower=fd->sbling=NULL; fd->id=fd; fd->hide=false;
    get_permission(&fd,mode);
    strcpy(fd->owner,"root"); // User 기능이 없으므로 일단 root로 소유권 둠.
    strcpy(fd->group,"root");
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
    char mode_s[4],pset[]={'r','w','x'},temp[4]="000\0";
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

fptr chkdir(fptr *cur,char path[],int mode)
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
    if(next_token==NULL) get_fd(&curtemp,'d',token,mode); // token=='TT', curtemp=test2
    else get_fd(&curtemp,'d',next_token,mode);
    return curtemp;
}

fptr fwddir_p(fptr *cur,fptr *curtemp,char dirn[],int mode)
{
    if(!strncmp(dirn,"~",1)) return root;
    else if(!strncmp(dirn,".",1)) return *cur;
    if((*curtemp)->lower==NULL){
        return get_fd(curtemp,'d',dirn,mode);
    }
    fptr t=(*curtemp)->lower;
    while(t!=NULL&&strcmp(t->name,dirn)){
        t=t->sbling;
    }
    if(t==NULL){
        return get_fd(curtemp,'d',dirn,mode);
    }
    else return t;
}

void chkdir_p(fptr *cur,char path[],int mode)
{
    fptr curtemp=*cur;
    char *token;
    if(!strncmp(path,"/",1)) curtemp=root;
    token=strtok(path,"/");
    while(token!=NULL){
        curtemp=fwddir_p(cur,&curtemp,token,mode);
        token=strtok(NULL,"/");
    }
}

void mkdir(fptr *cur, char *argv[])
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
            if(chkdir(cur,path_list[i],mode)==NULL) // Need the fork() p==0 case
                return NULL;
        }
    }
    else if(p==1){ // sequence generate part
        for(i=0;i<pathnum;i++){
            chkdir_p(cur,path_list[i],mode); // Need the fork() p==1 case
        }
    }
    for(i=0;i<pathnum;i++)
        free(path_list[i]);
    free(path_list);
}

fptr get_fd(fptr *mydir, char type, char *newname, int mode)
{
    if (*mydir==NULL){
        *mydir=make_fd('d',"root",mydir,mode);
        root=*mydir;
        return *mydir;
    }
    else{
        fptr temp, fd=make_fd(type,newname,mydir,mode);
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

fptr chdir(fptr *curr,char *path[],char mode[])
{
    fptr curtemp=*curr;
    char *s=(char*)malloc(strlen(path));strcpy(s,path);
    if(!strncmp(path,"/",1)){
        if((curtemp=change_directory(curr,&curtemp,"/",mode))==NULL)
            return NULL;
    }
    char *dir=strtok(s,"/");
    while(dir!=NULL)
    {
        if((curtemp=change_directory(curr,&curtemp,dir,mode))==NULL){
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

int cd(fptr *curr,char *argv[])
{
    if(check_arg(argv,1)){
        printf("bash: cd: too many arguments\n");
        return 0;
    }
    fptr curtemp;
    if ((curtemp=chdir(curr,argv,"cd"))!=NULL){
        *curr=curtemp;
    }
    else return 0;
    return 1;
}

fptr change_directory(fptr *cur,fptr *_cur,char dirname[],char mode[])
{
    if(!strcmp(dirname,"~")||!strcmp(dirname,"/")) return root; //dirname = "~/test/starcraft
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



/*int cat(fptr *cur, char filename[])
{
	int state = 1;
	fptr newptr = NULL;
	fptr tempptr = NULL;

	rtime rawtime;
    struct tm * timeinfo;		//	시간 설정을 위해

	char month[10];
	char year[10];

	newptr = (fptr)malloc( sizeof(fptr) );
	newptr->lower = NULL;
	newptr->sbling = NULL;

	time(&rawtime);
    timeinfo = localtime ( &rawtime ); //현재시간 측정

	if(chkfile(cur, filename)!=NULL && chkfile(cur, filename)->type !='d' )
	{
		printf("touch: '%s' 파일을 만들 수 없습니다 : 파일이 존재 합니다. \n",filename);
		return 0;
	}
	if(newptr != NULL)
	{
		strncpy(newptr->name,filename,20);
		strftime (year,10,"%Y",timeinfo);	//  현재 시간 정보를 바탕으로, 년도를 구함
		strftime (month,10,"%m",timeinfo);	//  현재 시간 정보를 바탕으로, 월을 구함
		newptr->recent_time->year = atoi(month);
		newptr->recent_time->month = atoi(year);
		if(filename[0] =='.')
			newptr->type ='user';//형이 수정중
		else
			newptr->type ='-';
		if(cur->upper == NULL )  // 초기 헤더노드의 정보를 루트로 연결한다.
		{
			cur->upper = newptr;
			strcpy(newptr->owner,"root");
			strcpy(newptr->group,"root");
			newptr->permission[0] = 'r';
			newptr->permission[1] = 'w';
			newptr->permission[2] = 'r';
			newptr->permission[3] = '-';
			newptr->permission[4] = '-';
			newptr->permission[5] = '-';
			newptr->permission[6] = '-';
			newptr->permission[7] = '-';
			newptr->permission[8] = '-';
			newptr->upper = NULL;
		}
		else // 루트가 존재할 경우
		{
			// ----------------------- 해당 위치에서 대상의 허가권 검사 -----------------------
			if( state = permissionHandler(cur->permission,'w') == 0 )
			{
					printf("touch: '%s' 파일을 만들 수 없습니다.: 허가 거부됨 \n",cur);
					return 0;
			}
			strcpy(newptr->owner,uptr->name);  // 추가하려는 유저명
			strcpy(newptr->group,gptr->name); // 추가하려는 유저명
			newptr->permission[0] = 'r';
			newptr->permission[1] = 'w';
			newptr->permission[2] = '-';
			newptr->permission[3] = 'r';
			newptr->permission[4] = '-';
			newptr->permission[5] = '-';
			newptr->permission[6] = 'r';
			newptr->permission[7] = '-;';
			newptr->permission[8] = '-';
			newptr->upper = fptr; // 각 노드의 Parrent 값을 currentNode로 변경함으로써 pwd 를 이용한 스텍저장을 가능하도록 함.
			// ---------------------------- 일단 디렉토리의 정보를 저장한다. ----------------------------------
			if(cur->lower == NULL)  // dTree 가 가지고있는 currentNode를 기준으로 하위 파일 및 디렉토리를 추가한다.
			{
				cur->lower = newptr;
			}
			else											// current_node의 자식이 존재하므로 rsibiling으로 계속 연결 시킨다.
			{
				tempptr = cur->lower;				// 현재 위치이다.
				while (tempptr->sibiling != NULL)				// 검색을 해가면서 RSibiling NULL인 부분을 찾는다.
				{
					tempptr = tempptr->sbiling;
				}
				tempptr->sbling = newptr;
			}
		}
	}
	else
	{
		printf("[Error] NULL , MakeDirectory() \n");
		return 0;
	}

	return 1;

}

fptr chkfile(fptr* cur, char fileName[]){
	fptr* returnptr = NULL;
	returnptr = cur->lower;
	while(returnptr != NULL)
	{
		if(strcmp(returnptr->name,fileName) == 0 )
		{
			break;
		}
		returnptr = returnptr->sbling;
	}
	return returnptr;								// 찾지 못할시 NULL 값을 넘긴다.
}



/*
int rm(fptr* cur, char dirname[],int opt){
	int state = 1;
	fptr* delNode = NULL;
	fptr* searchNode = NULL;
	fptr* prevNode = NULL;

	char yesOrno[10];

	prevNode = searchNode = cur->lower;


	if(prevNode == NULL)
	{
		printf("[Error] %s 는 존재하지 않습니다.\n",dirname);
		return 0;
	}
	else if(strcmp(searchNode->name, dirname)==0)
	{
		cur->lower= searchNode->sbling;
		delNode = searchNode;
	}
	else
	{
		searchNode = searchNode->sbling;
		while( searchNode != NULL )
		{
			if( strcmp(searchNode -> name , dirname) ==0 )
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
		if( delNode->lower != NULL && opt !=3 )
		{
			printf("rmdir: %s 디렉토리가 비어 있지 않음 \n",delNode->name);
			return 0;
		}
		// ----------------------- 해당 위치에서의 허가권 검사 -----------------------
		if( state = permissionHandler(delNode,'w') == 0 )
		{
			printf("rmdir: '%s' 을(를) 삭제 할 수 없습니다.: 허가가 거부됨 \n",dirname);
			return 0;
		}
		// ----------------------- @해당 위치에서의 허가권 검사 ----------------------

		prevNode->sbiling = delNode->sbiling;
		if(opti== 1)		// 옵션이 1일경우 디렉토리 삭제 ..
		{
			FreeSubDirectory(delNode->lower);
		}
		free(delNode);
		return 1;
	}
	else
	{
		printf("[Error] %s 는 존재하지 않습니다. \n",dirname);
		return 0;
	}
}*/

/*int permissionHandler(fptr* cur, char opt)
{
	if( strcmp(cur->name,"root") == 0)	// 최고 계정일 경우 무조건, TRUE
	{
		return 1;
	}
	else if( strcmp(cur->name,Node->username) == 0) // 현재 로그인한 일반유저와 , 대상의 소유 정보가 같을경우..
	{
		if( opt == 'r')
		{
			if(cur->permission[0] == 'r')
				return 1;
			else
				return 0;
		}
		else if( option == 'w')
		{
			if(cur->permission[1] == 'w')
				return 1;
			else
				return 0;

		}
		else if( option == 'x')
		{
			if(cur->permission[2] == 1)
				return 1;
			else
				return 0;
		}
	}
	else													// 현재 로그인한 유저와 , 대상의 소유 정보가 다를경우..
	{
		if( option == 'r')
		{
			if(cur->permission[6] == 1)
				return 1;
			else
				return 0;
		}
		else if( option == 'w')
		{
			if(Node->permission[7] == 1)
				return 1;
			else
				return 0;

		}
		else if( option == 'x')
		{
			if(Node->permission[8] == 1)
				return 1;
			else
				return 0;
		}
	}
	return 0;
}*/

int main()
{
    fptr cur=NULL;
    //get_fd(&cur,'d',"root",777); // load_mydir 쓸꺼면 주석처리하고, load 안헐꺼면 이거 활성화
    //printf("load end--------------------------------------------\n");
    load_mydir(&cur,"directory.bin"); // if you want to load previous directory, use this.
    preorder(root), printf("\n");
    char argv[MAX_ARGV]={'\0'};
    printf("*--Welcome to the DGU OS project system.--*\nIf you want to exit, enter \"exit\".\n");
    while(printf("%s:%s$ ",root->name,cur->name)&&gets(argv)!=EOF&&strncmp(argv,"exit",4)){
        //int cmdlen=0,remainlen=0,i=0;
        if(!strncmp(argv," ",1)||strlen(argv)<=0) continue;
        char cmd[MAX_CMD], remain[MAX_ARGV-MAX_CMD];
        split_cmd(argv,cmd,remain);
        if(!strcmp(cmd,"cd")) cd(&cur,&remain);
        else if(!strcmp(cmd,"mkdir")) mkdir(&cur,&remain);
        else if(!strcmp(cmd,"ls")) ls(cur,&remain);
        else if(!strcmp(cmd,"pwd")) pwd(cur,remain);
        else printf("Command \'%s\' not found.\n",cmd);
        save_fd("directory.bin");
    }
    freeall(root);
    return 0;
}
