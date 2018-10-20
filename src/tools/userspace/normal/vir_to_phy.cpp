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
	unsigned long arg_start;	//�����в���
	unsigned long arg_end;
	unsigned long env_start;	//��������
	unsigned long env_end;
} taskMMInfo, *pTaskMMInfo;

#define OK		0
#define FAIL	1

#define PAGE_SIZE 65536
#define MAX_LINE	PAGE_SIZE
#define varCount	19

/*
*	request command
*/
#define REQUEST_TASK_INFO		1		/// get a task's memory map information
#define REQUEST_V2P					2		/// convert a process's linear address to physical address
#define REQUEST_KV2P				3		/// convert kernel virtual address to physical address
#define REQUEST_KFUNC_VA		4		/// get kernel function's addr(kernel virtual address)
#define REQUEST_READ_KFUNC	5		/// �����ȡ�ں˺�����ʼ��ַ����
#define REQUEST_WRITE_KFUNC	6		/// �����д�ں˺�����ʼ��ַ����

//#define REQUEST_WRITE				10 	/// �����дָ�������ַ���ݣ���Ϊ�û�̬ʵ�ִ˹���
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
int main(int argc, char *argv[])
{
	int pid;
	unsigned long va;
	int ret;
	int procFile;
	unsigned long physicalAddr = 0;
	char buff[MAX_LINE];

    if(argc != 3)   //  ��Ҫ3������
	{
		printf("Useage:./vir_to_phy pid vir_addr\n");
		return 0;

        //  argv[0] -=>  ������������
        //  argv[1] -=>  ��������̵�pid
        //  argv[2] -=>  ��ת���Ľ����ڵ�ַ
    }
	pid = atoi(argv[1]);            //  ��������̵�pid
	sscanf(argv[2], "%lx", &va);    //  ��������ڴ��ַ
	if(pid <= 0)
	{
		return FAIL;
	}

	// send control word
	bzero(buff, sizeof(buff));
	sprintf(buff, "echo %d > /proc/memoryEngine/pid", pid);
	printf("%s\n", buff);
    system(buff);

    bzero(buff, sizeof(buff));
	sprintf(buff, "echo %lx > /proc/memoryEngine/virtualAddr", va);
	printf("%s\n", buff);
	system(buff);

    bzero(buff, sizeof(buff));
	sprintf(buff, "echo %d > /proc/memoryEngine/ctl", REQUEST_V2P);
	printf("%s\n", buff);
	system(buff);

	//wait for ack signal
	procFile = open("/proc/memoryEngine/signal", O_RDONLY);
	if(procFile == -1)
	{
		perror("Failed to open /proc/memoryEngine/signal");
		return FAIL;
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
	} while(atoi(buff) != ACK_V2P);
	close(procFile);

	//read physicalAddr
	procFile = open("/proc/memoryEngine/physicalAddr",O_RDONLY);
	if(procFile == -1)
	{
		perror("Failed to open /proc/memoryEngine/physicalAddr");
		return FAIL;
	}
	bzero(buff, sizeof(buff));
	ret = read(procFile, buff, MAX_LINE);
	if(ret == -1)
	{
		perror("Failed to read /proc/memoryEngine/physicalAddr");
		return FAIL;
	}
	close(procFile);

	if(sscanf(buff, "%lx", &physicalAddr) != 1)
    {
        return FAIL;
    }
	printf("0x%lx\n", physicalAddr);
	return 0;
}
