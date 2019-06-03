#define _CRTDBG_MAP_ALLOC

#include "pagemap.h"

void file_assert(int error, char *s)
{
 if (error == 0)
  return;
 printf("open %s error\n", s);
 getchar();
 exit(-1);
}




void alloc_assert(void *p, char *s)
{
 if (p != NULL)
  return;
 printf("malloc %s error\n", s);
 getchar();
 exit(-1);
}






void trace_assert(int64_t time_t, int device, unsigned int lsn, int size, int ope)
{
 if (time_t < 0 || device < 0 || lsn < 0 || size < 0 || ope < 0)
 {
  printf("trace error:%lld %d %d %d %d\n", time_t, device, lsn, size, ope);
  getchar();
  exit(-1);
 }
 if (time_t == 0 && device == 0 && lsn == 0 && size == 0 && ope == 0)
 {
  printf("probable read a blank line\n");
  getchar();
 }
}





struct local *find_location(struct ssd_info *ssd, unsigned int ppn)
{
 struct local *location = NULL;
 unsigned int i = 0;
 int pn, ppn_value = ppn;
 int page_plane = 0, page_die = 0, page_chip = 0, page_channel = 0;

 pn = ppn;

#ifdef DEBUG
 printf("enter find_location\n");
#endif

 location = (struct local *)malloc(sizeof(struct local));
 alloc_assert(location, "location");
 memset(location, 0, sizeof(struct local));

 page_plane = ssd->parameter->page_block * ssd->parameter->block_plane;
 page_die = page_plane * ssd->parameter->plane_die;
 page_chip = page_die * ssd->parameter->die_chip;
 page_channel = page_chip * ssd->parameter->chip_channel[0];





 location->channel = ppn / page_channel;
 location->chip = (ppn % page_channel) / page_chip;
 location->die = ((ppn % page_channel) % page_chip) / page_die;
 location->plane = (((ppn % page_channel) % page_chip) % page_die) / page_plane;
 location->block = ((((ppn % page_channel) % page_chip) % page_die) % page_plane) / ssd->parameter->page_block;
 location->page = (((((ppn % page_channel) % page_chip) % page_die) % page_plane) % ssd->parameter->page_block) % ssd->parameter->page_block;

 return location;
}





unsigned int find_ppn(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int block, unsigned int page)
{
 unsigned int ppn = 0;
 unsigned int i = 0;
 int page_plane = 0, page_die = 0, page_chip = 0;
 int page_channel[100];

#ifdef DEBUG
 printf("enter find_psn,channel:%d, chip:%d, die:%d, plane:%d, block:%d, page:%d\n", channel, chip, die, plane, block, page);
#endif




 page_plane = ssd->parameter->page_block * ssd->parameter->block_plane;
 page_die = page_plane * ssd->parameter->plane_die;
 page_chip = page_die * ssd->parameter->die_chip;
 while (i < ssd->parameter->channel_number)
 {
  page_channel[i] = ssd->parameter->chip_channel[i] * page_chip;
  i++;
 }




 i = 0;
 while (i < channel)
 {
  ppn = ppn + page_channel[i];
  i++;
 }
 ppn = ppn + page_chip * chip + page_die * die + page_plane * plane + block * ssd->parameter->page_block + page;

 return ppn;
}




int set_entry_state(struct ssd_info *ssd, unsigned int lsn, unsigned int size)
{
 int temp, state, move;

 temp = ~(0xffffffff << size);
 move = lsn % ssd->parameter->subpage_page;
 state = temp << move;

 return state;
}

struct ssd_info *pre_process_page(struct ssd_info *ssd)
{
 int fl = 0;
 unsigned int device, lsn, size, ope, lpn, full_page;
 unsigned int largest_lsn, sub_size, ppn, add_size = 0;
 unsigned int i = 0, j, k;
 int map_entry_new, map_entry_old, modify;
 int flag = 0;
 char buffer_request[200];
 struct local *location;
 int64_t time;

 printf("\n");
 printf("begin pre_process_page.................\n");

 ssd->tracefile = fopen(ssd->tracefilename, "r");
 if (ssd->tracefile == NULL)
 {
  printf("the trace file can't open\n");
  return NULL;
 }

 full_page = ~(0xffffffff << (ssd->parameter->subpage_page));
 largest_lsn = (unsigned int)((ssd->parameter->chip_num * ssd->parameter->die_chip * ssd->parameter->plane_die * ssd->parameter->block_plane * ssd->parameter->page_block * ssd->parameter->subpage_page) * (1 - ssd->parameter->overprovide));

