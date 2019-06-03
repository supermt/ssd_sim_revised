#ifndef INITIALIZE_H
#define INITIALIZE_H 10000

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include "avlTree.h"

#define SECTOR 512
#define BUFSIZE 200

#define DYNAMIC_ALLOCATION 0
#define STATIC_ALLOCATION 1

#define DEFAULT_CONFIG_PATH "page_config/page.parameters"

#define INTERLEAVE 0
#define TWO_PLANE 1

#define NORMAL 2
#define INTERLEAVE_TWO_PLANE 3
#define COPY_BACK 4

#define AD_RANDOM 1
#define AD_COPYBACK 2
#define AD_TWOPLANE 4
#define AD_INTERLEAVE 8
#define AD_TWOPLANE_READ 16

#define READ 1
#define WRITE 0


#define CHANNEL_IDLE 000
#define CHANNEL_C_A_TRANSFER 3
#define CHANNEL_GC 4
#define CHANNEL_DATA_TRANSFER 7
#define CHANNEL_TRANSFER 8
#define CHANNEL_UNKNOWN 9

#define CHIP_IDLE 100
#define CHIP_WRITE_BUSY 101
#define CHIP_READ_BUSY 102
#define CHIP_C_A_TRANSFER 103
#define CHIP_DATA_TRANSFER 107
#define CHIP_WAIT 108
#define CHIP_ERASE_BUSY 109
#define CHIP_COPYBACK_BUSY 110
#define UNKNOWN 111

#define SR_WAIT 200
#define SR_R_C_A_TRANSFER 201
#define SR_R_READ 202
#define SR_R_DATA_TRANSFER 203
#define SR_W_C_A_TRANSFER 204
#define SR_W_DATA_TRANSFER 205
#define SR_W_TRANSFER 206
#define SR_COMPLETE 299

#define REQUEST_IN 300
#define OUTPUT 301

#define GC_WAIT 400
#define GC_ERASE_C_A 401
#define GC_COPY_BACK 402
#define GC_COMPLETE 403
#define GC_INTERRUPT 0
#define GC_UNINTERRUPT 1

#define CHANNEL(lsn) (lsn&0x0000)>>16
#define chip(lsn) (lsn&0x0000)>>16
#define die(lsn) (lsn&0x0000)>>16
#define PLANE(lsn) (lsn&0x0000)>>16
#define BLOKC(lsn) (lsn&0x0000)>>16
#define PAGE(lsn) (lsn&0x0000)>>16
#define SUBPAGE(lsn) (lsn&0x0000)>>16

#define PG_SUB 0xffffffff


#define TRUE 1
#define FALSE 0
#define SUCCESS 1
#define FAILURE 0
#define ERROR -1
#define INFEASIBLE -2
#define OVERFLOW -3
typedef int Status;

#define MAX_FILE_NAME 100

struct configuration {
    char tracename[MAX_FILE_NAME];
    char parameter[MAX_FILE_NAME];
};

struct ac_time_characteristics {
    int tPROG;
    int tDBSY;
    int tBERS;
    int tCLS;
    int tCLH;
    int tCS;
    int tCH;
    int tWP;
    int tALS;
    int tALH;
    int tDS;
    int tDH;
    int tWC;
    int tWH;
    int tADL;
    int tR;
    int tAR;
    int tCLR;
    int tRR;
    int tRP;
    int tWB;
    int tRC;
    int tREA;
    int tCEA;
    int tRHZ;
    int tCHZ;
    int tRHOH;
    int tRLOH;
    int tCOH;
    int tREH;
    int tIR;
    int tRHW;
    int tWHR;
    int tRST;
} ac_timing;


struct ssd_info {
    double ssd_energy;
    int64_t current_time;
    int64_t next_request_time;
    unsigned int real_time_subreq;
    int flag;
    int active_flag;
    unsigned int page;

    unsigned int token;
    unsigned int gc_request;

    unsigned int write_request_count;
    unsigned int read_request_count;
    int64_t write_avg;
    int64_t read_avg;

