
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include "includes/initialize.h"
#include "ssd_stat/flash.h"
#include "ssd_stat/pagemap.h"

#define MAX_INT64 0x7fffffffffffffffll

struct ssd_info *simulate(struct ssd_info *);
int get_requests(struct ssd_info *);
struct ssd_info *buffer_management(struct ssd_info *);
unsigned int lpn2ppn(struct ssd_info * ,unsigned int lsn);
struct ssd_info *distribute(struct ssd_info *);
void trace_output(struct ssd_info* );
void statistic_output(struct ssd_info *);
unsigned int size(unsigned int);
unsigned int transfer_size(struct ssd_info *,int,unsigned int,struct request *);
int64_t find_nearest_event(struct ssd_info *);
void free_all_node(struct ssd_info *);
struct ssd_info *make_aged(struct ssd_info *);
struct ssd_info *no_buffer_distribute(struct ssd_info *);