 while (fgets(buffer_request, 200, ssd->tracefile))
 {
  sscanf(buffer_request, "%lld %d %d %d %d", &time, &device, &lsn, &size, &ope);
  fl++;
  trace_assert(time, device, lsn, size, ope);

  add_size = 0;
  if (ope == 1)
  {
   while (add_size < size)
   {
    lsn = lsn % largest_lsn;
    sub_size = ssd->parameter->subpage_page - (lsn % ssd->parameter->subpage_page);
    if (add_size + sub_size >= size)
    {
     sub_size = size - add_size;
     add_size += sub_size;
    }

    if ((sub_size > ssd->parameter->subpage_page) || (add_size > size))
    {
     printf("pre_process sub_size:%d\n", sub_size);
    }







    lpn = lsn / ssd->parameter->subpage_page;
    if (ssd->dram->map->map_entry[lpn].state == 0)
    {




     ppn = get_ppn_for_pre_process(ssd, lsn);
     location = find_location(ssd, ppn);
     ssd->program_count++;
     ssd->channel_head[location->channel].program_count++;
     ssd->channel_head[location->channel].chip_head[location->chip].program_count++;
     ssd->dram->map->map_entry[lpn].pn = ppn;
     ssd->dram->map->map_entry[lpn].state = set_entry_state(ssd, lsn, sub_size);
     ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn = lpn;
     ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state = ssd->dram->map->map_entry[lpn].state;
     ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state = ((~ssd->dram->map->map_entry[lpn].state) & full_page);

     free(location);
     location = NULL;
    }
    else if (ssd->dram->map->map_entry[lpn].state > 0)
    {
     map_entry_new = set_entry_state(ssd, lsn, sub_size);
     map_entry_old = ssd->dram->map->map_entry[lpn].state;
     modify = map_entry_new | map_entry_old;
     ppn = ssd->dram->map->map_entry[lpn].pn;
     location = find_location(ssd, ppn);

     ssd->program_count++;
     ssd->channel_head[location->channel].program_count++;
     ssd->channel_head[location->channel].chip_head[location->chip].program_count++;
     ssd->dram->map->map_entry[lsn / ssd->parameter->subpage_page].state = modify;
     ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state = modify;
     ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state = ((~modify) & full_page);

     free(location);
     location = NULL;
    }
    lsn = lsn + sub_size;
    add_size += sub_size;
   }
  }
 }

 printf("\n");
 printf("pre_process is complete!\n");

 fclose(ssd->tracefile);

 for (i = 0; i < ssd->parameter->channel_number; i++)
  for (j = 0; j < ssd->parameter->die_chip; j++)
   for (k = 0; k < ssd->parameter->plane_die; k++)
   {
    fprintf(ssd->outputfile, "chip:%d,die:%d,plane:%d have free page: %d\n", i, j, k, ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);
    fflush(ssd->outputfile);
   }

 return ssd;
}





unsigned int get_ppn_for_pre_process(struct ssd_info *ssd, unsigned int lsn)
{
 unsigned int channel = 0, chip = 0, die = 0, plane = 0;
 unsigned int ppn, lpn;
 unsigned int active_block;
 unsigned int channel_num = 0, chip_num = 0, die_num = 0, plane_num = 0;

#ifdef DEBUG
 printf("enter get_psn_for_pre_process\n");
#endif

 channel_num = ssd->parameter->channel_number;
 chip_num = ssd->parameter->chip_channel[0];
 die_num = ssd->parameter->die_chip;
 plane_num = ssd->parameter->plane_die;
 lpn = lsn / ssd->parameter->subpage_page;

 if (ssd->parameter->allocation_scheme == 0)
 {
  if (ssd->parameter->dynamic_allocation == 0)
  {
   channel = ssd->token;
   ssd->token = (ssd->token + 1) % ssd->parameter->channel_number;
   chip = ssd->channel_head[channel].token;
   ssd->channel_head[channel].token = (chip + 1) % ssd->parameter->chip_channel[0];
   die = ssd->channel_head[channel].chip_head[chip].token;
   ssd->channel_head[channel].chip_head[chip].token = (die + 1) % ssd->parameter->die_chip;
   plane = ssd->channel_head[channel].chip_head[chip].die_head[die].token;
   ssd->channel_head[channel].chip_head[chip].die_head[die].token = (plane + 1) % ssd->parameter->plane_die;
  }
  else if (ssd->parameter->dynamic_allocation == 1)
  {
   channel = lpn % ssd->parameter->channel_number;
   chip = ssd->channel_head[channel].token;
   ssd->channel_head[channel].token = (chip + 1) % ssd->parameter->chip_channel[0];
   die = ssd->channel_head[channel].chip_head[chip].token;
   ssd->channel_head[channel].chip_head[chip].token = (die + 1) % ssd->parameter->die_chip;
   plane = ssd->channel_head[channel].chip_head[chip].die_head[die].token;
   ssd->channel_head[channel].chip_head[chip].die_head[die].token = (plane + 1) % ssd->parameter->plane_die;
  }
 }
 else if (ssd->parameter->allocation_scheme == 1)
 {
  switch (ssd->parameter->static_allocation)
  {

  case 0:
  {
   channel = (lpn / (plane_num * die_num * chip_num)) % channel_num;
   chip = lpn % chip_num;
   die = (lpn / chip_num) % die_num;
   plane = (lpn / (die_num * chip_num)) % plane_num;
   break;
  }
  case 1:
  {
   channel = lpn % channel_num;
   chip = (lpn / channel_num) % chip_num;
   die = (lpn / (chip_num * channel_num)) % die_num;
   plane = (lpn / (die_num * chip_num * channel_num)) % plane_num;

   break;
  }
  case 2:
  {
   channel = lpn % channel_num;
   chip = (lpn / (plane_num * channel_num)) % chip_num;
   die = (lpn / (plane_num * chip_num * channel_num)) % die_num;
   plane = (lpn / channel_num) % plane_num;
   break;
  }
  case 3:
  {
   channel = lpn % channel_num;
   chip = (lpn / (die_num * channel_num)) % chip_num;
   die = (lpn / channel_num) % die_num;
   plane = (lpn / (die_num * chip_num * channel_num)) % plane_num;
   break;
  }
  case 4:
  {
   channel = lpn % channel_num;
   chip = (lpn / (plane_num * die_num * channel_num)) % chip_num;
   die = (lpn / (plane_num * channel_num)) % die_num;
   plane = (lpn / channel_num) % plane_num;

   break;
  }
  case 5:
  {
   channel = lpn % channel_num;
   chip = (lpn / (plane_num * die_num * channel_num)) % chip_num;
   die = (lpn / channel_num) % die_num;
   plane = (lpn / (die_num * channel_num)) % plane_num;

   break;
  }
  case 6:
  {

   break;
  }
  case 7:
  {
   channel = (lpn / ssd->max_lpn[ssd->tracenumber]) / ssd->parameter->channel_number;
   chip = (lpn / ssd->max_lpn[ssd->tracenumber]) % ssd->parameter->channel_number;
   die = ssd->parameter->die_chip - 1;
   plane = ssd->parameter->plane_die - 1;

   break;
  }
  default:
   return 0;
  }
 }





 if (find_active_block(ssd, channel, chip, die, plane) == FAILURE)
 {

  return 0;
 }
 active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
 if (write_page(ssd, channel, chip, die, plane, active_block, &ppn) == ERROR)
 {
  return 0;
 }

 return ppn;
}





