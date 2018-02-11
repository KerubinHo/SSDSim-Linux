/*****************************************************************************************************************************
This project was supported by the National Basic Research 973 Program of China under Grant No.2011CB302301
Huazhong University of Science and Technology (HUST)   Wuhan National Laboratory for Optoelectronics

FileName£º flash.c
Author: Hu Yang		Version: 2.1	Date:2011/12/02
Description: 

History:
<contributor>     <time>        <version>       <desc>                   <e-mail>
Yang Hu	        2009/09/25	      1.0		    Creat SSDsim       yanghu@foxmail.com
                2010/05/01        2.x           Change 
Zhiming Zhu     2011/07/01        2.0           Change               812839842@qq.com
Shuangwu Zhang  2011/11/01        2.1           Change               820876427@qq.com
Chao Ren        2011/07/01        2.0           Change               529517386@qq.com
Hao Luo         2011/01/01        2.0           Change               luohao135680@gmail.com
*****************************************************************************************************************************/

#include "flash.h"

/**********************
*Õâ¸öº¯ÊýÖ»×÷ÓÃÓÚÐ´ÇëÇó
***********************/
Status allocate_location(struct ssd_info * ssd ,struct sub_request *sub_req)
{
	struct sub_request * update=NULL;
	unsigned int channel_num=0,chip_num=0,die_num=0,plane_num=0;
	struct local *location=NULL;

	channel_num=ssd->parameter->channel_number;
	chip_num=ssd->parameter->chip_channel[0];
	die_num=ssd->parameter->die_chip;
	plane_num=ssd->parameter->plane_die;
    
	
	if (ssd->parameter->allocation_scheme==0)                                          /*¶¯Ì¬·ÖÅäµÄÇé¿ö*/
	{
		/******************************************************************
		* ÔÚ¶¯Ì¬·ÖÅäÖÐ£¬ÒòÎªÒ³µÄ¸üÐÂ²Ù×÷Ê¹ÓÃ²»ÁËcopyback²Ù×÷£¬
		*ÐèÒª²úÉúÒ»¸ö¶ÁÇëÇó£¬²¢ÇÒÖ»ÓÐÕâ¸ö¶ÁÇëÇóÍê³Éºó²ÅÄÜ½øÐÐÕâ¸öÒ³µÄÐ´²Ù×÷
		*******************************************************************/
		if (ssd->dram->map->map_entry[sub_req->lpn].state!=0)    
		{
			if ((sub_req->state&ssd->dram->map->map_entry[sub_req->lpn].state)!=ssd->dram->map->map_entry[sub_req->lpn].state)
			{
				ssd->read_count++;
				ssd->update_read_count++;

				update=(struct sub_request *)malloc(sizeof(struct sub_request));
				alloc_assert(update,"update");
				memset(update,0, sizeof(struct sub_request));

				if(update==NULL)
				{
					return ERROR;
				}
				update->location=NULL;
				update->next_node=NULL;
				update->next_subs=NULL;
				update->update=NULL;						
				location = find_location(ssd,ssd->dram->map->map_entry[sub_req->lpn].pn);
				update->location=location;
				update->begin_time = ssd->current_time;
				update->current_state = SR_WAIT;
				update->current_time=MAX_INT64;
				update->next_state = SR_R_C_A_TRANSFER;
				update->next_state_predict_time=MAX_INT64;
				update->lpn = sub_req->lpn;
				update->state=((ssd->dram->map->map_entry[sub_req->lpn].state^sub_req->state)&0x7fffffff);
				update->size=size(update->state);
				update->ppn = ssd->dram->map->map_entry[sub_req->lpn].pn;
				update->operation = READ;
				
				if (ssd->channel_head[location->channel].subs_r_tail!=NULL)            /*²úÉúÐÂµÄ¶ÁÇëÇó£¬²¢ÇÒ¹Òµ½channelµÄsubs_r_tail¶ÓÁÐÎ²*/
				{
						ssd->channel_head[location->channel].subs_r_tail->next_node=update;
						ssd->channel_head[location->channel].subs_r_tail=update;
				} 
				else
				{
					ssd->channel_head[location->channel].subs_r_tail=update;
					ssd->channel_head[location->channel].subs_r_head=update;
				}
			}
		}
		/***************************************
		*Ò»ÏÂÊÇ¶¯Ì¬·ÖÅäµÄ¼¸ÖÖÇé¿ö
		*0£ºÈ«¶¯Ì¬·ÖÅä
		*1£º±íÊ¾channel¶¨package£¬die£¬plane¶¯Ì¬
		****************************************/
		switch(ssd->parameter->dynamic_allocation)
		{
			case 0:
			{
				sub_req->location->channel=-1;
				sub_req->location->chip=-1;
				sub_req->location->die=-1;
				sub_req->location->plane=-1;
				sub_req->location->block=-1;
				sub_req->location->page=-1;

				if (ssd->subs_w_tail!=NULL)
				{
					ssd->subs_w_tail->next_node=sub_req;
					ssd->subs_w_tail=sub_req;
				} 
				else
				{
					ssd->subs_w_tail=sub_req;
					ssd->subs_w_head=sub_req;
				}

				if (update!=NULL)
				{
					sub_req->update=update;
				}

				break;
			}
			case 1:
			{
				 
				sub_req->location->channel=sub_req->lpn%ssd->parameter->channel_number;
				sub_req->location->chip=-1;
				sub_req->location->die=-1;
				sub_req->location->plane=-1;
				sub_req->location->block=-1;
				sub_req->location->page=-1;

				if (update!=NULL)
				{
					sub_req->update=update;
				}

				break;
			}
			case 2:
			{
				break;
			}
			case 3:
			{
				break;
			}
		}

	}
	else                                                                          
	{   /***************************************************************************
		*ÊÇ¾²Ì¬·ÖÅä·½Ê½£¬ËùÒÔ¿ÉÒÔ½«Õâ¸ö×ÓÇëÇóµÄ×îÖÕchannel£¬chip£¬die£¬planeÈ«²¿µÃ³ö
		*×Ü¹²ÓÐ0,1,2,3,4,5,ÕâÁùÖÖ¾²Ì¬·ÖÅä·½Ê½¡£
		****************************************************************************/
		switch (ssd->parameter->static_allocation)
		{
			case 0:         //no striping static allocation
			{
				sub_req->location->channel=(sub_req->lpn/(plane_num*die_num*chip_num))%channel_num;
				sub_req->location->chip=sub_req->lpn%chip_num;
				sub_req->location->die=(sub_req->lpn/chip_num)%die_num;
				sub_req->location->plane=(sub_req->lpn/(die_num*chip_num))%plane_num;
				break;
			}
			case 1:
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/channel_num)%chip_num;
				sub_req->location->die=(sub_req->lpn/(chip_num*channel_num))%die_num;
				sub_req->location->plane=(sub_req->lpn/(die_num*chip_num*channel_num))%plane_num;
							
				break;
			}
			case 2:
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/(plane_num*channel_num))%chip_num;
				sub_req->location->die=(sub_req->lpn/(plane_num*chip_num*channel_num))%die_num;
				sub_req->location->plane=(sub_req->lpn/channel_num)%plane_num;
				break;
			}
			case 3:
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/(die_num*channel_num))%chip_num;
				sub_req->location->die=(sub_req->lpn/channel_num)%die_num;
				sub_req->location->plane=(sub_req->lpn/(die_num*chip_num*channel_num))%plane_num;
				break;
			}
			case 4:  
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/(plane_num*die_num*channel_num))%chip_num;
				sub_req->location->die=(sub_req->lpn/(plane_num*channel_num))%die_num;
				sub_req->location->plane=(sub_req->lpn/channel_num)%plane_num;
							
				break;
			}
			case 5:   
			{
				sub_req->location->channel=sub_req->lpn%channel_num;
				sub_req->location->chip=(sub_req->lpn/(plane_num*die_num*channel_num))%chip_num;
				sub_req->location->die=(sub_req->lpn/channel_num)%die_num;
				sub_req->location->plane=(sub_req->lpn/(die_num*channel_num))%plane_num;
							
				break;
			}
			default : return ERROR;
		
		}
		if (ssd->dram->map->map_entry[sub_req->lpn].state!=0)
		{                                                                              /*Õâ¸öÐ´»ØµÄ×ÓÇëÇóµÄÂß¼­Ò³²»¿ÉÒÔ¸²¸ÇÖ®Ç°±»Ð´»ØµÄÊý¾Ý ÐèÒª²úÉú¶ÁÇëÇó*/ 
			if ((sub_req->state&ssd->dram->map->map_entry[sub_req->lpn].state)!=ssd->dram->map->map_entry[sub_req->lpn].state)  
			{
				ssd->read_count++;
				ssd->update_read_count++;
				update=(struct sub_request *)malloc(sizeof(struct sub_request));
				alloc_assert(update,"update");
				memset(update,0, sizeof(struct sub_request));
				
				if(update==NULL)
				{
					return ERROR;
				}
				update->location=NULL;
				update->next_node=NULL;
				update->next_subs=NULL;
				update->update=NULL;						
				location = find_location(ssd,ssd->dram->map->map_entry[sub_req->lpn].pn);
				update->location=location;
				update->begin_time = ssd->current_time;
				update->current_state = SR_WAIT;
				update->current_time=MAX_INT64;
				update->next_state = SR_R_C_A_TRANSFER;
				update->next_state_predict_time=MAX_INT64;
				update->lpn = sub_req->lpn;
				update->state=((ssd->dram->map->map_entry[sub_req->lpn].state^sub_req->state)&0x7fffffff);
				update->size=size(update->state);
				update->ppn = ssd->dram->map->map_entry[sub_req->lpn].pn;
				update->operation = READ;
				
				if (ssd->channel_head[location->channel].subs_r_tail!=NULL)
				{
					ssd->channel_head[location->channel].subs_r_tail->next_node=update;
					ssd->channel_head[location->channel].subs_r_tail=update;
				} 
				else
				{
					ssd->channel_head[location->channel].subs_r_tail=update;
					ssd->channel_head[location->channel].subs_r_head=update;
				}
			}

			if (update!=NULL)
			{
				sub_req->update=update;

				sub_req->state=(sub_req->state|update->state);
				sub_req->size=size(sub_req->state);
			}

 		}
	}
	if ((ssd->parameter->allocation_scheme!=0)||(ssd->parameter->dynamic_allocation!=0))
	{
		if (ssd->channel_head[sub_req->location->channel].subs_w_tail!=NULL)
		{
			ssd->channel_head[sub_req->location->channel].subs_w_tail->next_node=sub_req;
			ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
		} 
		else
		{
			ssd->channel_head[sub_req->location->channel].subs_w_tail=sub_req;
			ssd->channel_head[sub_req->location->channel].subs_w_head=sub_req;
		}
	}
	return SUCCESS;					
}	


/*******************************************************************************
*insert2buffer这个函数是专门为写请求分配子请求服务的在buffer_management中被调用。
********************************************************************************/
struct ssd_info * insert2buffer(struct ssd_info *ssd,unsigned int lpn,int state,struct sub_request *sub,struct request *req)      
{
	int write_back_count,flag=0;                                                             /*flag表示为写入新数据腾空间是否完成，0表示需要进一步腾，1表示已经腾空*/
	unsigned int i,lsn,hit_flag,add_flag,sector_count,active_region_flag=0,free_sector=0;
	struct buffer_group *buffer_node=NULL,*pt,*new_node=NULL,key;
	struct sub_request *sub_req=NULL,*update=NULL;
	
	
	unsigned int sub_req_state=0, sub_req_size=0,sub_req_lpn=0;

	#ifdef DEBUG
	printf("enter insert2buffer,  current time:%I64u, lpn:%d, state:%d,\n",ssd->current_time,lpn,state);
	#endif

	sector_count=size(state);                                                                /*需要写到buffer的sector个数*/
	key.group=lpn;
	buffer_node= (struct buffer_group*)avlTreeFind(ssd->dram->buffer, (TREE_NODE *)&key);    /*在平衡二叉树中寻找buffer node*/ 
    
	/************************************************************************************************
	*没有命中。
	*第一步根据这个lpn有多少子页需要写到buffer，去除已写回的lsn，为该lpn腾出位置，
	*首先即要计算出free sector（表示还有多少可以直接写的buffer节点）。
	*如果free_sector>=sector_count，即有多余的空间够lpn子请求写，不需要产生写回请求
	*否则，没有多余的空间供lpn子请求写，这时需要释放一部分空间，产生写回请求。就要creat_sub_request()
	*************************************************************************************************/
	if(buffer_node==NULL)
	{
		free_sector=ssd->dram->buffer->max_buffer_sector-ssd->dram->buffer->buffer_sector_count;   
		if(free_sector>=sector_count)
		{
			flag=1;    
		}
		if(flag==0)     
		{
			write_back_count=sector_count-free_sector;
			ssd->dram->buffer->write_miss_hit=ssd->dram->buffer->write_miss_hit+write_back_count;
			while(write_back_count>0)
			{
				sub_req=NULL;
				sub_req_state=ssd->dram->buffer->buffer_tail->stored; 
				sub_req_size=size(ssd->dram->buffer->buffer_tail->stored);
				sub_req_lpn=ssd->dram->buffer->buffer_tail->group;
				sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE);
				
				/**********************************************************************************
				*req不为空，表示这个insert2buffer函数是在buffer_management中调用，传递了request进来
				*req为空，表示这个函数是在process函数中处理一对多映射关系的读的时候，需要将这个读出
				*的数据加到buffer中，这可能产生实时的写回操作，需要将这个实时的写回操作的子请求挂在
				*这个读请求的总请求上
				***********************************************************************************/
				if(req!=NULL)                                             
				{
				}
				else    
				{
					sub_req->next_subs=sub->next_subs;
					sub->next_subs=sub_req;
				}
                
				/*********************************************************************
				*写请求插入到了平衡二叉树，这时就要修改dram的buffer_sector_count；
				*维持平衡二叉树调用avlTreeDel()和AVL_TREENODE_FREE()函数；维持LRU算法；
				**********************************************************************/
				ssd->dram->buffer->buffer_sector_count=ssd->dram->buffer->buffer_sector_count-sub_req->size;
				pt = ssd->dram->buffer->buffer_tail;
				avlTreeDel(ssd->dram->buffer, (TREE_NODE *) pt);
				if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL){
					ssd->dram->buffer->buffer_head = NULL;
					ssd->dram->buffer->buffer_tail = NULL;
				}else{
					ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
					ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
				}
				pt->LRU_link_next=NULL;
				pt->LRU_link_pre=NULL;
				AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
				pt = NULL;
				
				write_back_count=write_back_count-sub_req->size;                            /*因为产生了实时写回操作，需要将主动写回操作区域增加*/
			}
		}
		
		/******************************************************************************
		*生成一个buffer node，根据这个页的情况分别赋值个各个成员，添加到队首和二叉树中
		*******************************************************************************/
		new_node=NULL;
		new_node=(struct buffer_group *)malloc(sizeof(struct buffer_group));
		alloc_assert(new_node,"buffer_group_node");
		memset(new_node,0, sizeof(struct buffer_group));
		
		new_node->group=lpn;
		new_node->stored=state;
		new_node->dirty_clean=state;
		new_node->LRU_link_pre = NULL;
		new_node->LRU_link_next=ssd->dram->buffer->buffer_head;
		if(ssd->dram->buffer->buffer_head != NULL){
			ssd->dram->buffer->buffer_head->LRU_link_pre=new_node;
		}else{
			ssd->dram->buffer->buffer_tail = new_node;
		}
		ssd->dram->buffer->buffer_head=new_node;
		new_node->LRU_link_pre=NULL;
		avlTreeAdd(ssd->dram->buffer, (TREE_NODE *) new_node);
		ssd->dram->buffer->buffer_sector_count += sector_count;
	}
	/****************************************************************************************
	*在buffer中命中的情况
	*算然命中了，但是命中的只是lpn，有可能新来的写请求，只是需要写lpn这一page的某几个sub_page
	*这时有需要进一步的判断
	*****************************************************************************************/
	else
	{
		for(i=0;i<ssd->parameter->subpage_page;i++)
		{
			/*************************************************************
			*判断state第i位是不是1
			*并且判断第i个sector是否存在buffer中，1表示存在，0表示不存在。
			**************************************************************/
			if((state>>i)%2!=0)                                                         
			{
				lsn=lpn*ssd->parameter->subpage_page+i;
				hit_flag=0;
				hit_flag=(buffer_node->stored)&(0x00000001<<i);
				
				if(hit_flag!=0)				                                          /*命中了，需要将该节点移到buffer的队首，并且将命中的lsn进行标记*/
				{	
					active_region_flag=1;                                             /*用来记录在这个buffer node中的lsn是否被命中，用于后面对阈值的判定*/

					if(req!=NULL)
					{
						if(ssd->dram->buffer->buffer_head!=buffer_node)     
						{				
							if(ssd->dram->buffer->buffer_tail==buffer_node)
							{				
								ssd->dram->buffer->buffer_tail=buffer_node->LRU_link_pre;
								buffer_node->LRU_link_pre->LRU_link_next=NULL;					
							}				
							else if(buffer_node != ssd->dram->buffer->buffer_head)
							{					
								buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;				
								buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;
							}				
							buffer_node->LRU_link_next=ssd->dram->buffer->buffer_head;	
							ssd->dram->buffer->buffer_head->LRU_link_pre=buffer_node;
							buffer_node->LRU_link_pre=NULL;				
							ssd->dram->buffer->buffer_head=buffer_node;					
						}					
						ssd->dram->buffer->write_hit++;
						req->complete_lsn_count++;                                        /*关键 当在buffer中命中时 就用req->complete_lsn_count++表示往buffer中写了数据。*/					
					}
					else
					{
					}				
				}			
				else                 			
				{
					/************************************************************************************************************
					*该lsn没有命中，但是节点在buffer中，需要将这个lsn加到buffer的对应节点中
					*从buffer的末端找一个节点，将一个已经写回的lsn从节点中删除(如果找到的话)，更改这个节点的状态，同时将这个新的
					*lsn加到相应的buffer节点中，该节点可能在buffer头，不在的话，将其移到头部。如果没有找到已经写回的lsn，在buffer
					*节点找一个group整体写回，将这个子请求挂在这个请求上。可以提前挂在一个channel上。
					*第一步:将buffer队尾的已经写回的节点删除一个，为新的lsn腾出空间，这里需要修改队尾某节点的stored状态这里还需要
					*       增加，当没有可以之间删除的lsn时，需要产生新的写子请求，写回LRU最后的节点。
					*第二步:将新的lsn加到所述的buffer节点中。
					*************************************************************************************************************/	
					ssd->dram->buffer->write_miss_hit++;
					
					if(ssd->dram->buffer->buffer_sector_count>=ssd->dram->buffer->max_buffer_sector)
					{
						if (buffer_node==ssd->dram->buffer->buffer_tail)                  /*如果命中的节点是buffer中最后一个节点，交换最后两个节点*/
						{
							pt = ssd->dram->buffer->buffer_tail->LRU_link_pre;
							ssd->dram->buffer->buffer_tail->LRU_link_pre=pt->LRU_link_pre;
							ssd->dram->buffer->buffer_tail->LRU_link_pre->LRU_link_next=ssd->dram->buffer->buffer_tail;
							ssd->dram->buffer->buffer_tail->LRU_link_next=pt;
							pt->LRU_link_next=NULL;
							pt->LRU_link_pre=ssd->dram->buffer->buffer_tail;
							ssd->dram->buffer->buffer_tail=pt;
							
						}
						sub_req=NULL;
						sub_req_state=ssd->dram->buffer->buffer_tail->stored; 
						sub_req_size=size(ssd->dram->buffer->buffer_tail->stored);
						sub_req_lpn=ssd->dram->buffer->buffer_tail->group;
						sub_req=creat_sub_request(ssd,sub_req_lpn,sub_req_size,sub_req_state,req,WRITE);

						if(req!=NULL)           
						{
							
						}
						else if(req==NULL)   
						{
							sub_req->next_subs=sub->next_subs;
							sub->next_subs=sub_req;
						}

						ssd->dram->buffer->buffer_sector_count=ssd->dram->buffer->buffer_sector_count-sub_req->size;
						pt = ssd->dram->buffer->buffer_tail;	
						avlTreeDel(ssd->dram->buffer, (TREE_NODE *) pt);
							
						/************************************************************************/
						/* 改:  挂在了子请求，buffer的节点不应立即删除，						*/
						/*			需等到写回了之后才能删除									*/
						/************************************************************************/
						if(ssd->dram->buffer->buffer_head->LRU_link_next == NULL)
						{
							ssd->dram->buffer->buffer_head = NULL;
							ssd->dram->buffer->buffer_tail = NULL;
						}else{
							ssd->dram->buffer->buffer_tail=ssd->dram->buffer->buffer_tail->LRU_link_pre;
							ssd->dram->buffer->buffer_tail->LRU_link_next=NULL;
						}
						pt->LRU_link_next=NULL;
						pt->LRU_link_pre=NULL;
						AVL_TREENODE_FREE(ssd->dram->buffer, (TREE_NODE *) pt);
						pt = NULL;	
					}

					                                                                     /*第二步:将新的lsn加到所述的buffer节点中*/	
					add_flag=0x00000001<<(lsn%ssd->parameter->subpage_page);
					
					if(ssd->dram->buffer->buffer_head!=buffer_node)                      /*如果该buffer节点不在buffer的队首，需要将这个节点提到队首*/
					{				
						if(ssd->dram->buffer->buffer_tail==buffer_node)
						{					
							buffer_node->LRU_link_pre->LRU_link_next=NULL;					
							ssd->dram->buffer->buffer_tail=buffer_node->LRU_link_pre;
						}			
						else						
						{			
							buffer_node->LRU_link_pre->LRU_link_next=buffer_node->LRU_link_next;						
							buffer_node->LRU_link_next->LRU_link_pre=buffer_node->LRU_link_pre;								
						}								
						buffer_node->LRU_link_next=ssd->dram->buffer->buffer_head;			
						ssd->dram->buffer->buffer_head->LRU_link_pre=buffer_node;
						buffer_node->LRU_link_pre=NULL;	
						ssd->dram->buffer->buffer_head=buffer_node;							
					}					
					buffer_node->stored=buffer_node->stored|add_flag;		
					buffer_node->dirty_clean=buffer_node->dirty_clean|add_flag;	
					ssd->dram->buffer->buffer_sector_count++;
				}			

			}
		}
	}

	return ssd;
}

