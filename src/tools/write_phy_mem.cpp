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
int main(int argc, char * argv[])
{
	unsigned long pa;
	unsigned long data;
	int len;
	int memfd;
	int pageSize;
	int shift;
	int size;
	int do_mlock;
	void volatile *mapStart;
	void volatile *mapAddr;
	unsigned long pa_base;
	unsigned long pa_offset;
	if(argc != 3)
	{
		printf("Usage:./write_phy_mem addr data\n");
		return 0;
	}
	sscanf(argv[1], "%lx", &pa);
	len = sizeof(long);
	sscanf(argv[2], "%lx", &data);
	memfd = open("/dev/mem", O_RDWR | O_SYNC);
	if(memfd == -1)
	{
		perror("Failed to open /dev/mem");
		return FAIL;
	}

	shift = 0;
	pageSize = PAGE_SIZE;
	while(pageSize > 0)
	{
		pageSize = pageSize >> 1;
		shift ++;
	}
	shift --;
	pa_base = (pa >> shift) << shift;
	pa_offset = pa - pa_base;

	mapStart = (void volatile *)mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, memfd, pa_base);
	if(mapStart == MAP_FAILED)
	{
		perror("Failed to mmap /dev/mem");
		close(memfd);
		return FAIL;
	}
	if(mlock((void *)mapStart, PAGE_SIZE) == -1)
	{
		perror("Failed to mlock mmaped space");
        do_mlock = 0;
    }
    do_mlock = 1;

    mapAddr = (void volatile *)((unsigned long)mapStart + pa_offset);

	/// ��Խ������ҳ��
    if(len + pa_offset > PAGE_SIZE)
    {
  	    size = PAGE_SIZE - pa_offset;
    }
    else
    {
  	    size = len;
    }
    printf("before write, data at 0x%lx : 0x%lx\n", pa, *(unsigned long *)mapAddr);
    memcpy((void *)mapAddr, &data, size);
    printf("after  write, data at 0x%lx : 0x%lx\n", pa, *(unsigned long *)mapAddr);
    if(munmap((void *)mapStart, PAGE_SIZE) != 0)
    {
  	    perror("Failed to munmap /dev/mem");
    }
	close(memfd);
	return OK;
}