struct ssd_info *get_ppn(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, struct sub_request *sub)
{
 int old_ppn = -1;
 unsigned int ppn, lpn, full_page;
 unsigned int active_block;
 unsigned int block;
 unsigned int page, flag = 0, flag1 = 0;
 unsigned int old_state = 0, state = 0, copy_subpage = 0;
 struct local *location;
 struct direct_erase *direct_erase_node, *new_direct_erase;
 struct gc_operation *gc_node;

 unsigned int i = 0, j = 0, k = 0, l = 0, m = 0, n = 0;

#ifdef DEBUG
 printf("enter get_ppn,channel:%d, chip:%d, die:%d, plane:%d\n", channel, chip, die, plane);
#endif

 full_page = ~(0xffffffff << (ssd->parameter->subpage_page));
 lpn = sub->lpn;





 if (find_active_block(ssd, channel, chip, die, plane) == FAILURE)
 {

  return ssd;
 }

 active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;

 if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page > 63)
 {
  printf("error! the last write page larger than 64!!\n");
  while (1)
  {
  }
 }

 block = active_block;
 page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;

 if (ssd->dram->map->map_entry[lpn].state == 0)
 {
  if (ssd->dram->map->map_entry[lpn].pn != 0)
  {

  }
  ssd->dram->map->map_entry[lpn].pn = find_ppn(ssd, channel, chip, die, plane, block, page);
  ssd->dram->map->map_entry[lpn].state = sub->state;
 }
 else
 {
  ppn = ssd->dram->map->map_entry[lpn].pn;
  location = find_location(ssd, ppn);
  if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn != lpn)
  {

  }

  ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state = 0;
  ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state = 0;
  ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn = 0;
  ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;




  if (ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num == ssd->parameter->page_block)
  {
   new_direct_erase = (struct direct_erase *)malloc(sizeof(struct direct_erase));
   alloc_assert(new_direct_erase, "new_direct_erase");
   memset(new_direct_erase, 0, sizeof(struct direct_erase));

   new_direct_erase->block = location->block;
   new_direct_erase->next_node = NULL;
   direct_erase_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
   if (direct_erase_node == NULL)
   {
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node = new_direct_erase;
   }
   else
   {
    new_direct_erase->next_node = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node;
    ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].erase_node = new_direct_erase;
   }
  }

  free(location);
  location = NULL;
  ssd->dram->map->map_entry[lpn].pn = find_ppn(ssd, channel, chip, die, plane, block, page);
  ssd->dram->map->map_entry[lpn].state = (ssd->dram->map->map_entry[lpn].state | sub->state);
 }

 sub->ppn = ssd->dram->map->map_entry[lpn].pn;
 sub->location->channel = channel;
 sub->location->chip = chip;
 sub->location->die = die;
 sub->location->plane = plane;
 sub->location->block = active_block;
 sub->location->page = page;

 ssd->program_count++;
 ssd->channel_head[channel].program_count++;
 ssd->channel_head[channel].chip_head[chip].program_count++;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].lpn = lpn;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].valid_state = sub->state;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].free_state = ((~(sub->state)) & full_page);
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
 ssd->write_flash_count++;

 if (ssd->parameter->active_write == 0)
 {
  if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page < (ssd->parameter->page_block * ssd->parameter->block_plane * ssd->parameter->gc_hard_threshold))
  {
   gc_node = (struct gc_operation *)malloc(sizeof(struct gc_operation));
   alloc_assert(gc_node, "gc_node");
   memset(gc_node, 0, sizeof(struct gc_operation));

   gc_node->next_node = NULL;
   gc_node->chip = chip;
   gc_node->die = die;
   gc_node->plane = plane;
   gc_node->block = 0xffffffff;
   gc_node->page = 0;
   gc_node->state = GC_WAIT;
   gc_node->priority = GC_UNINTERRUPT;
   gc_node->next_node = ssd->channel_head[channel].gc_command;
   ssd->channel_head[channel].gc_command = gc_node;
   ssd->gc_request++;
  }
 }

 return ssd;
}