/**************************************************************************************
*º¯ÊýµÄ¹¦ÄÜÊÇÑ°ÕÒ»îÔ¾¿ì£¬Ó¦ÎªÃ¿¸öplaneÖÐ¶¼Ö»ÓÐÒ»¸ö»îÔ¾¿é£¬Ö»ÓÐÕâ¸ö»îÔ¾¿éÖÐ²ÅÄÜ½øÐÐ²Ù×÷
***************************************************************************************/
Status  find_active_block(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane)
{
	unsigned int active_block;
	unsigned int free_page_num=0;
	unsigned int count=0;
	
	active_block=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
	free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	//last_write_page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
	while((free_page_num==0)&&(count<ssd->parameter->block_plane))
	{
		active_block=(active_block+1)%ssd->parameter->block_plane;	
		free_page_num=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num;
		count++;
	}
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block=active_block;
	if(count<ssd->parameter->block_plane)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}
}

/*************************************************
*Õâ¸öº¯ÊýµÄ¹¦ÄÜ¾ÍÊÇÒ»¸öÄ£ÄâÒ»¸öÊµÊµÔÚÔÚµÄÐ´²Ù×÷
*¾ÍÊÇ¸ü¸ÄÕâ¸öpageµÄÏà¹Ø²ÎÊý£¬ÒÔ¼°Õû¸össdµÄÍ³¼Æ²ÎÊý
**************************************************/
Status write_page(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int active_block,unsigned int *ppn)
{
	int last_write_page=0;
	last_write_page=++(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);	
	if(last_write_page>=(int)(ssd->parameter->page_block))
	{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page=0;
		printf("error! the last write page larger than 64!!\n");
		return ERROR;
	}
		
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--; 
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[last_write_page].written_count++;
	ssd->write_flash_count++;    
	*ppn=find_ppn(ssd,channel,chip,die,plane,active_block,last_write_page);

	return SUCCESS;
}

/**********************************************
*Õâ¸öº¯ÊýµÄ¹¦ÄÜÊÇ¸ù¾Ýlpn£¬size£¬state´´½¨×ÓÇëÇó
**********************************************/
struct sub_request * creat_sub_request(struct ssd_info * ssd,unsigned int lpn,int size,unsigned int state,struct request * req,unsigned int operation)
{
	struct sub_request* sub=NULL,* sub_r=NULL;
	struct channel_info * p_ch=NULL;
	struct local * loc=NULL;
	unsigned int flag=0;

	sub = (struct sub_request*)malloc(sizeof(struct sub_request));                        /*ÉêÇëÒ»¸ö×ÓÇëÇóµÄ½á¹¹*/
	alloc_assert(sub,"sub_request");
	memset(sub,0, sizeof(struct sub_request));

	if(sub==NULL)
	{
		return NULL;
	}
	sub->location=NULL;
	sub->next_node=NULL;
	sub->next_subs=NULL;
	sub->update=NULL;
	
	if(req!=NULL)
	{
		sub->next_subs = req->subs;
		req->subs = sub;
	}
	
	/*************************************************************************************
	*ÔÚ¶Á²Ù×÷µÄÇé¿öÏÂ£¬ÓÐÒ»µã·Ç³£ÖØÒª¾ÍÊÇÒªÔ¤ÏÈÅÐ¶Ï¶Á×ÓÇëÇó¶ÓÁÐÖÐÊÇ·ñÓÐÓëÕâ¸ö×ÓÇëÇóÏàÍ¬µÄ£¬
	*ÓÐµÄ»°£¬ÐÂ×ÓÇëÇó¾Í²»±ØÔÙÖ´ÐÐÁË£¬½«ÐÂµÄ×ÓÇëÇóÖ±½Ó¸³ÎªÍê³É
	**************************************************************************************/
	if (operation == READ)
	{	
		loc = find_location(ssd,ssd->dram->map->map_entry[lpn].pn);
		sub->location=loc;
		sub->begin_time = ssd->current_time;
		sub->current_state = SR_WAIT;
		sub->current_time=MAX_INT64;
		sub->next_state = SR_R_C_A_TRANSFER;
		sub->next_state_predict_time=MAX_INT64;
		sub->lpn = lpn;
		sub->size=size;                                                               /*ÐèÒª¼ÆËã³ö¸Ã×ÓÇëÇóµÄÇëÇó´óÐ¡*/

		p_ch = &ssd->channel_head[loc->channel];	
		sub->ppn = ssd->dram->map->map_entry[lpn].pn;
		sub->operation = READ;
		sub->state=(ssd->dram->map->map_entry[lpn].state&0x7fffffff);
		sub_r=p_ch->subs_r_head;                                                      /*Ò»ÏÂ¼¸ÐÐ°üÀ¨flagÓÃÓÚÅÐ¶Ï¸Ã¶Á×ÓÇëÇó¶ÓÁÐÖÐÊÇ·ñÓÐÓëÕâ¸ö×ÓÇëÇóÏàÍ¬µÄ£¬ÓÐµÄ»°£¬½«ÐÂµÄ×ÓÇëÇóÖ±½Ó¸³ÎªÍê³É*/
		flag=0;
		while (sub_r!=NULL)
		{
			if (sub_r->ppn==sub->ppn)
			{
				flag=1;
				break;
			}
			sub_r=sub_r->next_node;
		}
		if (flag==0)
		{
			if (p_ch->subs_r_tail!=NULL)
			{
				p_ch->subs_r_tail->next_node=sub;
				p_ch->subs_r_tail=sub;
			} 
			else
			{
				p_ch->subs_r_head=sub;
				p_ch->subs_r_tail=sub;
			}
		}
		else
		{
			sub->current_state = SR_R_DATA_TRANSFER;
			sub->current_time=ssd->current_time;
			sub->next_state = SR_COMPLETE;
			sub->next_state_predict_time=ssd->current_time+1000;
			sub->complete_time=ssd->current_time+1000;
		}
	}
	/*************************************************************************************
	*Ð´ÇëÇóµÄÇé¿öÏÂ£¬¾ÍÐèÒªÀûÓÃµ½º¯Êýallocate_location(ssd ,sub)À´´¦Àí¾²Ì¬·ÖÅäºÍ¶¯Ì¬·ÖÅäÁË
	**************************************************************************************/
	else if(operation == WRITE)
	{                                
		sub->ppn=0;
		sub->operation = WRITE;
		sub->location=(struct local *)malloc(sizeof(struct local));
		alloc_assert(sub->location,"sub->location");
		memset(sub->location,0, sizeof(struct local));

		sub->current_state=SR_WAIT;
		sub->current_time=ssd->current_time;
		sub->lpn=lpn;
		sub->size=size;
		sub->state=state;
		sub->begin_time=ssd->current_time;
      
		if (allocate_location(ssd ,sub)==ERROR)
		{
			free(sub->location);
			sub->location=NULL;
			free(sub);
			sub=NULL;
			return NULL;
		}
			
	}
	else
	{
		free(sub->location);
		sub->location=NULL;
		free(sub);
		sub=NULL;
		printf("\nERROR ! Unexpected command.\n");
		return NULL;
	}
	
	return sub;
}

/******************************************************
*º¯ÊýµÄ¹¦ÄÜÊÇÔÚ¸ø³öµÄchannel£¬chip£¬dieÉÏÃæÑ°ÕÒ¶Á×ÓÇëÇó
*Õâ¸ö×ÓÇëÇóµÄppnÒªÓëÏàÓ¦µÄplaneµÄ¼Ä´æÆ÷ÀïÃæµÄppnÏà·û
*******************************************************/
struct sub_request * find_read_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die)
{
	unsigned int plane=0;
	unsigned int address_ppn=0;
	struct sub_request *sub=NULL,* p=NULL;

	for(plane=0;plane<ssd->parameter->plane_die;plane++)
	{
		address_ppn=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].add_reg_ppn;
		if(address_ppn!=-1)
		{
			sub=ssd->channel_head[channel].subs_r_head;
			if(sub->ppn==address_ppn)
			{
				if(sub->next_node==NULL)
				{
					ssd->channel_head[channel].subs_r_head=NULL;
					ssd->channel_head[channel].subs_r_tail=NULL;
				}
				ssd->channel_head[channel].subs_r_head=sub->next_node;
			}
			while((sub->ppn!=address_ppn)&&(sub->next_node!=NULL))
			{
				if(sub->next_node->ppn==address_ppn)
				{
					p=sub->next_node;
					if(p->next_node==NULL)
					{
						sub->next_node=NULL;
						ssd->channel_head[channel].subs_r_tail=sub;
					}
					else
					{
						sub->next_node=p->next_node;
					}
					sub=p;
					break;
				}
				sub=sub->next_node;
			}
			if(sub->ppn==address_ppn)
			{
				sub->next_node=NULL;
				return sub;
			}
			else 
			{
				printf("Error! Can't find the sub request.");
			}
		}
	}
	return NULL;
}

/*******************************************************************************
*º¯ÊýµÄ¹¦ÄÜÊÇÑ°ÕÒÐ´×ÓÇëÇó¡£
*·ÖÁ½ÖÖÇé¿ö1£¬ÒªÊÇÊÇÍêÈ«¶¯Ì¬·ÖÅä¾ÍÔÚssd->subs_w_head¶ÓÁÐÉÏÕÒ
*2£¬ÒªÊÇ²»ÊÇÍêÈ«¶¯Ì¬·ÖÅäÄÇÃ´¾ÍÔÚssd->channel_head[channel].subs_w_head¶ÓÁÐÉÏ²éÕÒ
********************************************************************************/
struct sub_request * find_write_sub_request(struct ssd_info * ssd, unsigned int channel)
{
	struct sub_request * sub=NULL,* p=NULL;
	if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))    /*ÊÇÍêÈ«µÄ¶¯Ì¬·ÖÅä*/
	{
		sub=ssd->subs_w_head;
		while(sub!=NULL)        							
		{
			if(sub->current_state==SR_WAIT)								
			{
				if (sub->update!=NULL)                                                      /*Èç¹ûÓÐÐèÒªÌáÇ°¶Á³öµÄÒ³*/
				{
					if ((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))   //±»¸üÐÂµÄÒ³ÒÑ¾­±»¶Á³ö
					{
						break;
					}
				} 
				else
				{
					break;
				}						
			}
			p=sub;
			sub=sub->next_node;							
		}

		if (sub==NULL)                                                                      /*Èç¹ûÃ»ÓÐÕÒµ½¿ÉÒÔ·þÎñµÄ×ÓÇëÇó£¬Ìø³öÕâ¸öforÑ­»·*/
		{
			return NULL;
		}

		if (sub!=ssd->subs_w_head)
		{
			if (sub!=ssd->subs_w_tail)
			{
				p->next_node=sub->next_node;
			}
			else
			{
				ssd->subs_w_tail=p;
				ssd->subs_w_tail->next_node=NULL;
			}
		} 
		else
		{
			if (sub->next_node!=NULL)
			{
				ssd->subs_w_head=sub->next_node;
			} 
			else
			{
				ssd->subs_w_head=NULL;
				ssd->subs_w_tail=NULL;
			}
		}
		sub->next_node=NULL;
		if (ssd->channel_head[channel].subs_w_tail!=NULL)
		{
			ssd->channel_head[channel].subs_w_tail->next_node=sub;
			ssd->channel_head[channel].subs_w_tail=sub;
		} 
		else
		{
			ssd->channel_head[channel].subs_w_tail=sub;
			ssd->channel_head[channel].subs_w_head=sub;
		}
	}
	/**********************************************************
	*³ýÁËÈ«¶¯Ì¬·ÖÅä·½Ê½£¬ÆäËû·½Ê½µÄÇëÇóÒÑ¾­·ÖÅäµ½ÌØ¶¨µÄchannel£¬
	*¾ÍÖ»ÐèÒªÔÚchannelÉÏÕÒ³ö×¼±¸·þÎñµÄ×ÓÇëÇó
	***********************************************************/
	else            
	{
		sub=ssd->channel_head[channel].subs_w_head;
		while(sub!=NULL)        						
		{
			if(sub->current_state==SR_WAIT)								
			{
				if (sub->update!=NULL)    
				{
					if ((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))   //±»¸üÐÂµÄÒ³ÒÑ¾­±»¶Á³ö
					{
						break;
					}
				} 
				else
				{
					break;
				}						
			}
			p=sub;
			sub=sub->next_node;							
		}

		if (sub==NULL)
		{
			return NULL;
		}
	}
	
	return sub;
}

/*********************************************************************************************
*×¨ÃÅÎª¶Á×ÓÇëÇó·þÎñµÄº¯Êý
*1£¬Ö»ÓÐµ±¶Á×ÓÇëÇóµÄµ±Ç°×´Ì¬ÊÇSR_R_C_A_TRANSFER
*2£¬¶Á×ÓÇëÇóµÄµ±Ç°×´Ì¬ÊÇSR_COMPLETE»òÕßÏÂÒ»×´Ì¬ÊÇSR_COMPLETE²¢ÇÒÏÂÒ»×´Ì¬µ½´ïµÄÊ±¼ä±Èµ±Ç°Ê±¼äÐ¡
**********************************************************************************************/
Status services_2_r_cmd_trans_and_complete(struct ssd_info * ssd)
{
	unsigned int i=0;
	struct sub_request * sub=NULL, * p=NULL;
	for(i=0;i<ssd->parameter->channel_number;i++)                                       /*Õâ¸öÑ­»·´¦Àí²»ÐèÒªchannelµÄÊ±¼ä(¶ÁÃüÁîÒÑ¾­µ½´ïchip£¬chipÓÉready±äÎªbusy)£¬µ±¶ÁÇëÇóÍê³ÉÊ±£¬½«Æä´ÓchannelµÄ¶ÓÁÐÖÐÈ¡³ö*/
	{
		sub=ssd->channel_head[i].subs_r_head;

		while(sub!=NULL)
		{
			if(sub->current_state==SR_R_C_A_TRANSFER)                                  /*¶ÁÃüÁî·¢ËÍÍê±Ï£¬½«¶ÔÓ¦µÄdieÖÃÎªbusy£¬Í¬Ê±ÐÞ¸ÄsubµÄ×´Ì¬; Õâ¸ö²¿·Ö×¨ÃÅ´¦Àí¶ÁÇëÇóÓÉµ±Ç°×´Ì¬Îª´«ÃüÁî±äÎªdie¿ªÊ¼busy£¬die¿ªÊ¼busy²»ÐèÒªchannelÎª¿Õ£¬ËùÒÔµ¥¶ÀÁÐ³ö*/
			{
				if(sub->next_state_predict_time<=ssd->current_time)
				{
					go_one_step(ssd, sub,NULL, SR_R_READ,NORMAL);                      /*×´Ì¬Ìø±ä´¦Àíº¯Êý*/

				}
			}
			else if((sub->current_state==SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time)))					
			{			
				if(sub!=ssd->channel_head[i].subs_r_head)                             /*if the request is completed, we delete it from read queue */							
				{		
					p->next_node=sub->next_node;						
				}			
				else					
				{	
					if (ssd->channel_head[i].subs_r_head!=ssd->channel_head[i].subs_r_tail)
					{
						ssd->channel_head[i].subs_r_head=sub->next_node;
					} 
					else
					{
						ssd->channel_head[i].subs_r_head=NULL;
						ssd->channel_head[i].subs_r_tail=NULL;
					}							
				}			
			}
			p=sub;
			sub=sub->next_node;
		}
	}
	
	return SUCCESS;
}