    unsigned int min_lsn;
    unsigned int max_lsn;
    unsigned long read_count;
    unsigned long program_count;
    unsigned long erase_count;
    unsigned long direct_erase_count;
    unsigned long copy_back_count;
    unsigned long m_plane_read_count;
    unsigned long m_plane_prog_count;
    unsigned long interleave_count;
    unsigned long interleave_read_count;
    unsigned long inter_mplane_count;
    unsigned long inter_mplane_prog_count;
    unsigned long interleave_erase_count;
    unsigned long mplane_erase_conut;
    unsigned long interleave_mplane_erase_count;
    unsigned long gc_copy_back;
    unsigned long write_flash_count;
    unsigned long waste_page_count;
    float ave_read_size;
    float ave_write_size;
    unsigned int request_queue_length;
    unsigned int update_read_count;
    unsigned int gc_bg_time;
    unsigned int gc_fg_time;
    unsigned int gc_copypage;
    unsigned int min_wear;
    unsigned int min_wear_channel;
    unsigned int min_wear_chip;
    unsigned int min_wear_die;
    unsigned int min_wear_plane;
    unsigned int min_wear_block;
    double aged_valid_ratio;
    double aged_free_ratio;
    char parameterfilename[MAX_FILE_NAME];
    char tracefilename[MAX_FILE_NAME];
    char outputfilename[MAX_FILE_NAME];
    char statisticfilename[MAX_FILE_NAME];
    char statisticfilename2[MAX_FILE_NAME];
    unsigned int tmp_count;
    unsigned int total_gc;
    unsigned int max_lpn[29];
    unsigned int tracenumber;
    FILE *outputfile;
    FILE *tracefile;
    FILE *statisticfile;
    FILE *statisticfile2;

    struct parameter_value *parameter;
    struct dram_info *dram;
    struct request *request_queue;
    struct request *request_tail;
    struct sub_request *subs_w_head;
    struct sub_request *subs_w_tail;
    struct event_node *event;
    struct channel_info *channel_head;
};


struct channel_info {
    int chip;
    unsigned long read_count;
    unsigned long program_count;
    unsigned long erase_count;
    unsigned int token;

    int current_state;
    int next_state;
    int64_t current_time;
    int64_t next_state_predict_time;

    struct event_node *event;
    struct sub_request *subs_r_head;
    struct sub_request *subs_r_tail;
    struct sub_request *subs_w_head;
    struct sub_request *subs_w_tail;
    struct gc_operation *gc_command;
    struct chip_info *chip_head;
};


struct chip_info {
    unsigned int die_num;
    unsigned int plane_num_die;
    unsigned int block_num_plane;
    unsigned int page_num_block;
    unsigned int subpage_num_page;
    unsigned int ers_limit;
    unsigned int token;

    int current_state;
    int next_state;
    int64_t current_time;
    int64_t next_state_predict_time;

    unsigned long read_count;
    unsigned long program_count;
    unsigned long erase_count;

    struct ac_time_characteristics ac_timing;
    struct die_info *die_head;
};


struct die_info {

    unsigned int token;
    struct plane_info *plane_head;

};


struct plane_info {
    int add_reg_ppn;
    unsigned int free_page;
    unsigned int ers_invalid;
    unsigned int active_block;
    int can_erase_block;
    struct direct_erase *erase_node;
    struct blk_info *blk_head;
};


struct blk_info {
    unsigned int erase_count;
    unsigned int free_page_num;
    unsigned int invalid_page_num;
    int last_write_page;
    struct page_info *page_head;
};


struct page_info {
    int valid_state;
    int free_state;
    unsigned int lpn;
    unsigned int written_count;
    unsigned int make_aged;
};


struct dram_info {
    unsigned int dram_capacity;
    int64_t current_time;

    struct dram_parameter *dram_paramters;
    struct map_info *map;
    struct buffer_info *buffer;

};

typedef struct buffer_group {
    TREE_NODE node;
    struct buffer_group *LRU_link_next;
    struct buffer_group *LRU_link_pre;

    unsigned int group;
    unsigned int stored;
    unsigned int dirty_clean;
    int flag;
} buf_node;


struct dram_parameter {
    float active_current;
    float sleep_current;
    float voltage;
    int clock_time;
};