unsigned int get_ppn_for_gc(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane)
{
 unsigned int ppn;
 unsigned int active_block, block, page;

#ifdef DEBUG
 printf("enter get_ppn_for_gc,channel:%d, chip:%d, die:%d, plane:%d\n", channel, chip, die, plane);
#endif

 if (find_active_block(ssd, channel, chip, die, plane) != SUCCESS)
 {
  printf("\n\n Error int get_ppn_for_gc().\n");
  return 0xffffffff;
 }

 active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page++;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;

 if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page > 63)
 {
  printf("error! the last write page larger than 64!!\n");
  while (1)
  {
  }
 }

 block = active_block;
 page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page;

 ppn = find_ppn(ssd, channel, chip, die, plane, block, page);

 ssd->program_count++;
 ssd->channel_head[channel].program_count++;
 ssd->channel_head[channel].chip_head[chip].program_count++;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[page].written_count++;
 ssd->write_flash_count++;

 return ppn;
}
# 614 "pagemap.c"
Status erase_operation(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int block)
{
 unsigned int i = 0;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num = ssd->parameter->page_block;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num = 0;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page = -1;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_count++;
 for (i = 0; i < ssd->parameter->page_block; i++)
 {
  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state = PG_SUB;
  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state = 0;
  ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].lpn = -1;
 }
 ssd->erase_count++;
 ssd->channel_head[channel].erase_count++;
 ssd->channel_head[channel].chip_head[chip].erase_count++;
 ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page += ssd->parameter->page_block;

 return SUCCESS;
}