/**************************************************************************
*Õâ¸öº¯ÊýÒ²ÊÇÖ»´¦Àí¶Á×ÓÇëÇó£¬´¦Àíchipµ±Ç°×´Ì¬ÊÇCHIP_WAIT£¬
*»òÕßÏÂÒ»¸ö×´Ì¬ÊÇCHIP_DATA_TRANSFER²¢ÇÒÏÂÒ»×´Ì¬µÄÔ¤¼ÆÊ±¼äÐ¡ÓÚµ±Ç°Ê±¼äµÄchip
***************************************************************************/
Status services_2_r_data_trans(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag)
{
	int chip=0;
	unsigned int die=0,plane=0,address_ppn=0,die1=0;
	struct sub_request * sub=NULL, * p=NULL,*sub1=NULL;
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
	for(chip=0;chip<ssd->channel_head[channel].chip;chip++)           			    
	{				       		      
			if((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_WAIT)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_DATA_TRANSFER)&&
				(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time)))					       					
			{
				for(die=0;die<ssd->parameter->die_chip;die++)
				{
					sub=find_read_sub_request(ssd,channel,chip,die);                   /*ÔÚchannel,chip,dieÖÐÕÒµ½¶Á×ÓÇëÇó*/
					if(sub!=NULL)
					{
						break;
					}
				}

				if(sub==NULL)
				{
					continue;
				}
				
				/**************************************************************************************
				*Èç¹ûssdÖ§³Ö¸ß¼¶ÃüÁî£¬ÄÇÃ»ÎÒÃÇ¿ÉÒÔÒ»Æð´¦ÀíÖ§³ÖAD_TWOPLANE_READ£¬AD_INTERLEAVEµÄ¶Á×ÓÇëÇó
				*1£¬ÓÐ¿ÉÄÜ²úÉúÁËtwo plane²Ù×÷£¬ÔÚÕâÖÖÇé¿öÏÂ£¬½«Í¬Ò»¸ödieÉÏµÄÁ½¸öplaneµÄÊý¾ÝÒÀ´Î´«³ö
				*2£¬ÓÐ¿ÉÄÜ²úÉúÁËinterleave²Ù×÷£¬ÔÚÕâÖÖÇé¿öÏÂ£¬½«²»Í¬dieÉÏµÄÁ½¸öplaneµÄÊý¾ÝÒÀ´Î´«³ö
				***************************************************************************************/
				if(((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)==AD_TWOPLANE_READ)||((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
				{
					if ((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)==AD_TWOPLANE_READ)     /*ÓÐ¿ÉÄÜ²úÉúÁËtwo plane²Ù×÷£¬ÔÚÕâÖÖÇé¿öÏÂ£¬½«Í¬Ò»¸ödieÉÏµÄÁ½¸öplaneµÄÊý¾ÝÒÀ´Î´«³ö*/
					{
						sub_twoplane_one=sub;
						sub_twoplane_two=NULL;                                                      
						                                                                            /*ÎªÁË±£Ö¤ÕÒµ½µÄsub_twoplane_twoÓësub_twoplane_one²»Í¬£¬Áîadd_reg_ppn=-1*/
						ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[sub->location->plane].add_reg_ppn=-1;
						sub_twoplane_two=find_read_sub_request(ssd,channel,chip,die);               /*ÔÚÏàÍ¬µÄchannel,chip,dieÖÐÑ°ÕÒÁíÍâÒ»¸ö¶Á×ÓÇëÇó*/
						
						/******************************************************
						*Èç¹ûÕÒµ½ÁËÄÇÃ´¾ÍÖ´ÐÐTWO_PLANEµÄ×´Ì¬×ª»»º¯Êýgo_one_step
						*Èç¹ûÃ»ÕÒµ½ÄÇÃ´¾ÍÖ´ÐÐÆÕÍ¨ÃüÁîµÄ×´Ì¬×ª»»º¯Êýgo_one_step
						******************************************************/
						if (sub_twoplane_two==NULL)
						{
							go_one_step(ssd, sub_twoplane_one,NULL, SR_R_DATA_TRANSFER,NORMAL);   
							*change_current_time_flag=0;   
							*channel_busy_flag=1;

						}
						else
						{
							go_one_step(ssd, sub_twoplane_one,sub_twoplane_two, SR_R_DATA_TRANSFER,TWO_PLANE);
							*change_current_time_flag=0;  
							*channel_busy_flag=1;

						}
					} 
					else if ((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)      /*ÓÐ¿ÉÄÜ²úÉúÁËinterleave²Ù×÷£¬ÔÚÕâÖÖÇé¿öÏÂ£¬½«²»Í¬dieÉÏµÄÁ½¸öplaneµÄÊý¾ÝÒÀ´Î´«³ö*/
					{
						sub_interleave_one=sub;
						sub_interleave_two=NULL;
						ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[sub->location->plane].add_reg_ppn=-1;
						
						for(die1=0;die1<ssd->parameter->die_chip;die1++)
						{	
							if(die1!=die)
							{
								sub_interleave_two=find_read_sub_request(ssd,channel,chip,die1);    /*ÔÚÏàÍ¬µÄchannel£¬chhip²»Í¬µÄdieÉÏÃæÕÒÁíÍâÒ»¸ö¶Á×ÓÇëÇó*/
								if(sub_interleave_two!=NULL)
								{
									break;
								}
							}
						}	
						if (sub_interleave_two==NULL)
						{
							go_one_step(ssd, sub_interleave_one,NULL, SR_R_DATA_TRANSFER,NORMAL);

							*change_current_time_flag=0;  
							*channel_busy_flag=1;

						}
						else
						{
							go_one_step(ssd, sub_twoplane_one,sub_interleave_two, SR_R_DATA_TRANSFER,INTERLEAVE);
												
							*change_current_time_flag=0;   
							*channel_busy_flag=1;
							
						}
					}
				}
				else                                                                                 /*Èç¹ûssd²»Ö§³Ö¸ß¼¶ÃüÁîÄÇÃ´¾ÍÖ´ÐÐÒ»¸öÒ»¸öµÄÖ´ÐÐ¶Á×ÓÇëÇó*/
				{
											
					go_one_step(ssd, sub,NULL, SR_R_DATA_TRANSFER,NORMAL);
					*change_current_time_flag=0;  
					*channel_busy_flag=1;
					
				}
				break;
			}		
			
		if(*channel_busy_flag==1)
		{
			break;
		}
	}		
	return SUCCESS;
}


/******************************************************
*Õâ¸öº¯ÊýÒ²ÊÇÖ»·þÎñ¶Á×ÓÇëÇó£¬²¢ÇÒ´¦ÓÚµÈ´ý×´Ì¬µÄ¶Á×ÓÇëÇó
*******************************************************/
int services_2_r_wait(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag)
{
	unsigned int plane=0,address_ppn=0;
	struct sub_request * sub=NULL, * p=NULL;
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
	
	
	sub=ssd->channel_head[channel].subs_r_head;

	
	if ((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)==AD_TWOPLANE_READ)         /*to find whether there are two sub request can be served by two plane operation*/
	{
		sub_twoplane_one=NULL;
		sub_twoplane_two=NULL;                                                         
		                                                                                /*Ñ°ÕÒÄÜÖ´ÐÐtwo_planeµÄÁ½¸ö¶Á×ÓÇëÇó*/
		find_interleave_twoplane_sub_request(ssd,channel,sub_twoplane_one,sub_twoplane_two,TWO_PLANE);

		if (sub_twoplane_two!=NULL)                                                     /*¿ÉÒÔÖ´ÐÐtwo plane read ²Ù×÷*/
		{
			go_one_step(ssd, sub_twoplane_one,sub_twoplane_two, SR_R_C_A_TRANSFER,TWO_PLANE);
						
			*change_current_time_flag=0;
			*channel_busy_flag=1;                                                       /*ÒÑ¾­Õ¼ÓÃÁËÕâ¸öÖÜÆÚµÄ×ÜÏß£¬²»ÓÃÖ´ÐÐdieÖÐÊý¾ÝµÄ»Ø´«*/
		} 
		else if((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)       /*Ã»ÓÐÂú×ãÌõ¼þµÄÁ½¸öpage£¬£¬²¢ÇÒÃ»ÓÐinterleave readÃüÁîÊ±£¬Ö»ÄÜÖ´ÐÐµ¥¸öpageµÄ¶Á*/
		{
			while(sub!=NULL)                                                            /*if there are read requests in queue, send one of them to target die*/			
			{		
				if(sub->current_state==SR_WAIT)									
				{	                                                                    /*×¢ÒâÏÂ¸öÕâ¸öÅÐ¶ÏÌõ¼þÓëservices_2_r_data_transÖÐÅÐ¶ÏÌõ¼þµÄ²»Í¬
																						*/
					if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
						(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
					{	
						go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER,NORMAL);
									
						*change_current_time_flag=0;
						*channel_busy_flag=1;                                           /*ÒÑ¾­Õ¼ÓÃÁËÕâ¸öÖÜÆÚµÄ×ÜÏß£¬²»ÓÃÖ´ÐÐdieÖÐÊý¾ÝµÄ»Ø´«*/
						break;										
					}	
					else
					{
						                                                                /*ÒòÎªdieµÄbusyµ¼ÖÂµÄ×èÈû*/
					}
				}						
				sub=sub->next_node;								
			}
		}
	} 
	if ((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)               /*to find whether there are two sub request can be served by INTERLEAVE operation*/
	{
		sub_interleave_one=NULL;
		sub_interleave_two=NULL;
		find_interleave_twoplane_sub_request(ssd,channel,sub_interleave_one,sub_interleave_two,INTERLEAVE);
		
		if (sub_interleave_two!=NULL)                                                  /*¿ÉÒÔÖ´ÐÐinterleave read ²Ù×÷*/
		{

			go_one_step(ssd, sub_interleave_one,sub_interleave_two, SR_R_C_A_TRANSFER,INTERLEAVE);
						
			*change_current_time_flag=0;
			*channel_busy_flag=1;                                                      /*ÒÑ¾­Õ¼ÓÃÁËÕâ¸öÖÜÆÚµÄ×ÜÏß£¬²»ÓÃÖ´ÐÐdieÖÐÊý¾ÝµÄ»Ø´«*/
		} 
		else                                                                           /*Ã»ÓÐÂú×ãÌõ¼þµÄÁ½¸öpage£¬Ö»ÄÜÖ´ÐÐµ¥¸öpageµÄ¶Á*/
		{
			while(sub!=NULL)                                                           /*if there are read requests in queue, send one of them to target die*/			
			{		
				if(sub->current_state==SR_WAIT)									
				{	
					if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
						(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
					{	

						go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER,NORMAL);
									
						*change_current_time_flag=0;
						*channel_busy_flag=1;                                          /*ÒÑ¾­Õ¼ÓÃÁËÕâ¸öÖÜÆÚµÄ×ÜÏß£¬²»ÓÃÖ´ÐÐdieÖÐÊý¾ÝµÄ»Ø´«*/
						break;										
					}	
					else
					{
						                                                               /*ÒòÎªdieµÄbusyµ¼ÖÂµÄ×èÈû*/
					}
				}						
				sub=sub->next_node;								
			}
		}
	}

	/*******************************
	*ssd²»ÄÜÖ´ÐÐÖ´ÐÐ¸ß¼¶ÃüÁîµÄÇé¿öÏÂ
	*******************************/
	if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE_READ)!=AD_TWOPLANE_READ))
	{
		while(sub!=NULL)                                                               /*if there are read requests in queue, send one of them to target chip*/			
		{		
			if(sub->current_state==SR_WAIT)									
			{	                                                                       
				if((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state==CHIP_IDLE)&&
					(ssd->channel_head[sub->location->channel].chip_head[sub->location->chip].next_state_predict_time<=ssd->current_time)))												
				{	

					go_one_step(ssd, sub,NULL, SR_R_C_A_TRANSFER,NORMAL);
							
					*change_current_time_flag=0;
					*channel_busy_flag=1;                                              /*ÒÑ¾­Õ¼ÓÃÁËÕâ¸öÖÜÆÚµÄ×ÜÏß£¬²»ÓÃÖ´ÐÐdieÖÐÊý¾ÝµÄ»Ø´«*/
					break;										
				}	
				else
				{
					                                                                   /*ÒòÎªdieµÄbusyµ¼ÖÂµÄ×èÈû*/
				}
			}						
			sub=sub->next_node;								
		}
	}

	return SUCCESS;
}

/*********************************************************************
*µ±Ò»¸öÐ´×ÓÇëÇó´¦ÀíÍêºó£¬Òª´ÓÇëÇó¶ÓÁÐÉÏÉ¾³ý£¬Õâ¸öº¯Êý¾ÍÊÇÖ´ÐÐÕâ¸ö¹¦ÄÜ¡£
**********************************************************************/
int delete_w_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub )
{
	struct sub_request * p=NULL;
	if (sub==ssd->channel_head[channel].subs_w_head)                                   /*½«Õâ¸ö×ÓÇëÇó´Óchannel¶ÓÁÐÖÐÉ¾³ý*/
	{
		if (ssd->channel_head[channel].subs_w_head!=ssd->channel_head[channel].subs_w_tail)
		{
			ssd->channel_head[channel].subs_w_head=sub->next_node;
		} 
		else
		{
			ssd->channel_head[channel].subs_w_head=NULL;
			ssd->channel_head[channel].subs_w_tail=NULL;
		}
	}
	else
	{
		p=ssd->channel_head[channel].subs_w_head;
		while(p->next_node !=sub)
		{
			p=p->next_node;
		}

		if (sub->next_node!=NULL)
		{
			p->next_node=sub->next_node;
		} 
		else
		{
			p->next_node=NULL;
			ssd->channel_head[channel].subs_w_tail=p;
		}
	}
	
	return SUCCESS;	
}

/*
*º¯ÊýµÄ¹¦ÄÜ¾ÍÊÇÖ´ÐÐcopybackÃüÁîµÄ¹¦ÄÜ£¬
*/
Status copy_back(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die,struct sub_request * sub)
{
	int old_ppn=-1, new_ppn=-1;
	long long time=0;
	if (ssd->parameter->greed_CB_ad==1)                                               /*ÔÊÐíÌ°À·Ê¹ÓÃcopyback¸ß¼¶ÃüÁî*/
	{
		old_ppn=-1;
		if (ssd->dram->map->map_entry[sub->lpn].state!=0)                             /*ËµÃ÷Õâ¸öÂß¼­Ò³Ö®Ç°ÓÐÐ´¹ý£¬ÐèÒªÊ¹ÓÃcopyback+random inputÃüÁî£¬·ñÔòÖ±½ÓÐ´ÏÂÈ¥¼´¿É*/
		{
			if ((sub->state&ssd->dram->map->map_entry[sub->lpn].state)==ssd->dram->map->map_entry[sub->lpn].state)       
			{
				sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;	
			} 
			else
			{
				sub->next_state_predict_time=ssd->current_time+19*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				ssd->copy_back_count++;
				ssd->read_count++;
				ssd->update_read_count++;
				old_ppn=ssd->dram->map->map_entry[sub->lpn].pn;                       /*¼ÇÂ¼Ô­À´µÄÎïÀíÒ³£¬ÓÃÓÚÔÚcopybackÊ±£¬ÅÐ¶ÏÊÇ·ñÂú×ãÍ¬ÎªÆæµØÖ·»òÕßÅ¼µØÖ·*/
			}															
		} 
		else
		{
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		}
		sub->complete_time=sub->next_state_predict_time;		
		time=sub->complete_time;

		get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);

		if (old_ppn!=-1)                                                              /*²ÉÓÃÁËcopyback²Ù×÷£¬ÐèÒªÅÐ¶ÏÊÇ·ñÂú×ãÁËÆæÅ¼µØÖ·µÄÏÞÖÆ*/
		{
			new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
			while (old_ppn%2!=new_ppn%2)                                              /*Ã»ÓÐÂú×ãÆæÅ¼µØÖ·ÏÞÖÆ£¬ÐèÒªÔÙÍùÏÂÕÒÒ»Ò³*/
			{
				get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
				ssd->program_count--;
				ssd->write_flash_count--;
				ssd->waste_page_count++;
				new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
			}
		}
	} 
	else                                                                              /*²»ÄÜÌ°À·µÄÊ¹ÓÃcopyback¸ß¼¶ÃüÁî*/
	{
		if (ssd->dram->map->map_entry[sub->lpn].state!=0)
		{
			if ((sub->state&ssd->dram->map->map_entry[sub->lpn].state)==ssd->dram->map->map_entry[sub->lpn].state)        
			{
				sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
			} 
			else
			{
				old_ppn=ssd->dram->map->map_entry[sub->lpn].pn;                       /*¼ÇÂ¼Ô­À´µÄÎïÀíÒ³£¬ÓÃÓÚÔÚcopybackÊ±£¬ÅÐ¶ÏÊÇ·ñÂú×ãÍ¬ÎªÆæµØÖ·»òÕßÅ¼µØÖ·*/
				get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
				new_ppn=ssd->dram->map->map_entry[sub->lpn].pn;
				if (old_ppn%2==new_ppn%2)
				{
					ssd->copy_back_count++;
					sub->next_state_predict_time=ssd->current_time+19*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				} 
				else
				{
					sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(size(ssd->dram->map->map_entry[sub->lpn].state))*ssd->parameter->time_characteristics.tRC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				}
				ssd->read_count++;
				ssd->update_read_count++;
			}
		} 
		else
		{
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
			get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);
		}
		sub->complete_time=sub->next_state_predict_time;		
		time=sub->complete_time;
	}
    
	/****************************************************************
	*Ö´ÐÐcopyback¸ß¼¶ÃüÁîÊ±£¬ÐèÒªÐÞ¸Ächannel£¬chipµÄ×´Ì¬£¬ÒÔ¼°Ê±¼äµÈ
	*****************************************************************/
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
	ssd->channel_head[channel].next_state_predict_time=time;

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
	ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG;
	
	return SUCCESS;
}

