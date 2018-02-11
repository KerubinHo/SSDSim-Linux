#include "buffer.h"

int main () {
	struct ssd_info *ssd;
	struct request *request1;
	
	ssd=(struct ssd_info*)malloc(sizeof(struct ssd_info));
	alloc_assert(ssd,"ssd");
	ssd->parameter = (struct parameter_value *)malloc(sizeof(struct parameter_value));
	alloc_assert(ssd->parameter,"parameter_value");
	ssd->dram = (struct dram_info *)malloc(sizeof(struct dram_info));
	alloc_assert(ssd->dram,"ssd->dram");

	memset(ssd,0, sizeof(struct ssd_info));
	memset(ssd->dram,0,sizeof(struct dram_info));

	ssd->parameter->subpage_page = 4;
	ssd->dram->dram_capacity = 131074;
	ssd->dram->buffer = (tAVLTree *)avlTreeCreate((void*)keyCompareFunc , (void *)freeFunc);

	request1 = (struct request*)malloc(sizeof(struct request));
	alloc_assert(request1,"request");	
	memset(request1,0, sizeof(struct request));

	request1->lsn = 0;
	request1->size = 4;
	request1->operation = 1;	
	request1->begin_time = 1000;
	request1->response_time = 0;	
	request1->distri_flag = 0;              // indicate whether this request has been distributed already
	request1->subs = NULL;
	request1->need_distr_flag = NULL;
	request1->complete_lsn_count=0;
	
	buffer_management(ssd);
}