Status erase_planes(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die1, unsigned int plane1, unsigned int command)
{
 unsigned int die = 0;
 unsigned int plane = 0;
 unsigned int block = 0;
 struct direct_erase *direct_erase_node = NULL;
 unsigned int block0 = 0xffffffff;
 unsigned int block1 = 0;

 if ((ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node == NULL) ||
  ((command != INTERLEAVE_TWO_PLANE) && (command != INTERLEAVE) && (command != TWO_PLANE) && (command != NORMAL)))
 {
  return ERROR;
 }





 block1 = ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node->block;

 ssd->channel_head[channel].current_state = CHANNEL_TRANSFER;
 ssd->channel_head[channel].current_time = ssd->current_time;
 ssd->channel_head[channel].next_state = CHANNEL_IDLE;

 ssd->channel_head[channel].chip_head[chip].current_state = CHIP_ERASE_BUSY;
 ssd->channel_head[channel].chip_head[chip].current_time = ssd->current_time;
 ssd->channel_head[channel].chip_head[chip].next_state = CHIP_IDLE;

 if (command == INTERLEAVE_TWO_PLANE)
 {
  for (die = 0; die < ssd->parameter->die_chip; die++)
  {
   block0 = 0xffffffff;
   if (die == die1)
   {
    block0 = block1;
   }
   for (plane = 0; plane < ssd->parameter->plane_die; plane++)
   {
    direct_erase_node = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
    if (direct_erase_node != NULL)
    {

     block = direct_erase_node->block;

     if (block0 == 0xffffffff)
     {
      block0 = block;
     }
     else
     {
      if (block != block0)
      {
       continue;
      }
     }
     ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node = direct_erase_node->next_node;
     erase_operation(ssd, channel, chip, die, plane, block);
     free(direct_erase_node);
     direct_erase_node = NULL;
     ssd->direct_erase_count++;
    }
   }
  }

  ssd->interleave_mplane_erase_count++;
  ssd->channel_head[channel].next_state_predict_time = ssd->current_time + 18 * ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tWB;
  ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time - 9 * ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tBERS;
 }
 else if (command == INTERLEAVE)
 {
  for (die = 0; die < ssd->parameter->die_chip; die++)
  {
   for (plane = 0; plane < ssd->parameter->plane_die; plane++)
   {
    if (die == die1)
    {
     plane = plane1;
    }
    direct_erase_node = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
    if (direct_erase_node != NULL)
    {
     block = direct_erase_node->block;
     ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node = direct_erase_node->next_node;
     erase_operation(ssd, channel, chip, die, plane, block);
     free(direct_erase_node);
     direct_erase_node = NULL;
     ssd->direct_erase_count++;
     break;
    }
   }
  }
  ssd->interleave_erase_count++;
  ssd->channel_head[channel].next_state_predict_time = ssd->current_time + 14 * ssd->parameter->time_characteristics.tWC;
  ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tBERS;
 }
 else if (command == TWO_PLANE)
 {

  for (plane = 0; plane < ssd->parameter->plane_die; plane++)
  {
   direct_erase_node = ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node;
   if ((direct_erase_node != NULL))
   {
    block = direct_erase_node->block;
    if (block == block1)
    {
     ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane].erase_node = direct_erase_node->next_node;
     erase_operation(ssd, channel, chip, die1, plane, block);
     free(direct_erase_node);
     direct_erase_node = NULL;
     ssd->direct_erase_count++;
    }
   }
  }

  ssd->mplane_erase_conut++;
  ssd->channel_head[channel].next_state_predict_time = ssd->current_time + 14 * ssd->parameter->time_characteristics.tWC;
  ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tBERS;
 }
 else if (command == NORMAL)
 {
  direct_erase_node = ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;
  block = direct_erase_node->block;
  ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node = direct_erase_node->next_node;
  free(direct_erase_node);
  direct_erase_node = NULL;
  erase_operation(ssd, channel, chip, die1, plane1, block);

  ssd->direct_erase_count++;
  ssd->channel_head[channel].next_state_predict_time = ssd->current_time + 5 * ssd->parameter->time_characteristics.tWC;
  ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tWB + ssd->parameter->time_characteristics.tBERS;
 }
 else
 {
  return ERROR;
 }

 direct_erase_node = ssd->channel_head[channel].chip_head[chip].die_head[die1].plane_head[plane1].erase_node;

 if (((direct_erase_node) != NULL) && (direct_erase_node->block == block1))
 {
  return FAILURE;
 }
 else
 {
  return SUCCESS;
 }
}
# 800 "pagemap.c"
int gc_direct_erase(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane)
{
 unsigned int lv_die = 0, lv_plane = 0;
 unsigned int interleaver_flag = FALSE, muilt_plane_flag = FALSE;
 unsigned int normal_erase_flag = TRUE;

 struct direct_erase *direct_erase_node1 = NULL;
 struct direct_erase *direct_erase_node2 = NULL;

 direct_erase_node1 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
 if (direct_erase_node1 == NULL)
 {
  return FAILURE;
 }





 if ((ssd->parameter->advanced_commands & AD_TWOPLANE) == AD_TWOPLANE)
 {
  for (lv_plane = 0; lv_plane < ssd->parameter->plane_die; lv_plane++)
  {
   direct_erase_node2 = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node;
   if ((lv_plane != plane) && (direct_erase_node2 != NULL))
   {
    if ((direct_erase_node1->block) == (direct_erase_node2->block))
    {
     muilt_plane_flag = TRUE;
     break;
    }
   }
  }
 }





 if ((ssd->parameter->advanced_commands & AD_INTERLEAVE) == AD_INTERLEAVE)
 {
  for (lv_die = 0; lv_die < ssd->parameter->die_chip; lv_die++)
  {
   if (lv_die != die)
   {
    for (lv_plane = 0; lv_plane < ssd->parameter->plane_die; lv_plane++)
    {
     if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].erase_node != NULL)
     {
      interleaver_flag = TRUE;
      break;
     }
    }
   }
   if (interleaver_flag == TRUE)
   {
    break;
   }
  }
 }







 if ((muilt_plane_flag == TRUE) && (interleaver_flag == TRUE) && ((ssd->parameter->advanced_commands & AD_TWOPLANE) == AD_TWOPLANE) && ((ssd->parameter->advanced_commands & AD_INTERLEAVE) == AD_INTERLEAVE))
 {
  if (erase_planes(ssd, channel, chip, die, plane, INTERLEAVE_TWO_PLANE) == SUCCESS)
  {
   return SUCCESS;
  }
 }
 else if ((interleaver_flag == TRUE) && ((ssd->parameter->advanced_commands & AD_INTERLEAVE) == AD_INTERLEAVE))
 {
  if (erase_planes(ssd, channel, chip, die, plane, INTERLEAVE) == SUCCESS)
  {
   return SUCCESS;
  }
 }
 else if ((muilt_plane_flag == TRUE) && ((ssd->parameter->advanced_commands & AD_TWOPLANE) == AD_TWOPLANE))
 {
  if (erase_planes(ssd, channel, chip, die, plane, TWO_PLANE) == SUCCESS)
  {
   return SUCCESS;
  }
 }

 if ((normal_erase_flag == TRUE))
 {
  if (erase_planes(ssd, channel, chip, die, plane, NORMAL) == SUCCESS)
  {
   return SUCCESS;
  }
  else
  {
   return FAILURE;
  }
 }
 return SUCCESS;
}