/*****************
*¾²Ì¬Ð´²Ù×÷µÄÊµÏÖ
******************/
Status static_write(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub)
{
	long long time=0;
	if (ssd->dram->map->map_entry[sub->lpn].state!=0)                                    /*ËµÃ÷Õâ¸öÂß¼­Ò³Ö®Ç°ÓÐÐ´¹ý£¬ÐèÒªÊ¹ÓÃÏÈ¶Á³öÀ´£¬ÔÙÐ´ÏÂÈ¥£¬·ñÔòÖ±½ÓÐ´ÏÂÈ¥¼´¿É*/
	{
		if ((sub->state&ssd->dram->map->map_entry[sub->lpn].state)==ssd->dram->map->map_entry[sub->lpn].state)   /*¿ÉÒÔ¸²¸Ç*/
		{
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		} 
		else
		{
			sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(size((ssd->dram->map->map_entry[sub->lpn].state^sub->state)))*ssd->parameter->time_characteristics.tRC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
			ssd->read_count++;
			ssd->update_read_count++;
		}
	} 
	else
	{
		sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
	}
	sub->complete_time=sub->next_state_predict_time;		
	time=sub->complete_time;

	get_ppn(ssd,sub->location->channel,sub->location->chip,sub->location->die,sub->location->plane,sub);

    /****************************************************************
	*Ö´ÐÐcopyback¸ß¼¶ÃüÁîÊ±£¬ÐèÒªÐÞ¸Ächannel£¬chipµÄ×´Ì¬£¬ÒÔ¼°Ê±¼äµÈ
	*****************************************************************/
	ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
	ssd->channel_head[channel].current_time=ssd->current_time;										
	ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
	ssd->channel_head[channel].next_state_predict_time=time;

	ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
	ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
	ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
	ssd->channel_head[channel].chip_head[chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG;
	
	return SUCCESS;
}

/********************
Ð´×ÓÇëÇóµÄ´¦Àíº¯Êý
*********************/
Status services_2_write(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag)
{
	int j=0,chip=0;
	unsigned int k=0;
	unsigned int  old_ppn=0,new_ppn=0;
	unsigned int chip_token=0,die_token=0,plane_token=0,address_ppn=0;
	unsigned int  die=0,plane=0;
	long long time=0;
	struct sub_request * sub=NULL, * p=NULL;
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
    
	/************************************************************************************************************************
	*Ð´×ÓÇëÇó¹ÒÔÚÁ½¸öµØ·½Ò»¸öÊÇchannel_head[channel].subs_w_head£¬ÁíÍâÒ»¸öÊÇssd->subs_w_head£¬ËùÒÔÒª±£Ö¤ÖÁÉÙÓÐÒ»¸ö¶ÓÁÐ²»Îª¿Õ
	*Í¬Ê±×ÓÇëÇóµÄ´¦Àí»¹·ÖÎª¶¯Ì¬·ÖÅäºÍ¾²Ì¬·ÖÅä¡£
	*************************************************************************************************************************/
	if((ssd->channel_head[channel].subs_w_head!=NULL)||(ssd->subs_w_head!=NULL))      
	{
		if (ssd->parameter->allocation_scheme==0)                                       /*¶¯Ì¬·ÖÅä*/
		{
			for(j=0;j<ssd->channel_head[channel].chip;j++)					
			{		
				if((ssd->channel_head[channel].subs_w_head==NULL)&&(ssd->subs_w_head==NULL)) 
				{
					break;
				}
				
				chip_token=ssd->channel_head[channel].token;                            /*ÁîÅÆ*/
				if (*channel_busy_flag==0)
				{
					if((ssd->channel_head[channel].chip_head[chip_token].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip_token].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip_token].next_state_predict_time<=ssd->current_time)))				
					{
							if((ssd->channel_head[channel].subs_w_head==NULL)&&(ssd->subs_w_head==NULL)) 
							{
								break;
							}
							die_token=ssd->channel_head[channel].chip_head[chip_token].token;	
							if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))       //can't use advanced commands
							{
								sub=find_write_sub_request(ssd,channel);
								if(sub==NULL)
								{
									break;
								}
								
								if(sub->current_state==SR_WAIT)
								{
									plane_token=ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token;

									get_ppn(ssd,channel,chip_token,die_token,plane_token,sub);

									ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token=(ssd->channel_head[channel].chip_head[chip_token].die_head[die_token].token+1)%ssd->parameter->plane_die;

									*change_current_time_flag=0;

									if(ssd->parameter->ad_priority2==0)
									{
										ssd->real_time_subreq--;
									}
									go_one_step(ssd,sub,NULL,SR_W_TRANSFER,NORMAL);       /*Ö´ÐÐÆÕÍ¨µÄ×´Ì¬µÄ×ª±ä¡£*/
									delete_w_sub_request(ssd,channel,sub);                /*É¾µô´¦ÀíÍêºóµÄÐ´×ÓÇëÇó*/
						
									*channel_busy_flag=1;
									/**************************************************************************
									*Ìø³öforÑ­»·Ç°£¬ÐÞ¸ÄÁîÅÆ
									*ÕâÀïµÄtokenµÄ±ä»¯ÍêÈ«È¡¾öÓÚÔÚÕâ¸öchannel chip die planeÏÂÐ´ÊÇ·ñ³É¹¦ 
									*³É¹¦ÁË¾Íbreak Ã»³É¹¦token¾ÍÒª±ä»¯Ö±µ½ÕÒµ½ÄÜÐ´³É¹¦µÄchannel chip die plane
									***************************************************************************/
									ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
									ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
									break;
								}
							} 
							else                                                          /*use advanced commands*/
							{
								if (dynamic_advanced_process(ssd,channel,chip_token)==NULL)
								{
									*channel_busy_flag=0;
								}
								else
								{
									*channel_busy_flag=1;                                 /*Ö´ÐÐÁËÒ»¸öÇëÇó£¬´«ÊäÁËÊý¾Ý£¬Õ¼ÓÃÁË×ÜÏß£¬ÐèÒªÌø³öµ½ÏÂÒ»¸öchannel*/
                                    ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
                                    ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
									break;
								}
							}	
								
						ssd->channel_head[channel].chip_head[chip_token].token=(ssd->channel_head[channel].chip_head[chip_token].token+1)%ssd->parameter->die_chip;
					}
				}
								
				ssd->channel_head[channel].token=(ssd->channel_head[channel].token+1)%ssd->parameter->chip_channel[channel];
			}
		} 
		else if(ssd->parameter->allocation_scheme==1)                                     /*¾²Ì¬·ÖÅä*/
		{
			for(chip=0;chip<ssd->channel_head[channel].chip;chip++)					
			{	
				if((ssd->channel_head[channel].chip_head[chip].current_state==CHIP_IDLE)||((ssd->channel_head[channel].chip_head[chip].next_state==CHIP_IDLE)&&(ssd->channel_head[channel].chip_head[chip].next_state_predict_time<=ssd->current_time)))				
				{		
					if(ssd->channel_head[channel].subs_w_head==NULL)
					{
						break;
					}
					if (*channel_busy_flag==0)
					{
							                                                            
							if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))     /*²»Ö´ÐÐ¸ß¼¶ÃüÁî*/
							{
								for(die=0;die<ssd->channel_head[channel].chip_head[chip].die_num;die++)				
								{	
									if(ssd->channel_head[channel].subs_w_head==NULL)
									{
										break;
									}
									sub=ssd->channel_head[channel].subs_w_head;
									while (sub!=NULL)
									{
										if ((sub->current_state==SR_WAIT)&&(sub->location->channel==channel)&&(sub->location->chip==chip)&&(sub->location->die==die))      /*¸Ã×ÓÇëÇó¾ÍÊÇµ±Ç°dieµÄÇëÇó*/
										{
											break;
										}
										sub=sub->next_node;
									}
									if (sub==NULL)
									{
										continue;
									}

									if(sub->current_state==SR_WAIT)
									{
										sub->current_time=ssd->current_time;
										sub->current_state=SR_W_TRANSFER;
										sub->next_state=SR_COMPLETE;

										if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
										{
											copy_back(ssd, channel,chip, die,sub);      /*Èç¹û¿ÉÒÔÖ´ÐÐcopyback¸ß¼¶ÃüÁî£¬ÄÇÃ´¾ÍÓÃº¯Êýcopy_back(ssd, channel,chip, die,sub)´¦ÀíÐ´×ÓÇëÇó*/
											*change_current_time_flag=0;
										} 
										else
										{
											static_write(ssd, channel,chip, die,sub);   /*²»ÄÜÖ´ÐÐcopyback¸ß¼¶ÃüÁî£¬ÄÇÃ´¾ÍÓÃstatic_write(ssd, channel,chip, die,sub)º¯ÊýÀ´´¦ÀíÐ´×ÓÇëÇó*/ 
											*change_current_time_flag=0;
										}
										
										delete_w_sub_request(ssd,channel,sub);
										*channel_busy_flag=1;
										break;
									}
								}
							} 
							else                                                        /*²»ÄÜ´¦Àí¸ß¼¶ÃüÁî*/
							{
								if (dynamic_advanced_process(ssd,channel,chip)==NULL)
								{
									*channel_busy_flag=0;
								}
								else
								{
									*channel_busy_flag=1;                               /*Ö´ÐÐÁËÒ»¸öÇëÇó£¬´«ÊäÁËÊý¾Ý£¬Õ¼ÓÃÁË×ÜÏß£¬ÐèÒªÌø³öµ½ÏÂÒ»¸öchannel*/
									break;
								}
							}	
						
					}
				}		
			}
		}			
	}
	return SUCCESS;	
}


/********************************************************
*Õâ¸öº¯ÊýµÄÖ÷Òª¹¦ÄÜÊÇÖ÷¿Ø¶Á×ÓÇëÇóºÍÐ´×ÓÇëÇóµÄ×´Ì¬±ä»¯´¦Àí
*********************************************************/

struct ssd_info *process(struct ssd_info *ssd)   
{

	/*********************************************************************************************************
	*flag_die±íÊ¾ÊÇ·ñÒòÎªdieµÄbusy£¬×èÈûÁËÊ±¼äÇ°½ø£¬-1±íÊ¾Ã»ÓÐ£¬·Ç-1±íÊ¾ÓÐ×èÈû£¬
	*flag_dieµÄÖµ±íÊ¾dieºÅ,old ppn¼ÇÂ¼ÔÚcopybackÖ®Ç°µÄÎïÀíÒ³ºÅ£¬ÓÃÓÚÅÐ¶ÏcopybackÊÇ·ñ×ñÊØÁËÆæÅ¼µØÖ·µÄÏÞÖÆ£»
	*two_plane_bit[8],two_plane_place[8]Êý×é³ÉÔ±±íÊ¾Í¬Ò»¸öchannelÉÏÃ¿¸ödieµÄÇëÇó·ÖÅäÇé¿ö£»
	*chg_cur_time_flag×÷ÎªÊÇ·ñÐèÒªµ÷Õûµ±Ç°Ê±¼äµÄ±êÖ¾Î»£¬µ±ÒòÎªchannel´¦ÓÚbusyµ¼ÖÂÇëÇó×èÈûÊ±£¬ÐèÒªµ÷Õûµ±Ç°Ê±¼ä£»
	*³õÊ¼ÈÏÎªÐèÒªµ÷Õû£¬ÖÃÎª1£¬µ±ÈÎºÎÒ»¸öchannel´¦ÀíÁË´«ËÍÃüÁî»òÕßÊý¾ÝÊ±£¬Õâ¸öÖµÖÃÎª0£¬±íÊ¾²»ÐèÒªµ÷Õû£»
	**********************************************************************************************************/
	int old_ppn=-1,flag_die=-1; 
	unsigned int i,chan,random_num;     
	unsigned int flag=0,new_write=0,chg_cur_time_flag=1,flag2=0,flag_gc=0;       
	int64_t time, channel_time=MAX_INT64;
	struct sub_request *sub;          

#ifdef DEBUG
	printf("enter process,  current time:%lld\n",ssd->current_time);
#endif

	/*********************************************************
	*ÅÐ¶ÏÊÇ·ñÓÐ¶ÁÐ´×ÓÇëÇó£¬Èç¹ûÓÐÄÇÃ´flagÁîÎª0£¬Ã»ÓÐflag¾ÍÎª1
	*µ±flagÎª1Ê±£¬ÈôssdÖÐÓÐgc²Ù×÷ÕâÊ±¾Í¿ÉÒÔÖ´ÐÐgc²Ù×÷
	**********************************************************/
	for(i=0;i<ssd->parameter->channel_number;i++)
	{          
		if((ssd->channel_head[i].subs_r_head==NULL)&&(ssd->channel_head[i].subs_w_head==NULL)&&(ssd->subs_w_head==NULL))
		{
			flag=1;
		}
		else
		{
			flag=0;
			break;
		}
	}
	if(flag==1)
	{
		ssd->flag=1;                                                                
		if (ssd->gc_request>0)                                                            /*SSDÖÐÓÐgc²Ù×÷µÄÇëÇó*/
		{
			gc(ssd,0,1);                                                                  /*Õâ¸ögcÒªÇóËùÓÐchannel¶¼±ØÐë±éÀúµ½*/
		}
		return ssd;
	}
	else
	{
		ssd->flag=0;
	}
		
	time = ssd->current_time;
	services_2_r_cmd_trans_and_complete(ssd);                                            /*´¦Àíµ±Ç°×´Ì¬ÊÇSR_R_C_A_TRANSFER»òÕßµ±Ç°×´Ì¬ÊÇSR_COMPLETE£¬»òÕßÏÂÒ»×´Ì¬ÊÇSR_COMPLETE²¢ÇÒÏÂÒ»×´Ì¬Ô¤¼ÆÊ±¼äÐ¡ÓÚµ±Ç°×´Ì¬Ê±¼ä*/

	random_num=ssd->program_count%ssd->parameter->channel_number;                        /*²úÉúÒ»¸öËæ»úÊý£¬±£Ö¤Ã¿´Î´Ó²»Í¬µÄchannel¿ªÊ¼²éÑ¯*/

	/*****************************************
	*Ñ­»·´¦ÀíËùÓÐchannelÉÏµÄ¶ÁÐ´×ÓÇëÇó
	*·¢¶ÁÇëÇóÃüÁî£¬´«¶ÁÐ´Êý¾Ý£¬¶¼ÐèÒªÕ¼ÓÃ×ÜÏß£¬
	******************************************/
	for(chan=0;chan<ssd->parameter->channel_number;chan++)	     
	{
		i=(random_num+chan)%ssd->parameter->channel_number;
		flag=0;
		flag_gc=0;                                                                       /*Ã¿´Î½øÈëchannelÊ±£¬½«gcµÄ±êÖ¾Î»ÖÃÎª0£¬Ä¬ÈÏÈÏÎªÃ»ÓÐ½øÐÐgc²Ù×÷*/
		if((ssd->channel_head[i].current_state==CHANNEL_IDLE)||(ssd->channel_head[i].next_state==CHANNEL_IDLE&&ssd->channel_head[i].next_state_predict_time<=ssd->current_time))		
		{   
			if (ssd->gc_request>0)                                                       /*ÓÐgc²Ù×÷£¬ÐèÒª½øÐÐÒ»¶¨µÄÅÐ¶Ï*/
			{
				if (ssd->channel_head[i].gc_command!=NULL)
				{
					flag_gc=gc(ssd,i,0);                                                 /*gcº¯Êý·µ»ØÒ»¸öÖµ£¬±íÊ¾ÊÇ·ñÖ´ÐÐÁËgc²Ù×÷£¬Èç¹ûÖ´ÐÐÁËgc²Ù×÷£¬Õâ¸öchannelÔÚÕâ¸öÊ±¿Ì²»ÄÜ·þÎñÆäËûµÄÇëÇó*/
				}
				if (flag_gc==1)                                                          /*Ö´ÐÐ¹ýgc²Ù×÷£¬ÐèÒªÌø³ö´Ë´ÎÑ­»·*/
				{
					continue;
				}
			}

			sub=ssd->channel_head[i].subs_r_head;                                        /*ÏÈ´¦Àí¶ÁÇëÇó*/
			services_2_r_wait(ssd,i,&flag,&chg_cur_time_flag);                           /*´¦Àí´¦ÓÚµÈ´ý×´Ì¬µÄ¶Á×ÓÇëÇó*/
		
			if((flag==0)&&(ssd->channel_head[i].subs_r_head!=NULL))                      /*if there are no new read request and data is ready in some dies, send these data to controller and response this request*/		
			{		     
				services_2_r_data_trans(ssd,i,&flag,&chg_cur_time_flag);                    
						
			}
			if(flag==0)                                                                  /*if there are no read request to take channel, we can serve write requests*/ 		
			{	
				services_2_write(ssd,i,&flag,&chg_cur_time_flag);
				
			}	
		}	
	}

	return ssd;
}

/****************************************************************************************************************************
*µ±ssdÖ§³Ö¸ß¼¶ÃüÁîÊ±£¬Õâ¸öº¯ÊýµÄ×÷ÓÃ¾ÍÊÇ´¦Àí¸ß¼¶ÃüÁîµÄÐ´×ÓÇëÇó
*¸ù¾ÝÇëÇóµÄ¸öÊý£¬¾ö¶¨Ñ¡ÔñÄÄÖÖ¸ß¼¶ÃüÁî£¨Õâ¸öº¯ÊýÖ»´¦ÀíÐ´ÇëÇó£¬¶ÁÇëÇóÒÑ¾­·ÖÅäµ½Ã¿¸öchannel£¬ËùÒÔÔÚÖ´ÐÐÊ±Ö®¼ä½øÐÐÑ¡È¡ÏàÓ¦µÄÃüÁî£©
*****************************************************************************************************************************/
struct ssd_info *dynamic_advanced_process(struct ssd_info *ssd,unsigned int channel,unsigned int chip)         
{
	unsigned int die=0,plane=0;
	unsigned int subs_count=0;
	int flag;
	unsigned int gate;                                                                    /*record the max subrequest that can be executed in the same channel. it will be used when channel-level priority order is highest and allocation scheme is full dynamic allocation*/
	unsigned int plane_place;                                                             /*record which plane has sub request in static allocation*/
	struct sub_request *sub=NULL,*p=NULL,*sub0=NULL,*sub1=NULL,*sub2=NULL,*sub3=NULL,*sub0_rw=NULL,*sub1_rw=NULL,*sub2_rw=NULL,*sub3_rw=NULL;
	struct sub_request ** subs=NULL;
	unsigned int max_sub_num=0;
	unsigned int die_token=0,plane_token=0;
	unsigned int * plane_bits=NULL;
	unsigned int interleaver_count=0;
	
	unsigned int mask=0x00000001;
	unsigned int i=0,j=0;
	
	max_sub_num=(ssd->parameter->die_chip)*(ssd->parameter->plane_die);
	gate=max_sub_num;
	subs=(struct sub_request **)malloc(max_sub_num*sizeof(struct sub_request *));
	alloc_assert(subs,"sub_request");
	
	for(i=0;i<max_sub_num;i++)
	{
		subs[i]=NULL;
	}
	
	if((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0)&&(ssd->parameter->ad_priority2==0))
	{
		gate=ssd->real_time_subreq/ssd->parameter->channel_number;

		if(gate==0)
		{
			gate=1;
		}
		else
		{
			if(ssd->real_time_subreq%ssd->parameter->channel_number!=0)
			{
				gate++;
			}
		}
	}

