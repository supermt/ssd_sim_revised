
#ifndef FLASH_H
#define FLASH_H 100000

#include <stdlib.h>
#include "pagemap.h"

struct ssd_info *process(struct ssd_info *);
struct ssd_info *insert2buffer(struct ssd_info *,unsigned int,int,struct sub_request *,struct request *);

struct ssd_info *flash_page_state_modify(struct ssd_info *,struct sub_request *,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int);
struct ssd_info *make_same_level(struct ssd_info *,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int);
int find_level_page(struct ssd_info *,unsigned int,unsigned int,unsigned int,struct sub_request *,struct sub_request *);
int make_level_page(struct ssd_info * ssd, struct sub_request * sub0,struct sub_request * sub1);
struct ssd_info *compute_serve_time(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,struct sub_request **subs,unsigned int subs_count,unsigned int command);
int get_ppn_for_advanced_commands(struct ssd_info *ssd,unsigned int channel,unsigned int chip,struct sub_request * * subs ,unsigned int subs_count,unsigned int command);
int get_ppn_for_normal_command(struct ssd_info * ssd, unsigned int channel,unsigned int chip,struct sub_request * sub);
struct ssd_info *dynamic_advanced_process(struct ssd_info *ssd,unsigned int channel,unsigned int chip);

struct sub_request *find_two_plane_page(struct ssd_info *, struct sub_request *);
struct sub_request *find_interleave_read_page(struct ssd_info *, struct sub_request *);
int find_twoplane_write_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub_twoplane_one,struct sub_request * sub_twoplane_two);
int find_interleave_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub_interleave_one,struct sub_request * sub_interleave_two);
struct sub_request * find_read_sub_request(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die);
struct sub_request * find_write_sub_request(struct ssd_info * ssd, unsigned int channel);
struct sub_request * creat_sub_request(struct ssd_info * ssd,unsigned int lpn,int size,unsigned int state,struct request * req,unsigned int operation);

struct sub_request *find_interleave_twoplane_page(struct ssd_info *ssd, struct sub_request *onepage,unsigned int command);
int find_interleave_twoplane_sub_request(struct ssd_info * ssd, unsigned int channel,struct sub_request * sub_request_one,struct sub_request * sub_request_two,unsigned int command);

struct ssd_info *delete_from_channel(struct ssd_info *ssd,unsigned int channel,struct sub_request * sub_req);
struct ssd_info *un_greed_interleave_copyback(struct ssd_info *,unsigned int,unsigned int,unsigned int,struct sub_request *,struct sub_request *);
struct ssd_info *un_greed_copyback(struct ssd_info *,unsigned int,unsigned int,unsigned int,struct sub_request *);
int find_active_block(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane);
int write_page(struct ssd_info *ssd,unsigned int channel,unsigned int chip,unsigned int die,unsigned int plane,unsigned int active_block,unsigned int *ppn);
int allocate_location(struct ssd_info * ssd ,struct sub_request *sub_req);


int go_one_step(struct ssd_info * ssd, struct sub_request * sub1,struct sub_request *sub2, unsigned int aim_state,unsigned int command);
int services_2_r_cmd_trans_and_complete(struct ssd_info * ssd);
int services_2_r_wait(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag);

int services_2_r_data_trans(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag);
int services_2_write(struct ssd_info * ssd,unsigned int channel,unsigned int * channel_busy_flag, unsigned int * change_current_time_flag);
int delete_w_sub_request(struct ssd_info * ssd, unsigned int channel, struct sub_request * sub );
int copy_back(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die,struct sub_request * sub);
int static_write(struct ssd_info * ssd, unsigned int channel,unsigned int chip, unsigned int die,struct sub_request * sub);

#endif
