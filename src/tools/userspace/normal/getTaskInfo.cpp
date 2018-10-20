#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <iostream>
#include <exception>
using namespace std;

#define     KERNEL_STACK


#define     STACK_SIZE   0X1000



typedef struct procMMInfo
{
	unsigned long total;	//���̵�ַ�ռ��С
	unsigned long locked;	//����ס���޷�������ҳ����
	unsigned long shared;	//�����ڴ�ӳ��
	unsigned long exec;		//��ִ���ڴ�ӳ��
    unsigned long stack;	//�û���ջ

    unsigned long reserve;//������
	unsigned long def_flags;//
	unsigned long nr_ptes;	//
	unsigned long start_code;	//����ο�ʼ��ַ
	unsigned long end_code;		//����ν�����ַ

    unsigned long start_data;	//���ݶο�ʼ��ַ
	unsigned long end_data;		//���ݶν�����ַ
	unsigned long start_brk;	//�ѵ���ʼ��ַ
	unsigned long brk;				//�ѵĵ�ǰ����ַ
	unsigned long start_stack;//�û���ջ����ʼ��ַ

#ifdef KERNEL_STACK
    unsigned long kstack;       //  �ں�ջ��thread_info�ĵ�ַ
    unsigned long start_kstack; //  kstack + sizeof(struct thread_info)
    unsigned long end_kstack;   //  kstack + sizeof(union thread_union)
#endif
    unsigned long arg_start;	//�����в���
    unsigned long arg_end;

    unsigned long env_start;	//��������
	unsigned long env_end;
} taskMMInfo, *pTaskMMInfo;

#define OK		0
#define FAIL	1

#define PAGE_SIZE 65536
#define MAX_LINE	PAGE_SIZE


// procMMInfo�ṹ���еĳ�Ա��Ŀ, ���ڶ���unsigned long��ʽ,
// ��˿���ʹ��sizeofֱ�Ӽ���
#define varCount	(sizeof(struct procMMInfo)/sizeof(unsigned long))
/*
#ifdef KERNEL_STACK
#define varcount    22
#else
#define varcount    19
#endif


/*
*	request command
*/
#define REQUEST_TASK_INFO		1		/// get a task's memory map information
#define REQUEST_V2P					2		/// convert a process's linear address to physical address
#define REQUEST_KV2P				3		/// convert kernel virtual address to physical address
#define REQUEST_KFUNC_VA		4		/// get kernel function's addr(kernel virtual address)
#define REQUEST_READ_KFUNC	5		/// �����ȡ�ں˺�����ʼ��ַ����
#define REQUEST_WRITE_KFUNC	6		/// �����д�ں˺�����ʼ��ַ����
///#define REQUEST_WRITE				10 	/// �����дָ�������ַ���ݣ���Ϊ�û�̬ʵ�ִ˹���
///#define REQUEST_MEM					11	/// �����ȡȫ�������ڴ���Ϣ
///#define REQUEST_ADDR_STOP		12	///

/*
*	ack signals
*/
#define ACK_TASK_INFO			REQUEST_TASK_INFO
#define ACK_V2P						REQUEST_V2P
#define ACK_KV2P					REQUEST_KV2P
#define ACK_KFUNC_VA			REQUEST_KFUNC_VA
#define ACK_READ_KFUNC		REQUEST_READ_KFUNC
#define ACK_WRITE_KFUNC		REQUEST_WRITE_KFUNC
///#define REQUEST_WRITE			REQUEST_WRITE
///#define ACK_MEM						REQUEST_MEM
///#define ACK_ADDR_STOP			REQUEST_ADDR_STOP

/*
*	utility functions
*/
int getTaskInfo(int pid);
void startExe(char **exe);
int ReadLine(char *input,char *line);

int main(int argc, char * argv[])
{
	int             pid;
    int             ret;
    char            args[MAX_LINE];

	if(argc < 3)
	{
		printf("Useage:./getTaskInfo [-p pid] | [ -e exefile]\n");
        exit(-1);
    }
    if(strcmp(argv[1], "-e") == 0)
    {

	    pid = vfork();
	    if(pid < 0)
    	{
	    	perror("fork");
		    return FAIL;
	    }
	    else if(pid == 0)	/// child
	    {
            // printf("child pid = %d\n", getpid( ));
		    startExe(argv+ 2);
	        //execv(argv[2], argv + 2);
		    _exit( -1 );
	    }
        else
        {
            printf("child pid = %d\n", pid);
        }
    }
    else if(strcmp(argv[1], "-p") == 0)
    {
	    pid = atoi(argv[2]);
    }
    else
    {
		printf("Useage:./getTaskInfo pid\n");
        exit(-1);
    }
    getTaskInfo(pid);

    return 0;
}


