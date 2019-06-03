# 18 "pagemap.h"
#ifndef PAGEMAP_H
#define PAGEMAP_H 10000

#include <sys/types.h>
#include "../includes/initialize.h"

#define MAX_INT64 0x7fffffffffffffffll

void file_assert(int error,char *s);
void alloc_assert(void *p,char *s);
void trace_assert(int64_t time_t,int device,unsigned int lsn,int size,int ope);

struct local *find_location(struct ssd_info *ssd,unsigned int ppn);
unsigned int find_ppn(struct ssd_info * ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int block,unsigned int page);
struct ssd_info *pre_process_page(struct ssd_info *ssd);
unsigned int get_ppn_for_pre_process(struct ssd_info *ssd,unsigned int lsn);
struct ssd_info *get_ppn(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct sub_request *sub);
unsigned int gc(struct ssd_info *ssd,unsigned int channel, unsigned int flag);
int gc_direct_erase(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);
int uninterrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);
int interrupt_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,struct gc_operation *gc_node);
int decide_gc_invoke(struct ssd_info *ssd, unsigned int channel);
int set_entry_state(struct ssd_info *ssd,unsigned int lsn,unsigned int size);
unsigned int get_ppn_for_gc(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);

int erase_operation(struct ssd_info * ssd,unsigned int channel ,unsigned int chip ,unsigned int die,unsigned int plane ,unsigned int block);
int erase_planes(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1,unsigned int command);
int move_page(struct ssd_info * ssd, struct local *location,unsigned int * transfer_size);
int gc_for_channel(struct ssd_info *ssd, unsigned int channel);
int delete_gc_node(struct ssd_info *ssd, unsigned int channel,struct gc_operation *gc_node);

#endif