Status move_page(struct ssd_info *ssd, struct local *location, unsigned int *transfer_size)
{
 struct local *new_location = NULL;
 unsigned int free_state = 0, valid_state = 0;
 unsigned int lpn = 0, old_ppn = 0, ppn = 0;

 lpn = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn;
 valid_state = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state;
 free_state = ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state;
 old_ppn = find_ppn(ssd, location->channel, location->chip, location->die, location->plane, location->block, location->page);
 ppn = get_ppn_for_gc(ssd, location->channel, location->chip, location->die, location->plane);

 new_location = find_location(ssd, ppn);

 if ((ssd->parameter->advanced_commands & AD_COPYBACK) == AD_COPYBACK)
 {
  if (ssd->parameter->greed_CB_ad == 1)
  {
   ssd->copy_back_count++;
   ssd->gc_copy_back++;
   while (old_ppn % 2 != ppn % 2)
   {
    ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state = 0;
    ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn = 0;
    ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state = 0;
    ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].invalid_page_num++;

    free(new_location);
    new_location = NULL;

    ppn = get_ppn_for_gc(ssd, location->channel, location->chip, location->die, location->plane);
    ssd->program_count--;
    ssd->write_flash_count--;
    ssd->waste_page_count++;
   }
   if (new_location == NULL)
   {
    new_location = find_location(ssd, ppn);
   }

   ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state = free_state;
   ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn = lpn;
   ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state = valid_state;
  }
  else
  {
   if (old_ppn % 2 != ppn % 2)
   {
    (*transfer_size) += size(valid_state);
   }
   else
   {

    ssd->copy_back_count++;
    ssd->gc_copy_back++;
   }
  }
 }
 else
 {
  (*transfer_size) += size(valid_state);
 }
 ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].free_state = free_state;
 ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].lpn = lpn;
 ssd->channel_head[new_location->channel].chip_head[new_location->chip].die_head[new_location->die].plane_head[new_location->plane].blk_head[new_location->block].page_head[new_location->page].valid_state = valid_state;

 ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].free_state = 0;
 ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].lpn = 0;
 ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].page_head[location->page].valid_state = 0;
 ssd->channel_head[location->channel].chip_head[location->chip].die_head[location->die].plane_head[location->plane].blk_head[location->block].invalid_page_num++;

 if (old_ppn == ssd->dram->map->map_entry[lpn].pn)
 {
  ssd->dram->map->map_entry[lpn].pn = ppn;
 }

 free(new_location);
 new_location = NULL;

 return SUCCESS;
}





int uninterrupt_gc(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane)
{
 unsigned int i = 0, invalid_page = 0;
 unsigned int block, active_block, transfer_size, free_page, page_move_count = 0;
 struct local *location = NULL;
 unsigned int total_invalid_page_num = 0;

 if (find_active_block(ssd, channel, chip, die, plane) != SUCCESS)
 {

  return ERROR;
 }
 active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;

 invalid_page = 0;
 transfer_size = 0;
 block = -1;
 for (i = 0; i < ssd->parameter->block_plane; i++)
 {
  total_invalid_page_num += ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
  if ((active_block != i) && (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num > invalid_page))
  {
   invalid_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
   block = i;
  }
 }
 if (block == -1)
 {
  return 1;
 }






 free_page = 0;
 for (i = 0; i < ssd->parameter->page_block; i++)
 {
  if ((ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state & PG_SUB) == 0x0000000f)
  {
   free_page++;
  }
  if (free_page != 0)
  {
   printf("\ntoo much free page. \t %d\t .%d\t%d\t%d\t%d\t\n", free_page, channel, chip, die, plane);
  }
  if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state > 0)
  {
   location = (struct local *)malloc(sizeof(struct local));
   alloc_assert(location, "location");
   memset(location, 0, sizeof(struct local));

   location->channel = channel;
   location->chip = chip;
   location->die = die;
   location->plane = plane;
   location->block = block;
   location->page = i;
   move_page(ssd, location, &transfer_size);
   page_move_count++;

   free(location);
   location = NULL;
  }
 }
 erase_operation(ssd, channel, chip, die, plane, block);

 ssd->channel_head[channel].current_state = CHANNEL_GC;
 ssd->channel_head[channel].current_time = ssd->current_time;
 ssd->channel_head[channel].next_state = CHANNEL_IDLE;
 ssd->channel_head[channel].chip_head[chip].current_state = CHIP_ERASE_BUSY;
 ssd->channel_head[channel].chip_head[chip].current_time = ssd->current_time;
 ssd->channel_head[channel].chip_head[chip].next_state = CHIP_IDLE;





 if ((ssd->parameter->advanced_commands & AD_COPYBACK) == AD_COPYBACK)
 {
  if (ssd->parameter->greed_CB_ad == 1)
  {

   ssd->channel_head[channel].next_state_predict_time = ssd->current_time + page_move_count * (7 * ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tR + 7 * ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tPROG);
   ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tBERS;
  }
 }
 else
 {

  ssd->channel_head[channel].next_state_predict_time = ssd->current_time + page_move_count * (7 * ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tR + 7 * ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tPROG) + transfer_size * SECTOR * (ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tRC);
  ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tBERS;
 }
 ssd->gc_copypage += page_move_count;
 return 1;
}





