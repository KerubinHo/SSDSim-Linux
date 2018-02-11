#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include "avlTree.h"

#define READ 1
#define WRITE 0

struct ssd_info{
	struct dram_info *dram;
	struct parameter_value *parameter;   //SSD参数因子
	struct request *request_tail;	     // the tail of the request queue
    int64_t current_time;                //记录该通道的当前时间
};

struct request{
	unsigned int lsn;                  //请求的起始地址，逻辑地址
	unsigned int size;                 //请求的大小，既多少个扇区
	unsigned int operation;            //请求的种类，1为读，0为写
	unsigned int* need_distr_flag;
	unsigned int complete_lsn_count;   //record the count of lsn served by buffer
	int distri_flag;		           // indicate whether this request has been distributed already
    int64_t begin_time;
	int64_t response_time;
	struct sub_request *subs;          //链接到属于该请求的所有子请求
};

struct dram_info{
	unsigned int dram_capacity;     
	int64_t current_time;
	struct buffer_info *buffer; 
};

struct parameter_value{
	unsigned int subpage_page;
};

struct sub_request{
	int size;
	struct sub_request *next_subs;    //指向属于同一个request的子请求
};


/*********************************************************************************************
*buffer中的已写回的页的管理方法:在buffer_info中维持一个队列:written。这个队列有队首，队尾。
*每次buffer management中，请求命中时，这个group要移到LRU的队首，同时看这个group中是否有已
*写回的lsn，有的话，需要将这个group同时移动到written队列的队尾。这个队列的增长和减少的方法
*如下:当需要通过删除已写回的lsn为新的写请求腾出空间时，在written队首中找出可以删除的lsn。
*当需要增加新的写回lsn时，找到可以写回的页，将这个group加到指针written_insert所指队列written
*节点前。我们需要再维持一个指针，在buffer的LRU队列中指向最老的一个写回了的页，下次要再写回时，
*只需由这个指针回退到前一个group写回即可。
**********************************************************************************************/
typedef struct buffer_group{
	TREE_NODE node;                     //树节点的结构一定要放在用户自定义结构的最前面，注意!
	struct buffer_group *LRU_link_next;	// next node in LRU list
	struct buffer_group *LRU_link_pre;	// previous node in LRU list

	unsigned int group;                 //the first data logic sector number of a group stored in buffer 
	unsigned int stored;                //indicate the sector is stored in buffer or not. 1 indicates the sector is stored and 0 indicate the sector isn't stored.EX.  00110011 indicates the first, second, fifth, sixth sector is stored in buffer.
	unsigned int dirty_clean;           //it is flag of the data has been modified, one bit indicates one subpage. EX. 0001 indicates the first subpage is dirty
	int flag;			                //indicates if this node is the last 20% of the LRU list	
}buf_node;

struct ssd_info *buffer_management(struct ssd_info *);
struct ssd_info *insert2buffer(struct ssd_info *,unsigned int,int,struct sub_request *,struct request *);
unsigned int size(unsigned int stored);
void alloc_assert(void *p,char *s);
int keyCompareFunc(TREE_NODE *p , TREE_NODE *p1);
int freeFunc(TREE_NODE *pNode);