	if ((ssd->parameter->allocation_scheme==0))                                           /*È«¶¯Ì¬·ÖÅä£¬ÐèÒª´Óssd->subs_w_headÉÏÑ¡È¡µÈ´ý·þÎñµÄ×ÓÇëÇó*/
	{
		if(ssd->parameter->dynamic_allocation==0)
		{
			sub=ssd->subs_w_head;
		}
		else
		{
			sub=ssd->channel_head[channel].subs_w_head;
		}
		
		subs_count=0;
		
		while ((sub!=NULL)&&(subs_count<max_sub_num)&&(subs_count<gate))
		{
			if(sub->current_state==SR_WAIT)								
			{
				if ((sub->update==NULL)||((sub->update!=NULL)&&((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))))    //Ã»ÓÐÐèÒªÌáÇ°¶Á³öµÄÒ³
				{
					subs[subs_count]=sub;
					subs_count++;
				}						
			}
			
			p=sub;
			sub=sub->next_node;	
		}

		if (subs_count==0)                                                               /*Ã»ÓÐÇëÇó¿ÉÒÔ·þÎñ£¬·µ»ØNULL*/
		{
			for(i=0;i<max_sub_num;i++)
			{
				subs[i]=NULL;
			}
			free(subs);

			subs=NULL;
			free(plane_bits);
			return NULL;
		}
		if(subs_count>=2)
		{
		    /*********************************************
			*two plane,interleave¶¼¿ÉÒÔÊ¹ÓÃ
			*ÔÚÕâ¸öchannelÉÏ£¬Ñ¡ÓÃinterleave_two_planeÖ´ÐÐ
			**********************************************/
			if (((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))     
			{                                                                        
				get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE_TWO_PLANE); 
			}
			else if (((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE))
			{
				if(subs_count>ssd->parameter->plane_die)
				{	
					for(i=ssd->parameter->plane_die;i<subs_count;i++)
					{
						subs[i]=NULL;
					}
					subs_count=ssd->parameter->plane_die;
				}
				get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,TWO_PLANE);
			}
			else if (((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE)&&((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE))
			{
				
				if(subs_count>ssd->parameter->die_chip)
				{	
					for(i=ssd->parameter->die_chip;i<subs_count;i++)
					{
						subs[i]=NULL;
					}
					subs_count=ssd->parameter->die_chip;
				}
				get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE);
			}
			else
			{
				for(i=1;i<subs_count;i++)
				{
					subs[i]=NULL;
				}
				subs_count=1;
				get_ppn_for_normal_command(ssd,channel,chip,subs[0]);
			}
			
		}//if(subs_count>=2)
		else if(subs_count==1)     //only one request
		{
			get_ppn_for_normal_command(ssd,channel,chip,subs[0]);
		}
		
	}//if ((ssd->parameter->allocation_scheme==0)) 
	else                                                                                  /*¾²Ì¬·ÖÅä·½Ê½£¬Ö»Ðè´ÓÕâ¸öÌØ¶¨µÄchannelÉÏÑ¡È¡µÈ´ý·þÎñµÄ×ÓÇëÇó*/
	{
		                                                                                  /*ÔÚ¾²Ì¬·ÖÅä·½Ê½ÖÐ£¬¸ù¾ÝchannelÉÏµÄÇëÇóÂäÔÚÍ¬Ò»¸ödieÉÏµÄÄÇÐ©planeÀ´È·¶¨Ê¹ÓÃÊ²Ã´ÃüÁî*/
		
			sub=ssd->channel_head[channel].subs_w_head;
			plane_bits=(unsigned int * )malloc((ssd->parameter->die_chip)*sizeof(unsigned int));
			alloc_assert(plane_bits,"plane_bits");
			memset(plane_bits,0, (ssd->parameter->die_chip)*sizeof(unsigned int));

			for(i=0;i<ssd->parameter->die_chip;i++)
			{
				plane_bits[i]=0x00000000;
			}
			subs_count=0;
			
			while ((sub!=NULL)&&(subs_count<max_sub_num))
			{
				if(sub->current_state==SR_WAIT)								
				{
					if ((sub->update==NULL)||((sub->update!=NULL)&&((sub->update->current_state==SR_COMPLETE)||((sub->update->next_state==SR_COMPLETE)&&(sub->update->next_state_predict_time<=ssd->current_time)))))
					{
						if (sub->location->chip==chip)
						{
							plane_place=0x00000001<<(sub->location->plane);
	
							if ((plane_bits[sub->location->die]&plane_place)!=plane_place)      //we have not add sub request to this plane
							{
								subs[sub->location->die*ssd->parameter->plane_die+sub->location->plane]=sub;
								subs_count++;
								plane_bits[sub->location->die]=(plane_bits[sub->location->die]|plane_place);
							}
						}
					}						
				}
				sub=sub->next_node;	
			}//while ((sub!=NULL)&&(subs_count<max_sub_num))

			if (subs_count==0)                                                            /*Ã»ÓÐÇëÇó¿ÉÒÔ·þÎñ£¬·µ»ØNULL*/
			{
				for(i=0;i<max_sub_num;i++)
				{
					subs[i]=NULL;
				}
				free(subs);
				subs=NULL;
				free(plane_bits);
				return NULL;
			}
			
			flag=0;
			if (ssd->parameter->advanced_commands!=0)
			{
				if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)        /*È«²¿¸ß¼¶ÃüÁî¶¼¿ÉÒÔÊ¹ÓÃ*/
				{
					if (subs_count>1)                                                    /*ÓÐ1¸öÒÔÉÏ¿ÉÒÔÖ±½Ó·þÎñµÄÐ´ÇëÇó*/
					{
						get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,COPY_BACK);
					} 
					else
					{
						for(i=0;i<max_sub_num;i++)
						{
							if(subs[i]!=NULL)
							{
								break;
							}
						}
						get_ppn_for_normal_command(ssd,channel,chip,subs[i]);
					}
				
				}// if ((ssd->parameter->advanced_commands&AD_COPYBACK)==AD_COPYBACK)
				else                                                                     /*²»ÄÜÖ´ÐÐcopyback*/
				{
					if (subs_count>1)                                                    /*ÓÐ1¸öÒÔÉÏ¿ÉÒÔÖ±½Ó·þÎñµÄÐ´ÇëÇó*/
					{
						if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
						{
							get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE_TWO_PLANE);
						} 
						else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))
						{
							for(die=0;die<ssd->parameter->die_chip;die++)
							{
								if(plane_bits[die]!=0x00000000)
								{
									for(i=0;i<ssd->parameter->plane_die;i++)
									{
										plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
										ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane_token+1)%ssd->parameter->plane_die;
										mask=0x00000001<<plane_token;
										if((plane_bits[die]&mask)==mask)
										{
											plane_bits[die]=mask;
											break;
										}
									}
									for(i=i+1;i<ssd->parameter->plane_die;i++)
									{
										plane=(plane_token+1)%ssd->parameter->plane_die;
										subs[die*ssd->parameter->plane_die+plane]=NULL;
										subs_count--;
									}
									interleaver_count++;
								}//if(plane_bits[die]!=0x00000000)
							}//for(die=0;die<ssd->parameter->die_chip;die++)
							if(interleaver_count>=2)
							{
								get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,INTERLEAVE);
							}
							else
							{
								for(i=0;i<max_sub_num;i++)
								{
									if(subs[i]!=NULL)
									{
										break;
									}
								}
								get_ppn_for_normal_command(ssd,channel,chip,subs[i]);	
							}
						}//else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)==AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)!=AD_TWOPLANE))
						else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
						{
							for(i=0;i<ssd->parameter->die_chip;i++)
							{
								die_token=ssd->channel_head[channel].chip_head[chip].token;
								ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
								if(size(plane_bits[die_token])>1)
								{
									break;
								}
								
							}
							
							if(i<ssd->parameter->die_chip)
							{
								for(die=0;die<ssd->parameter->die_chip;die++)
								{
									if(die!=die_token)
									{
										for(plane=0;plane<ssd->parameter->plane_die;plane++)
										{
											if(subs[die*ssd->parameter->plane_die+plane]!=NULL)
											{
												subs[die*ssd->parameter->plane_die+plane]=NULL;
												subs_count--;
											}
										}
									}
								}
								get_ppn_for_advanced_commands(ssd,channel,chip,subs,subs_count,TWO_PLANE);
							}//if(i<ssd->parameter->die_chip)
							else
							{
								for(i=0;i<ssd->parameter->die_chip;i++)
								{
									die_token=ssd->channel_head[channel].chip_head[chip].token;
									ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
									if(plane_bits[die_token]!=0x00000000)
									{
										for(j=0;j<ssd->parameter->plane_die;j++)
										{
											plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die_token].token;
											ssd->channel_head[channel].chip_head[chip].die_head[die_token].token=(plane_token+1)%ssd->parameter->plane_die;
											if(((plane_bits[die_token])&(0x00000001<<plane_token))!=0x00000000)
											{
												sub=subs[die_token*ssd->parameter->plane_die+plane_token];
												break;
											}
										}
									}
								}//for(i=0;i<ssd->parameter->die_chip;i++)
								get_ppn_for_normal_command(ssd,channel,chip,sub);
							}//else
						}//else if (((ssd->parameter->advanced_commands&AD_INTERLEAVE)!=AD_INTERLEAVE)&&((ssd->parameter->advanced_commands&AD_TWOPLANE)==AD_TWOPLANE))
					}//if (subs_count>1)  
					else
					{
						for(i=0;i<ssd->parameter->die_chip;i++)
						{
							die_token=ssd->channel_head[channel].chip_head[chip].token;
							ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
							if(plane_bits[die_token]!=0x00000000)
							{
								for(j=0;j<ssd->parameter->plane_die;j++)
								{
									plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die_token].token;
									ssd->channel_head[channel].chip_head[chip].die_head[die_token].token=(plane_token+1)%ssd->parameter->plane_die;
									if(((plane_bits[die_token])&(0x00000001<<plane_token))!=0x00000000)
									{
										sub=subs[die_token*ssd->parameter->plane_die+plane_token];
										break;
									}
								}
								if(sub!=NULL)
								{
									break;
								}
							}
						}//for(i=0;i<ssd->parameter->die_chip;i++)
						get_ppn_for_normal_command(ssd,channel,chip,sub);
					}//else
				}
			}//if (ssd->parameter->advanced_commands!=0)
			else
			{
				for(i=0;i<ssd->parameter->die_chip;i++)
				{
					die_token=ssd->channel_head[channel].chip_head[chip].token;
					ssd->channel_head[channel].chip_head[chip].token=(die_token+1)%ssd->parameter->die_chip;
					if(plane_bits[die_token]!=0x00000000)
					{
						for(j=0;j<ssd->parameter->plane_die;j++)
						{
							plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die_token].token;
							ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane_token+1)%ssd->parameter->plane_die;
							if(((plane_bits[die_token])&(0x00000001<<plane_token))!=0x00000000)
							{
								sub=subs[die_token*ssd->parameter->plane_die+plane_token];
								break;
							}
						}
						if(sub!=NULL)
						{
							break;
						}
					}
				}//for(i=0;i<ssd->parameter->die_chip;i++)
				get_ppn_for_normal_command(ssd,channel,chip,sub);
			}//else
		
	}//else

	for(i=0;i<max_sub_num;i++)
	{
		subs[i]=NULL;
	}
	free(subs);
	subs=NULL;
	free(plane_bits);
	return ssd;
}

/****************************************
*Ö´ÐÐÐ´×ÓÇëÇóÊ±£¬ÎªÆÕÍ¨µÄÐ´×ÓÇëÇó»ñÈ¡ppn
*****************************************/
Status get_ppn_for_normal_command(struct ssd_info * ssd, unsigned int channel,unsigned int chip, struct sub_request * sub)
{
	unsigned int die=0;
	unsigned int plane=0;
	if(sub==NULL)
	{
		return ERROR;
	}
	
	if (ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)
	{
		die=ssd->channel_head[channel].chip_head[chip].token;
		plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
		get_ppn(ssd,channel,chip,die,plane,sub);
		ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
		ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
		
		compute_serve_time(ssd,channel,chip,die,&sub,1,NORMAL);
		return SUCCESS;
	}
	else
	{
		die=sub->location->die;
		plane=sub->location->plane;
		get_ppn(ssd,channel,chip,die,plane,sub);   
		compute_serve_time(ssd,channel,chip,die,&sub,1,NORMAL);
		return SUCCESS;
	}

}



/************************************************************************************************
*Îª¸ß¼¶ÃüÁî»ñÈ¡ppn
*¸ù¾Ý²»Í¬µÄÃüÁî£¬×ñ´ÓÔÚÍ¬Ò»¸öblockÖÐË³ÐòÐ´µÄÒªÇó£¬Ñ¡È¡¿ÉÒÔ½øÐÐÐ´²Ù×÷µÄppn£¬Ìø¹ýµÄppnÈ«²¿ÖÃÎªÊ§Ð§¡£
*ÔÚÊ¹ÓÃtwo plane²Ù×÷Ê±£¬ÎªÁËÑ°ÕÒÏàÍ¬Ë®Æ½Î»ÖÃµÄÒ³£¬¿ÉÄÜÐèÒªÖ±½ÓÕÒµ½Á½¸öÍêÈ«¿Õ°×µÄ¿é£¬Õâ¸öÊ±ºòÔ­À´
*µÄ¿éÃ»ÓÐÓÃÍê£¬Ö»ÄÜ·ÅÔÚÕâ£¬µÈ´ýÏÂ´ÎÊ¹ÓÃ£¬Í¬Ê±ÐÞ¸Ä²éÕÒ¿Õ°×pageµÄ·½·¨£¬½«ÒÔÇ°Ê×ÏÈÑ°ÕÒfree¿é¸ÄÎª£¬Ö»
*Òªinvalid block!=64¼´¿É¡£
*except find aim page, we should modify token and decide gc operation
*************************************************************************************************/
Status get_ppn_for_advanced_commands(struct ssd_info *ssd,unsigned int channel,unsigned int chip,struct sub_request * * subs ,unsigned int subs_count,unsigned int command)      
{
	unsigned int die=0,plane=0;
	unsigned int die_token=0,plane_token=0;
	struct sub_request * sub=NULL;
	unsigned int i=0,j=0,k=0;
	unsigned int unvalid_subs_count=0;
	unsigned int valid_subs_count=0;
	unsigned int interleave_flag=FALSE;
	unsigned int multi_plane_falg=FALSE;
	unsigned int max_subs_num=0;
	struct sub_request * first_sub_in_chip=NULL;
	struct sub_request * first_sub_in_die=NULL;
	struct sub_request * second_sub_in_die=NULL;
	unsigned int state=SUCCESS;
	unsigned int multi_plane_flag=FALSE;

	max_subs_num=ssd->parameter->die_chip*ssd->parameter->plane_die;
	
	if (ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)                         /*¶¯Ì¬·ÖÅä²Ù×÷*/ 
	{
		if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))                      /*INTERLEAVE_TWO_PLANEÒÔ¼°COPY_BACKµÄÇé¿ö*/
		{
			for(i=0;i<subs_count;i++)
			{
				die=ssd->channel_head[channel].chip_head[chip].token;
				if(i<ssd->parameter->die_chip)                                         /*ÎªÃ¿¸ösubs[i]»ñÈ¡ppn£¬iÐ¡ÓÚdie_chip*/
				{
					plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
					get_ppn(ssd,channel,chip,die,plane,subs[i]);
					ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
				}
				else                                                                  
				{   
					/*********************************************************************************************************************************
					*³¬¹ýdie_chipµÄiËùÖ¸ÏòµÄsubs[i]Óësubs[i%ssd->parameter->die_chip]»ñÈ¡ÏàÍ¬Î»ÖÃµÄppn
					*Èç¹û³É¹¦µÄ»ñÈ¡ÁËÔòÁîmulti_plane_flag=TRUE²¢Ö´ÐÐcompute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE_TWO_PLANE);
					*·ñÔòÖ´ÐÐcompute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
					***********************************************************************************************************************************/
					state=make_level_page(ssd,subs[i%ssd->parameter->die_chip],subs[i]);
					if(state!=SUCCESS)                                                 
					{
						subs[i]=NULL;
						unvalid_subs_count++;
					}
					else
					{
						multi_plane_flag=TRUE;
					}
				}
				ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
			}
			valid_subs_count=subs_count-unvalid_subs_count;
			ssd->interleave_count++;
			if(multi_plane_flag==TRUE)
			{
				ssd->inter_mplane_count++;
				compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE_TWO_PLANE);/*¼ÆËãÐ´×ÓÇëÇóµÄ´¦ÀíÊ±¼ä£¬ÒÔÐ´×ÓÇëÇóµÄ×´Ì¬×ª±ä*/		
			}
			else
			{
				compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
			}
			return SUCCESS;
		}//if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
		else if(command==INTERLEAVE)
		{
			/***********************************************************************************************
			*INTERLEAVE¸ß¼¶ÃüÁîµÄ´¦Àí£¬Õâ¸ö´¦Àí±ÈTWO_PLANE¸ß¼¶ÃüÁîµÄ´¦Àí¼òµ¥
			*ÒòÎªtwo_planeµÄÒªÇóÊÇÍ¬Ò»¸ödieÀïÃæ²»Í¬planeµÄÍ¬Ò»Î»ÖÃµÄpage£¬¶øinterleaveÒªÇóÔòÊÇ²»Í¬dieÀïÃæµÄ¡£
			************************************************************************************************/
			for(i=0;(i<subs_count)&&(i<ssd->parameter->die_chip);i++)
			{
				die=ssd->channel_head[channel].chip_head[chip].token;
				plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
				get_ppn(ssd,channel,chip,die,plane,subs[i]);
				ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane+1)%ssd->parameter->plane_die;
				ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
				valid_subs_count++;
			}
			ssd->interleave_count++;
			compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
			return SUCCESS;
		}//else if(command==INTERLEAVE)
		else if(command==TWO_PLANE)
		{
			if(subs_count<2)
			{
				return ERROR;
			}
			die=ssd->channel_head[channel].chip_head[chip].token;
			for(j=0;j<subs_count;j++)
			{
				if(j==1)
				{
					state=find_level_page(ssd,channel,chip,die,subs[0],subs[1]);        /*Ñ°ÕÒÓësubs[0]µÄppnÎ»ÖÃÏàÍ¬µÄsubs[1]£¬Ö´ÐÐTWO_PLANE¸ß¼¶ÃüÁî*/
					if(state!=SUCCESS)
					{
						get_ppn_for_normal_command(ssd,channel,chip,subs[0]);           /*Ã»ÕÒµ½£¬ÄÇÃ´¾Íµ±ÆÕÍ¨ÃüÁîÀ´´¦Àí*/
						return FAILURE;
					}
					else
					{
						valid_subs_count=2;
					}
				}
				else if(j>1)
				{
					state=make_level_page(ssd,subs[0],subs[j]);                         /*Ñ°ÕÒÓësubs[0]µÄppnÎ»ÖÃÏàÍ¬µÄsubs[j]£¬Ö´ÐÐTWO_PLANE¸ß¼¶ÃüÁî*/
					if(state!=SUCCESS)
					{
						for(k=j;k<subs_count;k++)
						{
							subs[k]=NULL;
						}
						subs_count=j;
						break;
					}
					else
					{
						valid_subs_count++;
					}
				}
			}//for(j=0;j<subs_count;j++)
			ssd->channel_head[channel].chip_head[chip].token=(die+1)%ssd->parameter->die_chip;
			ssd->m_plane_prog_count++;
			compute_serve_time(ssd,channel,chip,die,subs,valid_subs_count,TWO_PLANE);
			return SUCCESS;
		}//else if(command==TWO_PLANE)
		else 
		{
			return ERROR;
		}
	}//if (ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)
	else                                                                              /*¾²Ì¬·ÖÅäµÄÇé¿ö*/
	{
		if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
		{
			for(die=0;die<ssd->parameter->die_chip;die++)
			{
				first_sub_in_die=NULL;
				for(plane=0;plane<ssd->parameter->plane_die;plane++)
				{
					sub=subs[die*ssd->parameter->plane_die+plane];
					if(sub!=NULL)
					{
						if(first_sub_in_die==NULL)
						{
							first_sub_in_die=sub;
							get_ppn(ssd,channel,chip,die,plane,sub);
						}
						else
						{
							state=make_level_page(ssd,first_sub_in_die,sub);
							if(state!=SUCCESS)
							{
								subs[die*ssd->parameter->plane_die+plane]=NULL;
								subs_count--;
								sub=NULL;
							}
							else
							{
								multi_plane_flag=TRUE;
							}
						}
					}
				}
			}
			if(multi_plane_flag==TRUE)
			{
				ssd->inter_mplane_count++;
				compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE_TWO_PLANE);
				return SUCCESS;
			}
			else
			{
				compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);
				return SUCCESS;
			}
		}//if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
		else if(command==INTERLEAVE)
		{
			for(die=0;die<ssd->parameter->die_chip;die++)
			{	
				first_sub_in_die=NULL;
				for(plane=0;plane<ssd->parameter->plane_die;plane++)
				{
					sub=subs[die*ssd->parameter->plane_die+plane];
					if(sub!=NULL)
					{
						if(first_sub_in_die==NULL)
						{
							first_sub_in_die=sub;
							get_ppn(ssd,channel,chip,die,plane,sub);
							valid_subs_count++;
						}
						else
						{
							subs[die*ssd->parameter->plane_die+plane]=NULL;
							subs_count--;
							sub=NULL;
						}
					}
				}
			}
			if(valid_subs_count>1)
			{
				ssd->interleave_count++;
			}
			compute_serve_time(ssd,channel,chip,0,subs,valid_subs_count,INTERLEAVE);	
		}//else if(command==INTERLEAVE)
		else if(command==TWO_PLANE)
		{
			for(die=0;die<ssd->parameter->die_chip;die++)
			{	
				first_sub_in_die=NULL;
				second_sub_in_die=NULL;
				for(plane=0;plane<ssd->parameter->plane_die;plane++)
				{
					sub=subs[die*ssd->parameter->plane_die+plane];
					if(sub!=NULL)
					{	
						if(first_sub_in_die==NULL)
						{
							first_sub_in_die=sub;
						}
						else if(second_sub_in_die==NULL)
						{
							second_sub_in_die=sub;
							state=find_level_page(ssd,channel,chip,die,first_sub_in_die,second_sub_in_die);
							if(state!=SUCCESS)
							{
								subs[die*ssd->parameter->plane_die+plane]=NULL;
								subs_count--;
								second_sub_in_die=NULL;
								sub=NULL;
							}
							else
							{
								valid_subs_count=2;
							}
						}
						else
						{
							state=make_level_page(ssd,first_sub_in_die,sub);
							if(state!=SUCCESS)
							{
								subs[die*ssd->parameter->plane_die+plane]=NULL;
								subs_count--;
								sub=NULL;
							}
							else
							{
								valid_subs_count++;
							}
						}
					}//if(sub!=NULL)
				}//for(plane=0;plane<ssd->parameter->plane_die;plane++)
				if(second_sub_in_die!=NULL)
				{
					multi_plane_flag=TRUE;
					break;
				}
			}//for(die=0;die<ssd->parameter->die_chip;die++)
			if(multi_plane_flag==TRUE)
			{
				ssd->m_plane_prog_count++;
				compute_serve_time(ssd,channel,chip,die,subs,valid_subs_count,TWO_PLANE);
				return SUCCESS;
			}//if(multi_plane_flag==TRUE)
			else
			{
				i=0;
				sub=NULL;
				while((sub==NULL)&&(i<max_subs_num))
				{
					sub=subs[i];
					i++;
				}
				if(sub!=NULL)
				{
					get_ppn_for_normal_command(ssd,channel,chip,sub);
					return FAILURE;
				}
				else 
				{
					return ERROR;
				}
			}//else
		}//else if(command==TWO_PLANE)
		else
		{
			return ERROR;
		}
	}//elseb ¾²Ì¬·ÖÅäµÄÇé¿ö
}