int interrupt_gc(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, struct gc_operation *gc_node)
{
 unsigned int i, block, active_block, transfer_size, invalid_page = 0;
 struct local *location;

 active_block = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].active_block;
 transfer_size = 0;
 if (gc_node->block >= ssd->parameter->block_plane)
 {
  for (i = 0; i < ssd->parameter->block_plane; i++)
  {
   if ((active_block != i) && (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num > invalid_page))
   {
    invalid_page = ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[i].invalid_page_num;
    block = i;
   }
  }
  gc_node->block = block;
 }

 if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num != ssd->parameter->page_block)
 {
  for (i = gc_node->page; i < ssd->parameter->page_block; i++)
  {
   if (ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].page_head[i].valid_state > 0)
   {
    location = (struct local *)malloc(sizeof(struct local));
    alloc_assert(location, "location");
    memset(location, 0, sizeof(struct local));

    location->channel = channel;
    location->chip = chip;
    location->die = die;
    location->plane = plane;
    location->block = block;
    location->page = i;
    transfer_size = 0;

    move_page(ssd, location, &transfer_size);
    ssd->gc_copypage++;
    free(location);
    location = NULL;

    gc_node->page = i + 1;
    ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[gc_node->block].invalid_page_num++;
    ssd->channel_head[channel].current_state = CHANNEL_C_A_TRANSFER;
    ssd->channel_head[channel].current_time = ssd->current_time;
    ssd->channel_head[channel].next_state = CHANNEL_IDLE;
    ssd->channel_head[channel].chip_head[chip].current_state = CHIP_COPYBACK_BUSY;
    ssd->channel_head[channel].chip_head[chip].current_time = ssd->current_time;
    ssd->channel_head[channel].chip_head[chip].next_state = CHIP_IDLE;

    if ((ssd->parameter->advanced_commands & AD_COPYBACK) == AD_COPYBACK)
    {
     ssd->channel_head[channel].next_state_predict_time = ssd->current_time + 7 * ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tR + 7 * ssd->parameter->time_characteristics.tWC;
     ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tPROG;
    }
    else
    {
     ssd->channel_head[channel].next_state_predict_time = ssd->current_time + (7 + transfer_size * SECTOR) * ssd->parameter->time_characteristics.tWC + ssd->parameter->time_characteristics.tR + (7 + transfer_size * SECTOR) * ssd->parameter->time_characteristics.tWC;
     ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tPROG;
    }
    return 0;
   }
  }
 }
 else
 {
  erase_operation(ssd, channel, chip, die, plane, gc_node->block);

  ssd->channel_head[channel].current_state = CHANNEL_C_A_TRANSFER;
  ssd->channel_head[channel].current_time = ssd->current_time;
  ssd->channel_head[channel].next_state = CHANNEL_IDLE;
  ssd->channel_head[channel].next_state_predict_time = ssd->current_time + 5 * ssd->parameter->time_characteristics.tWC;

  ssd->channel_head[channel].chip_head[chip].current_state = CHIP_ERASE_BUSY;
  ssd->channel_head[channel].chip_head[chip].current_time = ssd->current_time;
  ssd->channel_head[channel].chip_head[chip].next_state = CHIP_IDLE;
  ssd->channel_head[channel].chip_head[chip].next_state_predict_time = ssd->channel_head[channel].next_state_predict_time + ssd->parameter->time_characteristics.tBERS;

  return 1;
 }

 printf("there is a problem in interrupt_gc\n");
 return 1;
}




int delete_gc_node(struct ssd_info *ssd, unsigned int channel, struct gc_operation *gc_node)
{
 struct gc_operation *gc_pre = NULL;
 if (gc_node == NULL)
 {
  return ERROR;
 }

 if (gc_node == ssd->channel_head[channel].gc_command)
 {
  ssd->channel_head[channel].gc_command = gc_node->next_node;
 }
 else
 {
  gc_pre = ssd->channel_head[channel].gc_command;
  while (gc_pre->next_node != NULL)
  {
   if (gc_pre->next_node == gc_node)
   {
    gc_pre->next_node = gc_node->next_node;
    break;
   }
   gc_pre = gc_pre->next_node;
  }
 }
 free(gc_node);
 gc_node = NULL;
 ssd->gc_request--;
 return SUCCESS;
}