int getTaskInfo(int pid)
{
	pTaskMMInfo     taskInfo;
	int             ret;
	int             count;
	int             procFile;
	char            buff[MAX_LINE];
	char            line[MAX_LINE];
	unsigned long   iLine;
	int             i;

    taskInfo = (pTaskMMInfo)malloc(sizeof(taskMMInfo));
	bzero(buff, sizeof(buff));
	sprintf(buff,"echo %d > /proc/memoryEngine/pid", pid);
    printf("%s\n", buff);
	system(buff);
	//send control word
	bzero(buff, sizeof(buff));
	sprintf(buff,"echo %d > /proc/memoryEngine/ctl", REQUEST_TASK_INFO);
    printf("%s\n", buff);
	system(buff);
    //wait for ack signal
	procFile = open("/proc/memoryEngine/signal", O_RDONLY);
	if(procFile == -1)
	{
		perror("Failed to open /proc/memoryEngine/signal");
		return FAIL;
	}
    else
    {
        printf("open success...");
    }

	do
    {
		bzero(buff, sizeof(buff));
		ret = read(procFile, buff, MAX_LINE);

        if(ret == -1)
		{
			perror("Failed to read /proc/memoryEngine/signal");
			return FAIL;
		}
        printf("buff = %s\n", buff);
        //getchar();

    }while(atoi(buff) != ACK_TASK_INFO);

    close(procFile);

	//read task info
	procFile = open("/proc/memoryEngine/taskInfo", O_RDONLY);
	if(procFile == -1)
	{
		perror("Failed to open /proc/memoryEngine/taskInfo");
		return FAIL;
	}
	bzero(buff, sizeof(buff));
	ret = read(procFile, buff, MAX_LINE);
	if(ret == -1)
	{
		perror("Failed to read /proc/memoryEngine/taskInfo");
		return FAIL;
	}
	close(procFile);

	//printf("%s",buff);

	//fill struct taskMMInfo
	count = 0;
	for(i = 0; i < varCount; i++)
	{
		bzero(line, sizeof(line));
		count += ReadLine(buff + count, line);
		sscanf(line, "%lx", &iLine);
		memcpy((void *)((unsigned long)taskInfo + i * sizeof(unsigned long)),&iLine,sizeof(unsigned long));
	}
	for(i = 0; i < varCount; i++)
	{
		printf("0x%lx\n", *(unsigned long *)((unsigned long)taskInfo+i*sizeof(unsigned long)) );
	}

    /*
	printf("start_code\t\t%lx\n", taskInfo->start_code);
	printf("end_code\t\t%lx\n", taskInfo->end_code);
	printf("start_data\t\t%lx\n", taskInfo->start_data);
	printf("end_data\t\t%lx\n", taskInfo->end_data);
	printf("start_brk\t\t%lx\n", taskInfo->start_brk);
	printf("brk\t\t\t%lx\n", taskInfo->brk);
	printf("start_stack\t\t%lx\n", taskInfo->start_stack);
    */
	printf("code    : [0x%lx, 0x%lx]\n", taskInfo->start_code, taskInfo->end_code);
	printf("data    : [0x%lx, 0x%lx]\n", taskInfo->start_data, taskInfo->end_data);
            //  heap�ӵ͵�ַ��ߵ�ַ��չ�����ڴ�������Ҫ��Щ��
            //  stack�Ӹߵ�ַ��͵�ַ��չ������ջ�ռ����ʼλ�þ���ȷ����������̬�ĵ���ջ�ռ��СҲ����Ҫ�ƶ�ջ�ڵ����ݡ�
	printf("heap    : [0x%lx, 0x%lx]\n", taskInfo->start_brk, taskInfo->brk);
	printf("stack   : [0x%lx, 0x%lx]\n", taskInfo->start_stack - STACK_SIZE, taskInfo->start_stack);

#ifdef KERNEL_STACK
    printf("ksatck  : 0x%lx\n", taskInfo->kstack);
	printf("kstack  : [0x%lx, 0x%lx]\n", taskInfo->start_kstack, taskInfo->end_kstack);
#endif
    return OK;
}

/*
*	Read a line from a string.
*/
int ReadLine(char *input,char *line)
{
	int iLen = 0;
	while(input[iLen] != '\n' && input[iLen] != '\0')
	{
		iLen ++;
	}
	memcpy(line,input,iLen);
	return ++iLen;
}

void startExe(char **exe)
{
	int fd;
	if((fd = open("/dev/null", O_RDWR)) == -1)
	{
		perror("open");
		_exit(FAIL);
	}
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
	close(fd);

	execv(exe[0], exe);

}