/***********************************************
*º¯ÊýµÄ×÷ÓÃÊÇÈÃsub0£¬sub1µÄppnËùÔÚµÄpageÎ»ÖÃÏàÍ¬
************************************************/
Status make_level_page(struct ssd_info * ssd, struct sub_request * sub0,struct sub_request * sub1)
{
	unsigned int i=0,j=0,k=0;
	unsigned int channel=0,chip=0,die=0,plane0=0,plane1=0,block0=0,block1=0,page0=0,page1=0;
	unsigned int active_block0=0,active_block1=0;
	unsigned int old_plane_token=0;
	
	if((sub0==NULL)||(sub1==NULL)||(sub0->location==NULL))
	{
		return ERROR;
	}
	channel=sub0->location->channel;
	chip=sub0->location->chip;
	die=sub0->location->die;
	plane0=sub0->location->plane;
	block0=sub0->location->block;
	page0=sub0->location->page;
	old_plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die].token;

	/***********************************************************************************************
	*¶¯Ì¬·ÖÅäµÄÇé¿öÏÂ
	*sub1µÄplaneÊÇ¸ù¾Ýsub0µÄssd->channel_head[channel].chip_head[chip].die_head[die].tokenÁîÅÆ»ñÈ¡µÄ
	*sub1µÄchannel£¬chip£¬die£¬block£¬page¶¼ºÍsub0µÄÏàÍ¬
	************************************************************************************************/
	if(ssd->parameter->allocation_scheme==DYNAMIC_ALLOCATION)                             
	{
		old_plane_token=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
		for(i=0;i<ssd->parameter->plane_die;i++)
		{
			plane1=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
			if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].add_reg_ppn==-1)
			{
				find_active_block(ssd,channel,chip,die,plane1);                               /*ÔÚplane1ÖÐÕÒµ½»îÔ¾¿é*/
				block1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].active_block;

				/*********************************************************************************************
				*Ö»ÓÐÕÒµ½µÄblock1Óëblock0ÏàÍ¬£¬²ÅÄÜ¼ÌÐøÍùÏÂÑ°ÕÒÏàÍ¬µÄpage
				*ÔÚÑ°ÕÒpageÊ±±È½Ï¼òµ¥£¬Ö±½ÓÓÃlast_write_page£¨ÉÏÒ»´ÎÐ´µÄpage£©+1¾Í¿ÉÒÔÁË¡£
				*Èç¹ûÕÒµ½µÄpage²»ÏàÍ¬£¬ÄÇÃ´Èç¹ûssdÔÊÐíÌ°À·µÄÊ¹ÓÃ¸ß¼¶ÃüÁî£¬ÕâÑù¾Í¿ÉÒÔÈÃÐ¡µÄpage Íù´óµÄpage¿¿Â£
				*********************************************************************************************/
				if(block1==block0)
				{
					page1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].blk_head[block1].last_write_page+1;
					if(page1==page0)
					{
						break;
					}
					else if(page1<page0)
					{
						if (ssd->parameter->greed_MPW_ad==1)                                  /*ÔÊÐíÌ°À·µÄÊ¹ÓÃ¸ß¼¶ÃüÁî*/
						{                                                                   
							//make_same_level(ssd,channel,chip,die,plane1,active_block1,page0); /*Ð¡µÄpageµØÖ·Íù´óµÄpageµØÖ·¿¿*/
							make_same_level(ssd,channel,chip,die,plane1,block1,page0);
							break;
						}    
					}
				}//if(block1==block0)
			}
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane1+1)%ssd->parameter->plane_die;
		}//for(i=0;i<ssd->parameter->plane_die;i++)
		if(i<ssd->parameter->plane_die)
		{
			flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page0);          /*Õâ¸öº¯ÊýµÄ×÷ÓÃ¾ÍÊÇ¸üÐÂpage1Ëù¶ÔÓ¦µÄÎïÀíÒ³ÒÔ¼°location»¹ÓÐmap±í*/
			//flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page1);
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(plane1+1)%ssd->parameter->plane_die;
			return SUCCESS;
		}
		else
		{
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane_token;
			return FAILURE;
		}
	}
	else                                                                                      /*¾²Ì¬·ÖÅäµÄÇé¿ö*/
	{
		if((sub1->location==NULL)||(sub1->location->channel!=channel)||(sub1->location->chip!=chip)||(sub1->location->die!=die))
		{
			return ERROR;
		}
		plane1=sub1->location->plane;
		if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].add_reg_ppn==-1)
		{
			find_active_block(ssd,channel,chip,die,plane1);
			block1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].active_block;
			if(block1==block0)
			{
				page1=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane1].blk_head[block1].last_write_page+1;
				if(page1>page0)
				{
					return FAILURE;
				}
				else if(page1<page0)
				{
					if (ssd->parameter->greed_MPW_ad==1)
					{ 
						//make_same_level(ssd,channel,chip,die,plane1,active_block1,page0);    /*Ð¡µÄpageµØÖ·Íù´óµÄpageµØÖ·¿¿*/
                        make_same_level(ssd,channel,chip,die,plane1,block1,page0);
						flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page0);
						//flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page1);
						return SUCCESS;
					}
					else
					{
						return FAILURE;
					}					
				}
				else
				{
					flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page0);
					//flash_page_state_modify(ssd,sub1,channel,chip,die,plane1,block1,page1);
					return SUCCESS;
				}
				
			}
			else
			{
				return FAILURE;
			}
			
		}
		else
		{
			return ERROR;
		}
	}
	
}

/******************************************************************************************************
*º¯ÊýµÄ¹¦ÄÜÊÇÎªtwo planeÃüÁîÑ°ÕÒ³öÁ½¸öÏàÍ¬Ë®Æ½Î»ÖÃµÄÒ³£¬²¢ÇÒÐÞ¸ÄÍ³¼ÆÖµ£¬ÐÞ¸ÄÒ³µÄ×´Ì¬
*×¢ÒâÕâ¸öº¯ÊýÓëÉÏÒ»¸öº¯Êýmake_level_pageº¯ÊýµÄÇø±ð£¬make_level_pageÕâ¸öº¯ÊýÊÇÈÃsub1Óësub0µÄpageÎ»ÖÃÏàÍ¬
*¶øfind_level_pageº¯ÊýµÄ×÷ÓÃÊÇÔÚ¸ø¶¨µÄchannel£¬chip£¬dieÖÐÕÒÁ½¸öÎ»ÖÃÏàÍ¬µÄsubAºÍsubB¡£
*******************************************************************************************************/
Status find_level_page(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request *subA,struct sub_request *subB)       
{
	unsigned int i,planeA,planeB,active_blockA,active_blockB,pageA,pageB,aim_page,old_plane;
	struct gc_operation *gc_node;

	old_plane=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
    
	/************************************************************
	*ÔÚ¶¯Ì¬·ÖÅäµÄÇé¿öÏÂ
	*planeA¸³³õÖµÎªdieµÄÁîÅÆ£¬Èç¹ûplaneAÊÇÅ¼ÊýÄÇÃ´planeB=planeA+1
	*planeAÊÇÆæÊý£¬ÄÇÃ´planeA+1±äÎªÅ¼Êý£¬ÔÙÁîplaneB=planeA+1
	*************************************************************/
	if (ssd->parameter->allocation_scheme==0)                                                
	{
		planeA=ssd->channel_head[channel].chip_head[chip].die_head[die].token;
		if (planeA%2==0)
		{
			planeB=planeA+1;
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(ssd->channel_head[channel].chip_head[chip].die_head[die].token+2)%ssd->parameter->plane_die;
		} 
		else
		{
			planeA=(planeA+1)%ssd->parameter->plane_die;
			planeB=planeA+1;
			ssd->channel_head[channel].chip_head[chip].die_head[die].token=(ssd->channel_head[channel].chip_head[chip].die_head[die].token+3)%ssd->parameter->plane_die;
		}
	} 
	else                                                                                     /*¾²Ì¬·ÖÅäµÄÇé¿ö£¬¾ÍÖ±½Ó¸³Öµ¸øplaneAºÍplaneB*/
	{
		planeA=subA->location->plane;
		planeB=subB->location->plane;
	}
	find_active_block(ssd,channel,chip,die,planeA);                                          /*Ñ°ÕÒactive_block*/
	find_active_block(ssd,channel,chip,die,planeB);
	active_blockA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].active_block;
	active_blockB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].active_block;

	
    
	/*****************************************************
	*Èç¹ûactive_blockÏàÍ¬£¬ÄÇÃ´¾ÍÔÚÕâÁ½¸ö¿éÖÐÕÒÏàÍ¬µÄpage
	*»òÕßÊ¹ÓÃÌ°À·µÄ·½·¨ÕÒµ½Á½¸öÏàÍ¬µÄpage
	******************************************************/
	if (active_blockA==active_blockB)
	{
		pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockA].last_write_page+1;      
		pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockB].last_write_page+1;
		if (pageA==pageB)                                                                    /*Á½¸ö¿ÉÓÃµÄÒ³ÕýºÃÔÚÍ¬Ò»¸öË®Æ½Î»ÖÃÉÏ*/
		{
			flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,pageA);
			flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,pageB);
		} 
		else
		{
			if (ssd->parameter->greed_MPW_ad==1)                                             /*Ì°À·µØÊ¹ÓÃ¸ß¼¶ÃüÁî*/
			{
				if (pageA<pageB)                                                            
				{
					aim_page=pageB;
					make_same_level(ssd,channel,chip,die,planeA,active_blockA,aim_page);     /*Ð¡µÄpageµØÖ·Íù´óµÄpageµØÖ·¿¿*/
				}
				else
				{
					aim_page=pageA;
					make_same_level(ssd,channel,chip,die,planeB,active_blockB,aim_page);    
				}
				flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,aim_page);
				flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,aim_page);
			} 
			else                                                                             /*²»ÄÜÌ°À·µÄÊ¹ÓÃ¸ß¼¶ÃüÁî*/
			{
				subA=NULL;
				subB=NULL;
				ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
				return FAILURE;
			}
		}
	}
	/*********************************
	*Èç¹ûÕÒµ½µÄÁ½¸öactive_block²»ÏàÍ¬
	**********************************/
	else
	{   
		pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockA].last_write_page+1;      
		pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockB].last_write_page+1;
		if (pageA<pageB)
		{
			if (ssd->parameter->greed_MPW_ad==1)                                             /*Ì°À·µØÊ¹ÓÃ¸ß¼¶ÃüÁî*/
			{
				/*******************************************************************************
				*ÔÚplaneAÖÐ£¬Óëactive_blockBÏàÍ¬Î»ÖÃµÄµÄblockÖÐ£¬ÓëpageBÏàÍ¬Î»ÖÃµÄpageÊÇ¿ÉÓÃµÄ¡£
				*Ò²¾ÍÊÇpalneAÖÐµÄÏàÓ¦Ë®Æ½Î»ÖÃÊÇ¿ÉÓÃµÄ£¬½«Æä×îÎªÓëplaneBÖÐ¶ÔÓ¦µÄÒ³¡£
				*ÄÇÃ´¿ÉÒ²ÈÃplaneA£¬active_blockBÖÐµÄpageÍùpageB¿¿Â£
				********************************************************************************/
				if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockB].page_head[pageB].free_state==PG_SUB)    
				{
					make_same_level(ssd,channel,chip,die,planeA,active_blockB,pageB);
					flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockB,pageB);
					flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,pageB);
				}
                /********************************************************************************
				*ÔÚplaneAÖÐ£¬Óëactive_blockBÏàÍ¬Î»ÖÃµÄµÄblockÖÐ£¬ÓëpageBÏàÍ¬Î»ÖÃµÄpageÊÇ¿ÉÓÃµÄ¡£
				*ÄÇÃ´¾ÍÒªÖØÐÂÑ°ÕÒblock£¬ÐèÒªÖØÐÂÕÒË®Æ½Î»ÖÃÏàÍ¬µÄÒ»¶ÔÒ³
				*********************************************************************************/
				else    
				{
					for (i=0;i<ssd->parameter->block_plane;i++)
					{
						pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].last_write_page+1;
						pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].last_write_page+1;
						if ((pageA<ssd->parameter->page_block)&&(pageB<ssd->parameter->page_block))
						{
							if (pageA<pageB)
							{
								if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].page_head[pageB].free_state==PG_SUB)
								{
									aim_page=pageB;
									make_same_level(ssd,channel,chip,die,planeA,i,aim_page);
									break;
								}
							} 
							else
							{
								if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].page_head[pageA].free_state==PG_SUB)
								{
									aim_page=pageA;
									make_same_level(ssd,channel,chip,die,planeB,i,aim_page);
									break;
								}
							}
						}
					}//for (i=0;i<ssd->parameter->block_plane;i++)
					if (i<ssd->parameter->block_plane)
					{
						flash_page_state_modify(ssd,subA,channel,chip,die,planeA,i,aim_page);
						flash_page_state_modify(ssd,subB,channel,chip,die,planeB,i,aim_page);
					} 
					else
					{
						subA=NULL;
						subB=NULL;
						ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
						return FAILURE;
					}
				}
			}//if (ssd->parameter->greed_MPW_ad==1)  
			else
			{
				subA=NULL;
				subB=NULL;
				ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
				return FAILURE;
			}
		}//if (pageA<pageB)
		else
		{
			if (ssd->parameter->greed_MPW_ad==1)     
			{
				if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockA].page_head[pageA].free_state==PG_SUB)
				{
					make_same_level(ssd,channel,chip,die,planeB,active_blockA,pageA);
					flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,pageA);
					flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockA,pageA);
				}
				else    
				{
					for (i=0;i<ssd->parameter->block_plane;i++)
					{
						pageA=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].last_write_page+1;
						pageB=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].last_write_page+1;
						if ((pageA<ssd->parameter->page_block)&&(pageB<ssd->parameter->page_block))
						{
							if (pageA<pageB)
							{
								if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[i].page_head[pageB].free_state==PG_SUB)
								{
									aim_page=pageB;
									make_same_level(ssd,channel,chip,die,planeA,i,aim_page);
									break;
								}
							} 
							else
							{
								if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[i].page_head[pageA].free_state==PG_SUB)
								{
									aim_page=pageA;
									make_same_level(ssd,channel,chip,die,planeB,i,aim_page);
									break;
								}
							}
						}
					}//for (i=0;i<ssd->parameter->block_plane;i++)
					if (i<ssd->parameter->block_plane)
					{
						flash_page_state_modify(ssd,subA,channel,chip,die,planeA,i,aim_page);
						flash_page_state_modify(ssd,subB,channel,chip,die,planeB,i,aim_page);
					} 
					else
					{
						subA=NULL;
						subB=NULL;
						ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
						return FAILURE;
					}
				}
			} //if (ssd->parameter->greed_MPW_ad==1) 
			else
			{
				if ((pageA==pageB)&&(pageA==0))
				{
					/*******************************************************************************************
					*ÏÂÃæÊÇÁ½ÖÖÇé¿ö
					*1£¬planeA£¬planeBÖÐµÄactive_blockA£¬pageAÎ»ÖÃ¶¼¿ÉÓÃ£¬ÄÇÃ´²»Í¬plane µÄÏàÍ¬Î»ÖÃ£¬ÒÔblockAÎª×¼
					*2£¬planeA£¬planeBÖÐµÄactive_blockB£¬pageAÎ»ÖÃ¶¼¿ÉÓÃ£¬ÄÇÃ´²»Í¬plane µÄÏàÍ¬Î»ÖÃ£¬ÒÔblockBÎª×¼
					********************************************************************************************/
					if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockA].page_head[pageA].free_state==PG_SUB)
					  &&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockA].page_head[pageA].free_state==PG_SUB))
					{
						flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockA,pageA);
						flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockA,pageA);
					}
					else if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].blk_head[active_blockB].page_head[pageA].free_state==PG_SUB)
						   &&(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].blk_head[active_blockB].page_head[pageA].free_state==PG_SUB))
					{
						flash_page_state_modify(ssd,subA,channel,chip,die,planeA,active_blockB,pageA);
						flash_page_state_modify(ssd,subB,channel,chip,die,planeB,active_blockB,pageA);
					}
					else
					{
						subA=NULL;
						subB=NULL;
						ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
						return FAILURE;
					}
				}
				else
				{
					subA=NULL;
					subB=NULL;
					ssd->channel_head[channel].chip_head[chip].die_head[die].token=old_plane;
					return ERROR;
				}
			}
		}
	}

	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeA].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
	{
		gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
		alloc_assert(gc_node,"gc_node");
		memset(gc_node,0, sizeof(struct gc_operation));

		gc_node->next_node=NULL;
		gc_node->chip=chip;
		gc_node->die=die;
		gc_node->plane=planeA;
		gc_node->block=0xffffffff;
		gc_node->page=0;
		gc_node->state=GC_WAIT;
		gc_node->priority=GC_UNINTERRUPT;
		gc_node->next_node=ssd->channel_head[channel].gc_command;
		ssd->channel_head[channel].gc_command=gc_node;
		ssd->gc_request++;
	}
	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[planeB].free_page<(ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->gc_hard_threshold))
	{
		gc_node=(struct gc_operation *)malloc(sizeof(struct gc_operation));
		alloc_assert(gc_node,"gc_node");
		memset(gc_node,0, sizeof(struct gc_operation));

		gc_node->next_node=NULL;
		gc_node->chip=chip;
		gc_node->die=die;
		gc_node->plane=planeB;
		gc_node->block=0xffffffff;
		gc_node->page=0;
		gc_node->state=GC_WAIT;
		gc_node->priority=GC_UNINTERRUPT;
		gc_node->next_node=ssd->channel_head[channel].gc_command;
		ssd->channel_head[channel].gc_command=gc_node;
		ssd->gc_request++;
	}

	return SUCCESS;     
}