Status gc_for_channel(struct ssd_info *ssd, unsigned int channel)
{
 int flag_direct_erase = 1, flag_gc = 1, flag_invoke_gc = 1;
 unsigned int chip, die, plane, flag_priority = 0;
 unsigned int current_state = 0, next_state = 0;
 long long next_state_predict_time = 0;
 struct gc_operation *gc_node = NULL, *gc_p = NULL;






 gc_node = ssd->channel_head[channel].gc_command;
 while (gc_node != NULL)
 {
  current_state = ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
  next_state = ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
  next_state_predict_time = ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;
  if ((current_state == CHIP_IDLE) || ((next_state == CHIP_IDLE) && (next_state_predict_time <= ssd->current_time)))
  {
   if (gc_node->priority == GC_UNINTERRUPT)
   {
    flag_priority = 1;
    break;
   }
  }
  gc_node = gc_node->next_node;
 }
 if (flag_priority != 1)
 {
  gc_node = ssd->channel_head[channel].gc_command;
  while (gc_node != NULL)
  {
   current_state = ssd->channel_head[channel].chip_head[gc_node->chip].current_state;
   next_state = ssd->channel_head[channel].chip_head[gc_node->chip].next_state;
   next_state_predict_time = ssd->channel_head[channel].chip_head[gc_node->chip].next_state_predict_time;



   if ((current_state == CHIP_IDLE) || ((next_state == CHIP_IDLE) && (next_state_predict_time <= ssd->current_time)))
   {
    break;
   }
   gc_node = gc_node->next_node;
  }
 }
 if (gc_node == NULL)
 {
  return FAILURE;
 }

 chip = gc_node->chip;
 die = gc_node->die;
 plane = gc_node->plane;

 if (gc_node->priority == GC_UNINTERRUPT)
 {
  flag_direct_erase = gc_direct_erase(ssd, channel, chip, die, plane);
  if (flag_direct_erase != SUCCESS)
  {
   flag_gc = uninterrupt_gc(ssd, channel, chip, die, plane);
   if (flag_gc == 1)
   {
    delete_gc_node(ssd, channel, gc_node);
   }
  }
  else
  {
   delete_gc_node(ssd, channel, gc_node);
  }
  return SUCCESS;
 }




 else
 {
  flag_invoke_gc = decide_gc_invoke(ssd, channel);

  if (flag_invoke_gc == 1)
  {
   flag_direct_erase = gc_direct_erase(ssd, channel, chip, die, plane);
   if (flag_direct_erase == -1)
   {
    flag_gc = interrupt_gc(ssd, channel, chip, die, plane, gc_node);
    if (flag_gc == 1)
    {
     delete_gc_node(ssd, channel, gc_node);
    }
   }
   else if (flag_direct_erase == 1)
   {
    delete_gc_node(ssd, channel, gc_node);
   }
   return SUCCESS;
  }
  else
  {
   return FAILURE;
  }
 }
}
# 1329 "pagemap.c"
unsigned int gc(struct ssd_info *ssd, unsigned int channel, unsigned int flag)
{
 unsigned int i;
 int flag_direct_erase = 1, flag_gc = 1, flag_invoke_gc = 1;
 unsigned int flag_priority = 0;
 struct gc_operation *gc_node = NULL, *gc_p = NULL;

 if (flag == 1)
 {
  for (i = 0; i < ssd->parameter->channel_number; i++)
  {
   flag_priority = 0;
   flag_direct_erase = 1;
   flag_gc = 1;
   flag_invoke_gc = 1;
   gc_node = NULL;
   gc_p = NULL;
   if ((ssd->channel_head[i].current_state == CHANNEL_IDLE) || (ssd->channel_head[i].next_state == CHANNEL_IDLE && ssd->channel_head[i].next_state_predict_time <= ssd->current_time))
   {
    channel = i;
    if (ssd->channel_head[channel].gc_command != NULL)
    {
     gc_for_channel(ssd, channel);
     ssd->gc_bg_time++;
    }
   }
  }
  return SUCCESS;
 }
 else
 {
  if ((ssd->parameter->allocation_scheme == 1) || ((ssd->parameter->allocation_scheme == 0) && (ssd->parameter->dynamic_allocation == 1)))
  {
   if ((ssd->channel_head[channel].subs_r_head != NULL) || (ssd->channel_head[channel].subs_w_head != NULL))
   {
    return 0;
   }
  }

  gc_for_channel(ssd, channel);
  ssd->gc_fg_time++;
  return SUCCESS;
 }
}


int decide_gc_invoke(struct ssd_info *ssd, unsigned int channel)
{
 struct sub_request *sub;
 struct local *location;

 if ((ssd->channel_head[channel].subs_r_head == NULL) && (ssd->channel_head[channel].subs_w_head == NULL))
 {
  return 1;
 }
 else
 {
  if (ssd->channel_head[channel].subs_w_head != NULL)
  {
   return 0;
  }
  else if (ssd->channel_head[channel].subs_r_head != NULL)
  {
   sub = ssd->channel_head[channel].subs_r_head;
   while (sub != NULL)
   {
    if (sub->current_state == SR_WAIT)
    {
     location = find_location(ssd, sub->ppn);
     if ((ssd->channel_head[location->channel].chip_head[location->chip].current_state == CHIP_IDLE) || ((ssd->channel_head[location->channel].chip_head[location->chip].next_state == CHIP_IDLE) &&
                              (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time <= ssd->current_time)))
     {
      free(location);
      location = NULL;
      return 0;
     }
     free(location);
     location = NULL;
    }
    else if (sub->next_state == SR_R_DATA_TRANSFER)
    {
     location = find_location(ssd, sub->ppn);
     if (ssd->channel_head[location->channel].chip_head[location->chip].next_state_predict_time <= ssd->current_time)
     {
      free(location);
      location = NULL;
      return 0;
     }
     free(location);
     location = NULL;
    }
    sub = sub->next_node;
   }
  }
  return 1;
 }
}