struct map_info {
    struct entry *map_entry;
    struct buffer_info *attach_info;
};


struct controller_info {
    unsigned int frequency;
    int64_t clock_time;
    float power;
};


struct request {
    int64_t time;
    unsigned int lsn;
    unsigned int size;
    unsigned int operation;

    unsigned int *need_distr_flag;
    unsigned int complete_lsn_count;

    int distri_flag;

    int64_t begin_time;
    int64_t response_time;
    double energy_consumption;

    struct sub_request *subs;
    struct request *next_node;
};


struct sub_request {
    unsigned int lpn;
    unsigned int ppn;
    unsigned int operation;
    int size;

    unsigned int current_state;
    int64_t current_time;
    unsigned int next_state;
    int64_t next_state_predict_time;
    unsigned int state;


    int64_t begin_time;
    int64_t complete_time;

    struct local *location;
    struct sub_request *next_subs;
    struct sub_request *next_node;
    struct sub_request *update;
};


struct event_node {
    int type;
    int64_t predict_time;
    struct event_node *next_node;
    struct event_node *pre_node;
};

struct parameter_value {
    unsigned int chip_num;
    unsigned int dram_capacity;
    unsigned int cpu_sdram;

    unsigned int channel_number;
    unsigned int chip_channel[100];

    unsigned int die_chip;
    unsigned int plane_die;
    unsigned int block_plane;
    unsigned int page_block;
    unsigned int subpage_page;

    unsigned int page_capacity;
    unsigned int subpage_capacity;


    unsigned int ers_limit;
    int address_mapping;
    int wear_leveling;
    int gc;
    int clean_in_background;
    int alloc_pool;
    float overprovide;
    float gc_threshold;

    double operating_current;
    double supply_voltage;
    double dram_active_current;
    double dram_standby_current;
    double dram_refresh_current;
    double dram_voltage;

    int buffer_management;
    int scheduling_algorithm;
    float quick_radio;
    int related_mapping;

    unsigned int time_step;
    unsigned int small_large_write;

    int striping;
    int interleaving;
    int pipelining;
    int threshold_fixed_adjust;
    int threshold_value;
    int active_write;
    float gc_hard_threshold;
    int allocation_scheme;
    int static_allocation;
    int dynamic_allocation;
    int advanced_commands;
    int ad_priority;
    int ad_priority2;
    int greed_CB_ad;
    int greed_MPW_ad;
    int aged;
    float aged_ratio;
    int queue_length;

    struct ac_time_characteristics time_characteristics;
};


struct entry {
    unsigned int pn;
    int state;
};


struct local {
    unsigned int channel;
    unsigned int chip;
    unsigned int die;
    unsigned int plane;
    unsigned int block;
    unsigned int page;
    unsigned int sub_page;
};


struct gc_info {
    int64_t begin_time;
    int copy_back_count;
    int erase_count;
    int64_t process_time;
    double energy_consumption;
};


struct direct_erase {
    unsigned int block;
    struct direct_erase *next_node;
};


struct gc_operation {
    unsigned int chip;
    unsigned int die;
    unsigned int plane;
    unsigned int block;
    unsigned int page;
    unsigned int state;
    unsigned int priority;
    struct gc_operation *next_node;
};


typedef struct Dram_write_map {
    unsigned int state;
} Dram_write_map;


struct ssd_info *initiation(struct ssd_info *, struct configuration *config, unsigned int mcount);

struct parameter_value *load_parameters(const char *parameter_file);

struct page_info *initialize_page(struct page_info *p_page);

struct blk_info *initialize_block(struct blk_info *p_block, struct parameter_value *parameter);

struct plane_info *initialize_plane(struct plane_info *p_plane, struct parameter_value *parameter);

struct die_info *initialize_die(struct die_info *p_die, struct parameter_value *parameter, long long current_time);

struct chip_info *initialize_chip(struct chip_info *p_chip, struct parameter_value *parameter, long long current_time);

struct ssd_info *initialize_channels(struct ssd_info *ssd);

struct dram_info *initialize_dram(struct ssd_info *ssd);

#endif