/*
*º¯ÊýµÄ¹¦ÄÜÊÇÐÞ¸ÄÕÒµ½µÄpageÒ³µÄ×´Ì¬ÒÔ¼°ÏàÓ¦µÄdramÖÐÓ³Éä±íµÄÖµ
*/
struct ssd_info *flash_page_state_modify(struct ssd_info *ssd,struct sub_request *sub,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page)
{
	unsigned int ppn,full_page;
	struct local *location;
	struct direct_erase *new_direct_erase,*direct_erase_node;
	
	full_page=~(0xffffffff<<ssd->parameter->subpage_page);
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=page;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num--;

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page>63)
	{
		printf("error! the last write page larger than 64!!\n");
		while(1){}
	}

	if(ssd->dram->map->map_entry[sub->lpn].state==0)                                          /*this is the first logical page*/
	{
		ssd->dram->map->map_entry[sub->lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[sub->lpn].state=sub->state;
	}
	else                                                                                      /*Õâ¸öÂß¼­Ò³½øÐÐÁË¸üÐÂ£¬ÐèÒª½«Ô­À´µÄÒ³ÖÃÎªÊ§Ð§*/
	{
		ppn=ssd->dram->map->map_entry[sub->lpn].pn;
		location=find_location(ssd,ppn);
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state=0;        //±íÊ¾Ä³Ò»Ò³Ê§Ð§£¬Í¬Ê±±ê¼ÇvalidºÍfree×´Ì¬¶¼Îª0
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state=0;         //±íÊ¾Ä³Ò»Ò³Ê§Ð§£¬Í¬Ê±±ê¼ÇvalidºÍfree×´Ì¬¶¼Îª0
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn=0;
		ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;
		if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num==ssd->parameter->page_block)    //¸ÃblockÖÐÈ«ÊÇinvalidµÄÒ³£¬¿ÉÒÔÖ±½ÓÉ¾³ý
		{
			new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
			alloc_assert(new_direct_erase,"new_direct_erase");
			memset(new_direct_erase,0, sizeof(struct direct_erase));

			new_direct_erase->block=location->block;
			new_direct_erase->next_node=NULL;
			direct_erase_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
			if (direct_erase_node==NULL)
			{
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
			} 
			else
			{
				new_direct_erase->next_node=ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node=new_direct_erase;
			}
		}
		free(location);
		location=NULL;
		ssd->dram->map->map_entry[sub->lpn].pn=find_ppn(ssd,channel,chip,die,plane,block,page);
		ssd->dram->map->map_entry[sub->lpn].state=(ssd->dram->map->map_entry[sub->lpn].state|sub->state);
	}

	sub->ppn=ssd->dram->map->map_entry[sub->lpn].pn;
	sub->location->channel=channel;
	sub->location->chip=chip;
	sub->location->die=die;
	sub->location->plane=plane;
	sub->location->block=block;
	sub->location->page=page;
	
	ssd->program_count++;
	ssd->channel_head[channel].program_count++;
	ssd->channel_head[channel].chip_head[chip].program_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].lpn=sub->lpn;	
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].valid_state=sub->state;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].free_state=((~(sub->state))&full_page);
	ssd->write_flash_count++;

	return ssd;
}


/********************************************
*º¯ÊýµÄ¹¦ÄÜ¾ÍÊÇÈÃÁ½¸öÎ»ÖÃ²»Í¬µÄpageÎ»ÖÃÏàÍ¬
*********************************************/
struct ssd_info *make_same_level(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int aim_page)
{
	int i=0,step,page;
	struct direct_erase *new_direct_erase,*direct_erase_node;

	page=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page+1;                  /*ÐèÒªµ÷ÕûµÄµ±Ç°¿éµÄ¿ÉÐ´Ò³ºÅ*/
	step=aim_page-page;
	while (i<step)
	{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].valid_state=0;     /*±íÊ¾Ä³Ò»Ò³Ê§Ð§£¬Í¬Ê±±ê¼ÇvalidºÍfree×´Ì¬¶¼Îª0*/
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].free_state=0;      /*±íÊ¾Ä³Ò»Ò³Ê§Ð§£¬Í¬Ê±±ê¼ÇvalidºÍfree×´Ì¬¶¼Îª0*/
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page+i].lpn=0;

		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num++;

		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num--;

		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;

		i++;
	}

	ssd->waste_page_count+=step;

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page=aim_page-1;

	if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num==ssd->parameter->page_block)    /*¸ÃblockÖÐÈ«ÊÇinvalidµÄÒ³£¬¿ÉÒÔÖ±½ÓÉ¾³ý*/
	{
		new_direct_erase=(struct direct_erase *)malloc(sizeof(struct direct_erase));
		alloc_assert(new_direct_erase,"new_direct_erase");
		memset(new_direct_erase,0, sizeof(struct direct_erase));

		direct_erase_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
		if (direct_erase_node==NULL)
		{
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=new_direct_erase;
		} 
		else
		{
			new_direct_erase->next_node=ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
			ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node=new_direct_erase;
		}
	}

	if(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page>63)
		{
		printf("error! the last write page larger than 64!!\n");
		while(1){}
		}

	return ssd;
}


/****************************************************************************
*ÔÚ´¦Àí¸ß¼¶ÃüÁîµÄÐ´×ÓÇëÇóÊ±£¬Õâ¸öº¯ÊýµÄ¹¦ÄÜ¾ÍÊÇ¼ÆËã´¦ÀíÊ±¼äÒÔ¼°´¦ÀíµÄ×´Ì¬×ª±ä
*¹¦ÄÜ»¹²»ÊÇºÜÍêÉÆ£¬ÐèÒªÍêÉÆ£¬ÐÞ¸ÄÊ±×¢ÒâÒª·ÖÎª¾²Ì¬·ÖÅäºÍ¶¯Ì¬·ÖÅäÁ½ÖÖÇé¿ö
*****************************************************************************/
struct ssd_info *compute_serve_time(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request **subs, unsigned int subs_count,unsigned int command)
{
	unsigned int i=0;
	unsigned int max_subs_num=0;
	struct sub_request *sub=NULL,*p=NULL;
	struct sub_request * last_sub=NULL;
	max_subs_num=ssd->parameter->die_chip*ssd->parameter->plane_die;

	if((command==INTERLEAVE_TWO_PLANE)||(command==COPY_BACK))
	{
		for(i=0;i<max_subs_num;i++)
		{
			if(subs[i]!=NULL)
			{
				last_sub=subs[i];
				subs[i]->current_state=SR_W_TRANSFER;
				subs[i]->current_time=ssd->current_time;
				subs[i]->next_state=SR_COMPLETE;
				subs[i]->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				subs[i]->complete_time=subs[i]->next_state_predict_time;

				delete_from_channel(ssd,channel,subs[i]);
			}
		}
		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=last_sub->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;	
	}
	else if(command==TWO_PLANE)
	{
		for(i=0;i<max_subs_num;i++)
		{
			if(subs[i]!=NULL)
			{
				
				subs[i]->current_state=SR_W_TRANSFER;
				if(last_sub==NULL)
				{
					subs[i]->current_time=ssd->current_time;
				}
				else
				{
					subs[i]->current_time=last_sub->complete_time+ssd->parameter->time_characteristics.tDBSY;
				}
				
				subs[i]->next_state=SR_COMPLETE;
				subs[i]->next_state_predict_time=subs[i]->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				subs[i]->complete_time=subs[i]->next_state_predict_time;
				last_sub=subs[i];

				delete_from_channel(ssd,channel,subs[i]);
			}
		}
		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=last_sub->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
	}
	else if(command==INTERLEAVE)
	{
		for(i=0;i<max_subs_num;i++)
		{
			if(subs[i]!=NULL)
			{
				
				subs[i]->current_state=SR_W_TRANSFER;
				if(last_sub==NULL)
				{
					subs[i]->current_time=ssd->current_time;
				}
				else
				{
					subs[i]->current_time=last_sub->complete_time;
				}
				subs[i]->next_state=SR_COMPLETE;
				subs[i]->next_state_predict_time=subs[i]->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[i]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				subs[i]->complete_time=subs[i]->next_state_predict_time;
				last_sub=subs[i];

				delete_from_channel(ssd,channel,subs[i]);
			}
		}
		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=last_sub->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
	}
	else if(command==NORMAL)
	{
		subs[0]->current_state=SR_W_TRANSFER;
		subs[0]->current_time=ssd->current_time;
		subs[0]->next_state=SR_COMPLETE;
		subs[0]->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(subs[0]->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		subs[0]->complete_time=subs[0]->next_state_predict_time;

		delete_from_channel(ssd,channel,subs[0]);

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=subs[0]->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
	}
	else
	{
		return NULL;
	}
	
	return ssd;

}


/*****************************************************************************************
*º¯ÊýµÄ¹¦ÄÜ¾ÍÊÇ°Ñ×ÓÇëÇó´Óssd->subs_w_head»òÕßssd->channel_head[channel].subs_w_headÉÏÉ¾³ý
******************************************************************************************/
struct ssd_info *delete_from_channel(struct ssd_info *ssd,unsigned int channel,struct sub_request * sub_req)
{
	struct sub_request *sub,*p;
    
	/******************************************************************
	*ÍêÈ«¶¯Ì¬·ÖÅä×ÓÇëÇó¾ÍÔÚssd->subs_w_headÉÏ
	*²»ÊÇÍêÈ«¶¯Ì¬·ÖÅä×ÓÇëÇó¾ÍÔÚssd->channel_head[channel].subs_w_headÉÏ
	*******************************************************************/
	if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))    
	{
		sub=ssd->subs_w_head;
	} 
	else
	{
		sub=ssd->channel_head[channel].subs_w_head;
	}
	p=sub;

	while (sub!=NULL)
	{
		if (sub==sub_req)
		{
			if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0))
			{
				if(ssd->parameter->ad_priority2==0)
				{
					ssd->real_time_subreq--;
				}
				
				if (sub==ssd->subs_w_head)                                                     /*½«Õâ¸ö×ÓÇëÇó´Ósub request¶ÓÁÐÖÐÉ¾³ý*/
				{
					if (ssd->subs_w_head!=ssd->subs_w_tail)
					{
						ssd->subs_w_head=sub->next_node;
						sub=ssd->subs_w_head;
						continue;
					} 
					else
					{
						ssd->subs_w_head=NULL;
						ssd->subs_w_tail=NULL;
						p=NULL;
						break;
					}
				}//if (sub==ssd->subs_w_head) 
				else
				{
					if (sub->next_node!=NULL)
					{
						p->next_node=sub->next_node;
						sub=p->next_node;
						continue;
					} 
					else
					{
						ssd->subs_w_tail=p;
						ssd->subs_w_tail->next_node=NULL;
						break;
					}
				}
			}//if ((ssd->parameter->allocation_scheme==0)&&(ssd->parameter->dynamic_allocation==0)) 
			else
			{
				if (sub==ssd->channel_head[channel].subs_w_head)                               /*½«Õâ¸ö×ÓÇëÇó´Óchannel¶ÓÁÐÖÐÉ¾³ý*/
				{
					if (ssd->channel_head[channel].subs_w_head!=ssd->channel_head[channel].subs_w_tail)
					{
						ssd->channel_head[channel].subs_w_head=sub->next_node;
						sub=ssd->channel_head[channel].subs_w_head;
						continue;;
					} 
					else
					{
						ssd->channel_head[channel].subs_w_head=NULL;
						ssd->channel_head[channel].subs_w_tail=NULL;
						p=NULL;
						break;
					}
				}//if (sub==ssd->channel_head[channel].subs_w_head)
				else
				{
					if (sub->next_node!=NULL)
					{
						p->next_node=sub->next_node;
						sub=p->next_node;
						continue;
					} 
					else
					{
						ssd->channel_head[channel].subs_w_tail=p;
						ssd->channel_head[channel].subs_w_tail->next_node=NULL;
						break;
					}
				}//else
			}//else
		}//if (sub==sub_req)
		p=sub;
		sub=sub->next_node;
	}//while (sub!=NULL)

	return ssd;
}


struct ssd_info *un_greed_interleave_copyback(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request *sub1,struct sub_request *sub2)
{
	unsigned int old_ppn1,ppn1,old_ppn2,ppn2,greed_flag=0;

	old_ppn1=ssd->dram->map->map_entry[sub1->lpn].pn;
	get_ppn(ssd,channel,chip,die,sub1->location->plane,sub1);                                  /*ÕÒ³öÀ´µÄppnÒ»¶¨ÊÇ·¢ÉúÔÚÓë×ÓÇëÇóÏàÍ¬µÄplaneÖÐ,²ÅÄÜÊ¹ÓÃcopyback²Ù×÷*/
	ppn1=sub1->ppn;

	old_ppn2=ssd->dram->map->map_entry[sub2->lpn].pn;
	get_ppn(ssd,channel,chip,die,sub2->location->plane,sub2);                                  /*ÕÒ³öÀ´µÄppnÒ»¶¨ÊÇ·¢ÉúÔÚÓë×ÓÇëÇóÏàÍ¬µÄplaneÖÐ,²ÅÄÜÊ¹ÓÃcopyback²Ù×÷*/
	ppn2=sub2->ppn;

	if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2==ppn2%2))
	{
		ssd->copy_back_count++;
		ssd->copy_back_count++;

		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub1->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		sub2->current_state=SR_W_TRANSFER;
		sub2->current_time=sub1->complete_time;
		sub2->next_state=SR_COMPLETE;
		sub2->next_state_predict_time=sub2->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub2->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub2->complete_time=sub2->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub2->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;

		delete_from_channel(ssd,channel,sub1);
		delete_from_channel(ssd,channel,sub2);
	} //if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2==ppn2%2))
	else if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2!=ppn2%2))
	{
		ssd->interleave_count--;
		ssd->copy_back_count++;

		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub1->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;

		delete_from_channel(ssd,channel,sub1);
	}//else if ((old_ppn1%2==ppn1%2)&&(old_ppn2%2!=ppn2%2))
	else if ((old_ppn1%2!=ppn1%2)&&(old_ppn2%2==ppn2%2))
	{
		ssd->interleave_count--;
		ssd->copy_back_count++;

		sub2->current_state=SR_W_TRANSFER;
		sub2->current_time=ssd->current_time;
		sub2->next_state=SR_COMPLETE;
		sub2->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub2->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub2->complete_time=sub2->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub2->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;

		delete_from_channel(ssd,channel,sub2);
	}//else if ((old_ppn1%2!=ppn1%2)&&(old_ppn2%2==ppn2%2))
	else
	{
		ssd->interleave_count--;

		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+2*(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;

		delete_from_channel(ssd,channel,sub1);
	}//else

	return ssd;
}


struct ssd_info *un_greed_copyback(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request *sub1)
{
	unsigned int old_ppn,ppn;

	old_ppn=ssd->dram->map->map_entry[sub1->lpn].pn;
	get_ppn(ssd,channel,chip,die,0,sub1);                                                     /*ÕÒ³öÀ´µÄppnÒ»¶¨ÊÇ·¢ÉúÔÚÓë×ÓÇëÇóÏàÍ¬µÄplaneÖÐ,²ÅÄÜÊ¹ÓÃcopyback²Ù×÷*/
	ppn=sub1->ppn;
	
	if (old_ppn%2==ppn%2)
	{
		ssd->copy_back_count++;
		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+(sub1->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
	}//if (old_ppn%2==ppn%2)
	else
	{
		sub1->current_state=SR_W_TRANSFER;
		sub1->current_time=ssd->current_time;
		sub1->next_state=SR_COMPLETE;
		sub1->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC+ssd->parameter->time_characteristics.tR+2*(ssd->parameter->subpage_page*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
		sub1->complete_time=sub1->next_state_predict_time;

		ssd->channel_head[channel].current_state=CHANNEL_TRANSFER;										
		ssd->channel_head[channel].current_time=ssd->current_time;										
		ssd->channel_head[channel].next_state=CHANNEL_IDLE;										
		ssd->channel_head[channel].next_state_predict_time=sub1->complete_time;

		ssd->channel_head[channel].chip_head[chip].current_state=CHIP_WRITE_BUSY;										
		ssd->channel_head[channel].chip_head[chip].current_time=ssd->current_time;									
		ssd->channel_head[channel].chip_head[chip].next_state=CHIP_IDLE;										
		ssd->channel_head[channel].chip_head[chip].next_state_predict_time=ssd->channel_head[channel].next_state_predict_time+ssd->parameter->time_characteristics.tPROG;
	}//else

	delete_from_channel(ssd,channel,sub1);

	return ssd;
}


/****************************************************************************************
*º¯ÊýµÄ¹¦ÄÜÊÇÔÚ´¦Àí¶Á×ÓÇëÇóµÄ¸ß¼¶ÃüÁîÊ±£¬ÐèÒªÕÒÓëone_pageÏàÆ¥ÅäµÄÁíÍâÒ»¸öpage¼´two_page
*Ã»ÓÐÕÒµ½¿ÉÒÔºÍone_pageÖ´ÐÐtwo plane»òÕßinterleave²Ù×÷µÄÒ³,ÐèÒª½«one_pageÏòºóÒÆÒ»¸ö½Úµã
*****************************************************************************************/
struct sub_request *find_interleave_twoplane_page(struct ssd_info *ssd, struct sub_request *one_page,unsigned int command)
{
	struct sub_request *two_page;

	if (one_page->current_state!=SR_WAIT)
	{
		return NULL;                                                            
	}
	if (((ssd->channel_head[one_page->location->channel].chip_head[one_page->location->chip].current_state==CHIP_IDLE)||((ssd->channel_head[one_page->location->channel].chip_head[one_page->location->chip].next_state==CHIP_IDLE)&&
		(ssd->channel_head[one_page->location->channel].chip_head[one_page->location->chip].next_state_predict_time<=ssd->current_time))))
	{
		two_page=one_page->next_node;
		if(command==TWO_PLANE)
		{
			while (two_page!=NULL)
		    {
				if (two_page->current_state!=SR_WAIT)
				{
					two_page=two_page->next_node;
				}
				else if ((one_page->location->chip==two_page->location->chip)&&(one_page->location->die==two_page->location->die)&&(one_page->location->block==two_page->location->block)&&(one_page->location->page==two_page->location->page))
				{
					if (one_page->location->plane!=two_page->location->plane)
					{
						return two_page;                                                       /*ÕÒµ½ÁËÓëone_page¿ÉÒÔÖ´ÐÐtwo plane²Ù×÷µÄÒ³*/
					}
					else
					{
						two_page=two_page->next_node;
					}
				}
				else
				{
					two_page=two_page->next_node;
				}
		     }//while (two_page!=NULL)
		    if (two_page==NULL)                                                               /*Ã»ÓÐÕÒµ½¿ÉÒÔºÍone_pageÖ´ÐÐtwo_plane²Ù×÷µÄÒ³,ÐèÒª½«one_pageÏòºóÒÆÒ»¸ö½Úµã*/
		    {
				return NULL;
			}
		}//if(command==TWO_PLANE)
		else if(command==INTERLEAVE)
		{
			while (two_page!=NULL)
		    {
				if (two_page->current_state!=SR_WAIT)
				{
					two_page=two_page->next_node;
				}
				else if ((one_page->location->chip==two_page->location->chip)&&(one_page->location->die!=two_page->location->die))
				{
					return two_page;                                                           /*ÕÒµ½ÁËÓëone_page¿ÉÒÔÖ´ÐÐinterleave²Ù×÷µÄÒ³*/
				}
				else
				{
					two_page=two_page->next_node;
				}
		     }
		    if (two_page==NULL)                                                                /*Ã»ÓÐÕÒµ½¿ÉÒÔºÍone_pageÖ´ÐÐinterleave²Ù×÷µÄÒ³,ÐèÒª½«one_pageÏòºóÒÆÒ»¸ö½Úµã*/
		    {
				return NULL;
			}//while (two_page!=NULL)
		}//else if(command==INTERLEAVE)
		
	} 
	{
		return NULL;
	}
}


/*************************************************************************
*ÔÚ´¦Àí¶Á×ÓÇëÇó¸ß¼¶ÃüÁîÊ±£¬ÀûÓÃÕâ¸ö»¹ÊÇ²éÕÒ¿ÉÒÔÖ´ÐÐ¸ß¼¶ÃüÁîµÄsub_request
**************************************************************************/
int find_interleave_twoplane_sub_request(struct ssd_info * ssd, unsigned int channel,struct sub_request * sub_request_one,struct sub_request * sub_request_two,unsigned int command)
{
	sub_request_one=ssd->channel_head[channel].subs_r_head;
	while (sub_request_one!=NULL)
	{
		sub_request_two=find_interleave_twoplane_page(ssd,sub_request_one,command);                /*ÕÒ³öÁ½¸ö¿ÉÒÔ×ötwo_plane»òÕßinterleaveµÄread×ÓÇëÇó£¬°üÀ¨Î»ÖÃÌõ¼þºÍÊ±¼äÌõ¼þ*/
		if (sub_request_two==NULL)
		{
			sub_request_one=sub_request_one->next_node;
		}
		else if (sub_request_two!=NULL)                                                            /*ÕÒµ½ÁËÁ½¸ö¿ÉÒÔÖ´ÐÐtwo plane²Ù×÷µÄÒ³*/
		{
			break;
		}
	}

	if (sub_request_two!=NULL)
	{
		if (ssd->request_queue!=ssd->request_tail)      
		{                                                                                         /*È·±£interleave readµÄ×ÓÇëÇóÊÇµÚÒ»¸öÇëÇóµÄ×ÓÇëÇó*/
			if ((ssd->request_queue->lsn-ssd->parameter->subpage_page)<(sub_request_one->lpn*ssd->parameter->subpage_page))  
			{
				if ((ssd->request_queue->lsn+ssd->request_queue->size+ssd->parameter->subpage_page)>(sub_request_one->lpn*ssd->parameter->subpage_page))
				{
				}
				else
				{
					sub_request_two=NULL;
				}
			}
			else
			{
				sub_request_two=NULL;
			}
		}//if (ssd->request_queue!=ssd->request_tail) 
	}//if (sub_request_two!=NULL)

	if(sub_request_two!=NULL)
	{
		return SUCCESS;
	}
	else
	{
		return FAILURE;
	}

}


/**************************************************************************
*Õâ¸öº¯Êý·Ç³£ÖØÒª£¬¶Á×ÓÇëÇóµÄ×´Ì¬×ª±ä£¬ÒÔ¼°Ê±¼äµÄ¼ÆËã¶¼Í¨¹ýÕâ¸öº¯ÊýÀ´´¦Àí
*»¹ÓÐÐ´×ÓÇëÇóµÄÖ´ÐÐÆÕÍ¨ÃüÁîÊ±µÄ×´Ì¬£¬ÒÔ¼°Ê±¼äµÄ¼ÆËãÒ²ÊÇÍ¨¹ýÕâ¸öº¯ÊýÀ´´¦ÀíµÄ
****************************************************************************/
Status go_one_step(struct ssd_info * ssd, struct sub_request * sub1,struct sub_request *sub2, unsigned int aim_state,unsigned int command)
{
	unsigned int i=0,j=0,k=0,m=0;
	long long time=0;
	struct sub_request * sub=NULL ; 
	struct sub_request * sub_twoplane_one=NULL, * sub_twoplane_two=NULL;
	struct sub_request * sub_interleave_one=NULL, * sub_interleave_two=NULL;
	struct local * location=NULL;
	if(sub1==NULL)
	{
		return ERROR;
	}
	
	/***************************************************************************************************
	*´¦ÀíÆÕÍ¨ÃüÁîÊ±£¬¶Á×ÓÇëÇóµÄÄ¿±ê×´Ì¬·ÖÎªÒÔÏÂ¼¸ÖÖÇé¿öSR_R_READ£¬SR_R_C_A_TRANSFER£¬SR_R_DATA_TRANSFER
	*Ð´×ÓÇëÇóµÄÄ¿±ê×´Ì¬Ö»ÓÐSR_W_TRANSFER
	****************************************************************************************************/
	if(command==NORMAL)
	{
		sub=sub1;
		location=sub1->location;
		switch(aim_state)						
		{	
			case SR_R_READ:
			{   
				/*****************************************************************************************************
			    *Õâ¸öÄ¿±ê×´Ì¬ÊÇÖ¸flash´¦ÓÚ¶ÁÊý¾ÝµÄ×´Ì¬£¬subµÄÏÂÒ»×´Ì¬¾ÍÓ¦¸ÃÊÇ´«ËÍÊý¾ÝSR_R_DATA_TRANSFER
			    *ÕâÊ±ÓëchannelÎÞ¹Ø£¬Ö»ÓëchipÓÐ¹ØËùÒÔÒªÐÞ¸ÄchipµÄ×´Ì¬ÎªCHIP_READ_BUSY£¬ÏÂÒ»¸ö×´Ì¬¾ÍÊÇCHIP_DATA_TRANSFER
			    ******************************************************************************************************/
				sub->current_time=ssd->current_time;
				sub->current_state=SR_R_READ;
				sub->next_state=SR_R_DATA_TRANSFER;
				sub->next_state_predict_time=ssd->current_time+ssd->parameter->time_characteristics.tR;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_READ_BUSY;
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_DATA_TRANSFER;
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+ssd->parameter->time_characteristics.tR;

				break;
			}
			case SR_R_C_A_TRANSFER:
			{   
				/*******************************************************************************************************
				*Ä¿±ê×´Ì¬ÊÇÃüÁîµØÖ·´«ÊäÊ±£¬subµÄÏÂÒ»¸ö×´Ì¬¾ÍÊÇSR_R_READ
				*Õâ¸ö×´Ì¬Óëchannel£¬chipÓÐ¹Ø£¬ËùÒÔÒªÐÞ¸Ächannel£¬chipµÄ×´Ì¬·Ö±ðÎªCHANNEL_C_A_TRANSFER£¬CHIP_C_A_TRANSFER
				*ÏÂÒ»×´Ì¬·Ö±ðÎªCHANNEL_IDLE£¬CHIP_READ_BUSY
				*******************************************************************************************************/
				sub->current_time=ssd->current_time;									
				sub->current_state=SR_R_C_A_TRANSFER;									
				sub->next_state=SR_R_READ;									
				sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;									
				sub->begin_time=ssd->current_time;

				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=sub->ppn;
				ssd->read_count++;

				ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
				ssd->channel_head[location->channel].current_time=ssd->current_time;										
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
				ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC;
				
				break;
			
			}
			case SR_R_DATA_TRANSFER:
			{   
				/**************************************************************************************************************
				*Ä¿±ê×´Ì¬ÊÇÊý¾Ý´«ÊäÊ±£¬subµÄÏÂÒ»¸ö×´Ì¬¾ÍÊÇÍê³É×´Ì¬SR_COMPLETE
				*Õâ¸ö×´Ì¬µÄ´¦ÀíÒ²Óëchannel£¬chipÓÐ¹Ø£¬ËùÒÔchannel£¬chipµÄµ±Ç°×´Ì¬±äÎªCHANNEL_DATA_TRANSFER£¬CHIP_DATA_TRANSFER
				*ÏÂÒ»¸ö×´Ì¬·Ö±ðÎªCHANNEL_IDLE£¬CHIP_IDLE¡£
				***************************************************************************************************************/
				sub->current_time=ssd->current_time;					
				sub->current_state=SR_R_DATA_TRANSFER;		
				sub->next_state=SR_COMPLETE;				
				sub->next_state_predict_time=ssd->current_time+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub->complete_time=sub->next_state_predict_time;

				ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
				ssd->channel_head[location->channel].current_time=ssd->current_time;		
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
				ssd->channel_head[location->channel].next_state_predict_time=sub->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;

				break;
			}
			case SR_W_TRANSFER:
			{
				/******************************************************************************************************
				*ÕâÊÇ´¦ÀíÐ´×ÓÇëÇóÊ±£¬×´Ì¬µÄ×ª±äÒÔ¼°Ê±¼äµÄ¼ÆËã
				*ËäÈ»Ð´×ÓÇëÇóµÄ´¦Àí×´Ì¬Ò²Ïñ¶Á×ÓÇëÇóÄÇÃ´¶à£¬µ«ÊÇÐ´ÇëÇó¶¼ÊÇ´ÓÉÏÍùplaneÖÐ´«ÊäÊý¾Ý
				*ÕâÑù¾Í¿ÉÒÔ°Ñ¼¸¸ö×´Ì¬µ±Ò»¸ö×´Ì¬À´´¦Àí£¬¾Íµ±³ÉSR_W_TRANSFERÕâ¸ö×´Ì¬À´´¦Àí£¬subµÄÏÂÒ»¸ö×´Ì¬¾ÍÊÇÍê³É×´Ì¬ÁË
				*´ËÊ±channel£¬chipµÄµ±Ç°×´Ì¬±äÎªCHANNEL_TRANSFER£¬CHIP_WRITE_BUSY
				*ÏÂÒ»¸ö×´Ì¬±äÎªCHANNEL_IDLE£¬CHIP_IDLE
				*******************************************************************************************************/
				sub->current_time=ssd->current_time;
				sub->current_state=SR_W_TRANSFER;
				sub->next_state=SR_COMPLETE;
				sub->next_state_predict_time=ssd->current_time+7*ssd->parameter->time_characteristics.tWC+(sub->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tWC;
				sub->complete_time=sub->next_state_predict_time;		
				time=sub->complete_time;
				
				ssd->channel_head[location->channel].current_state=CHANNEL_TRANSFER;										
				ssd->channel_head[location->channel].current_time=ssd->current_time;										
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;										
				ssd->channel_head[location->channel].next_state_predict_time=time;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_WRITE_BUSY;										
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;									
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;										
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=time+ssd->parameter->time_characteristics.tPROG;
				
				break;
			}
			default :  return ERROR;
			
		}//switch(aim_state)	
	}//if(command==NORMAL)
	else if(command==TWO_PLANE)
	{   
		/**********************************************************************************************
		*¸ß¼¶ÃüÁîTWO_PLANEµÄ´¦Àí£¬ÕâÀïµÄTWO_PLANE¸ß¼¶ÃüÁîÊÇ¶Á×ÓÇëÇóµÄ¸ß¼¶ÃüÁî
		*×´Ì¬×ª±äÓëÆÕÍ¨ÃüÁîÒ»Ñù£¬²»Í¬µÄÊÇÔÚSR_R_C_A_TRANSFERÊ±¼ÆËãÊ±¼äÊÇ´®ÐÐµÄ£¬ÒòÎª¹²ÓÃÒ»¸öÍ¨µÀchannel
		*»¹ÓÐSR_R_DATA_TRANSFERÒ²ÊÇ¹²ÓÃÒ»¸öÍ¨µÀ
		**********************************************************************************************/
		if((sub1==NULL)||(sub2==NULL))
		{
			return ERROR;
		}
		sub_twoplane_one=sub1;
		sub_twoplane_two=sub2;
		location=sub1->location;
		
		switch(aim_state)						
		{	
			case SR_R_C_A_TRANSFER:
			{
				sub_twoplane_one->current_time=ssd->current_time;									
				sub_twoplane_one->current_state=SR_R_C_A_TRANSFER;									
				sub_twoplane_one->next_state=SR_R_READ;									
				sub_twoplane_one->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;									
				sub_twoplane_one->begin_time=ssd->current_time;

				ssd->channel_head[sub_twoplane_one->location->channel].chip_head[sub_twoplane_one->location->chip].die_head[sub_twoplane_one->location->die].plane_head[sub_twoplane_one->location->plane].add_reg_ppn=sub_twoplane_one->ppn;
				ssd->read_count++;

				sub_twoplane_two->current_time=ssd->current_time;									
				sub_twoplane_two->current_state=SR_R_C_A_TRANSFER;									
				sub_twoplane_two->next_state=SR_R_READ;									
				sub_twoplane_two->next_state_predict_time=sub_twoplane_one->next_state_predict_time;									
				sub_twoplane_two->begin_time=ssd->current_time;

				ssd->channel_head[sub_twoplane_two->location->channel].chip_head[sub_twoplane_two->location->chip].die_head[sub_twoplane_two->location->die].plane_head[sub_twoplane_two->location->plane].add_reg_ppn=sub_twoplane_two->ppn;
				ssd->read_count++;
				ssd->m_plane_read_count++;

				ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
				ssd->channel_head[location->channel].current_time=ssd->current_time;										
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
				ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

				
				break;
			}
			case SR_R_DATA_TRANSFER:
			{
				sub_twoplane_one->current_time=ssd->current_time;					
				sub_twoplane_one->current_state=SR_R_DATA_TRANSFER;		
				sub_twoplane_one->next_state=SR_COMPLETE;				
				sub_twoplane_one->next_state_predict_time=ssd->current_time+(sub_twoplane_one->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub_twoplane_one->complete_time=sub_twoplane_one->next_state_predict_time;
				
				sub_twoplane_two->current_time=sub_twoplane_one->next_state_predict_time;					
				sub_twoplane_two->current_state=SR_R_DATA_TRANSFER;		
				sub_twoplane_two->next_state=SR_COMPLETE;				
				sub_twoplane_two->next_state_predict_time=sub_twoplane_two->current_time+(sub_twoplane_two->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub_twoplane_two->complete_time=sub_twoplane_two->next_state_predict_time;
				
				ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
				ssd->channel_head[location->channel].current_time=ssd->current_time;		
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
				ssd->channel_head[location->channel].next_state_predict_time=sub_twoplane_one->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub_twoplane_one->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;
			
				break;
			}
			default :  return ERROR;
		}//switch(aim_state)	
	}//else if(command==TWO_PLANE)
	else if(command==INTERLEAVE)
	{
		if((sub1==NULL)||(sub2==NULL))
		{
			return ERROR;
		}
		sub_interleave_one=sub1;
		sub_interleave_two=sub2;
		location=sub1->location;
		
		switch(aim_state)						
		{	
			case SR_R_C_A_TRANSFER:
			{
				sub_interleave_one->current_time=ssd->current_time;									
				sub_interleave_one->current_state=SR_R_C_A_TRANSFER;									
				sub_interleave_one->next_state=SR_R_READ;									
				sub_interleave_one->next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;									
				sub_interleave_one->begin_time=ssd->current_time;

				ssd->channel_head[sub_interleave_one->location->channel].chip_head[sub_interleave_one->location->chip].die_head[sub_interleave_one->location->die].plane_head[sub_interleave_one->location->plane].add_reg_ppn=sub_interleave_one->ppn;
				ssd->read_count++;

				sub_interleave_two->current_time=ssd->current_time;									
				sub_interleave_two->current_state=SR_R_C_A_TRANSFER;									
				sub_interleave_two->next_state=SR_R_READ;									
				sub_interleave_two->next_state_predict_time=sub_interleave_one->next_state_predict_time;									
				sub_interleave_two->begin_time=ssd->current_time;

				ssd->channel_head[sub_interleave_two->location->channel].chip_head[sub_interleave_two->location->chip].die_head[sub_interleave_two->location->die].plane_head[sub_interleave_two->location->plane].add_reg_ppn=sub_interleave_two->ppn;
				ssd->read_count++;
				ssd->interleave_read_count++;

				ssd->channel_head[location->channel].current_state=CHANNEL_C_A_TRANSFER;									
				ssd->channel_head[location->channel].current_time=ssd->current_time;										
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;								
				ssd->channel_head[location->channel].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_C_A_TRANSFER;								
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;						
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_READ_BUSY;							
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=ssd->current_time+14*ssd->parameter->time_characteristics.tWC;
				
				break;
						
			}
			case SR_R_DATA_TRANSFER:
			{
				sub_interleave_one->current_time=ssd->current_time;					
				sub_interleave_one->current_state=SR_R_DATA_TRANSFER;		
				sub_interleave_one->next_state=SR_COMPLETE;				
				sub_interleave_one->next_state_predict_time=ssd->current_time+(sub_interleave_one->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub_interleave_one->complete_time=sub_interleave_one->next_state_predict_time;
				
				sub_interleave_two->current_time=sub_interleave_one->next_state_predict_time;					
				sub_interleave_two->current_state=SR_R_DATA_TRANSFER;		
				sub_interleave_two->next_state=SR_COMPLETE;				
				sub_interleave_two->next_state_predict_time=sub_interleave_two->current_time+(sub_interleave_two->size*ssd->parameter->subpage_capacity)*ssd->parameter->time_characteristics.tRC;			
				sub_interleave_two->complete_time=sub_interleave_two->next_state_predict_time;

				ssd->channel_head[location->channel].current_state=CHANNEL_DATA_TRANSFER;		
				ssd->channel_head[location->channel].current_time=ssd->current_time;		
				ssd->channel_head[location->channel].next_state=CHANNEL_IDLE;	
				ssd->channel_head[location->channel].next_state_predict_time=sub_interleave_two->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].current_state=CHIP_DATA_TRANSFER;				
				ssd->channel_head[location->channel].chip_head[location->chip].current_time=ssd->current_time;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state=CHIP_IDLE;			
				ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time=sub_interleave_two->next_state_predict_time;

				ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].add_reg_ppn=-1;
				
				break;
			}
			default :  return ERROR;	
		}//switch(aim_state)				
	}//else if(command==INTERLEAVE)
	else
	{
		printf("\nERROR: Unexpected command !\n" );
		return ERROR;
	}

	return SUCCESS;
}

