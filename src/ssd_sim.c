
#include "ssd_sim.h"

int main(int argc, char *argv[]) {
    unsigned int i, j, k, mcount;
    struct ssd_info *ssd;

    struct configuration *config = (struct configuration *) malloc(sizeof(struct configuration));

    if (argv[1] == NULL) {
        printf("few input arguments, please recheck!\n");
        free(config);
        return 0;
    }

    if (argv[2] == NULL) {
        strcpy(config->parameter, DEFAULT_CONFIG_PATH);
        printf("using default parameter configs: %s\n", config->parameter);
    }


    strcpy(config->tracename, argv[1]);
    mcount = 0;
    printf("--->%s,%d", config->tracename);


    ssd = (struct ssd_info *) malloc(sizeof(struct ssd_info));
    alloc_assert(ssd, "ssd");
    memset(ssd, 0, sizeof(struct ssd_info));

    ssd = initiation(ssd, config, mcount);
    make_aged(ssd);
    pre_process_page(ssd);

    for (i = 0; i < ssd->parameter->channel_number; i++) {
        for (j = 0; j < ssd->parameter->die_chip; j++) {
            for (k = 0; k < ssd->parameter->plane_die; k++) {
                printf("%d,0,%d,%d:  %5d\n", i, j, k,
                       ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].free_page);
            }
        }
    }
    fprintf(ssd->outputfile, "\t\t\t\t\t\t\t\t\tOUTPUT\n");
    fprintf(ssd->outputfile, "****************** TRACE INFO ******************\n");

    ssd = simulate(ssd);
    statistic_output(ssd);


    printf("\n");
    printf("the simulation is completed!\n");
    printf("gc time is:%d %d %d\n", ssd->gc_bg_time, ssd->gc_fg_time, ssd->gc_copypage);
    free(config);
    return 1;

}

struct ssd_info *simulate(struct ssd_info *ssd) {
    int flag = 1;

    printf("\n");
    printf("begin simulating.......................\n");
    printf("\n");
    printf("\n");
    printf("   ^o^    OK, please wait a moment, and enjoy music and coffee   ^o^    \n");

    ssd->tracefile = fopen(ssd->tracefilename, "r");
    if (ssd->tracefile == NULL) {
        printf("the trace file can't open\n");
        return NULL;
    }

    fprintf(ssd->outputfile,
            "      arrive           lsn     size ope     begin time    response time    process time\n");
    fflush(ssd->outputfile);

    while (flag != 100) {

        flag = get_requests(ssd);

        if (flag == 1) {

            if (ssd->parameter->dram_capacity != 0) {
                buffer_management(ssd);
                distribute(ssd);
            } else {
                no_buffer_distribute(ssd);
            }
        }

        process(ssd);
        trace_output(ssd);
        if (flag == 0 && ssd->request_queue == NULL)
            flag = 100;
    }

    fclose(ssd->tracefile);
    return ssd;
}

int read_ndata(char *buffer, int64_t *time_t, int *lsn, int *size, int *ope) {
    int64_t time_f;
    int disk;
    int lsn1;
    int size1;
    int rw;
    sscanf(buffer, "%ld  %d %d %d %d", &time_f, &disk, &lsn1, &size1, &rw);
    if (lsn1 == 0 && size1 == 0 && time_f == 0) {

        return 0;
    }

    *ope = rw;
    *lsn = lsn1;
    *size = size1;
    *time_t = (int64_t) (time_f);
    return 1;
}

int read_data(struct ssd_info *ssd, char *buffer, int64_t *time_t, int *lsn, int *size, int *ope) {

    read_ndata(buffer, time_t, lsn, size, ope);
}

int get_requests(struct ssd_info *ssd) {
    char buffer[200];
    unsigned int lsn = 0;
    int device, size, ope, large_lsn, i = 0, j = 0;
    struct request *request1;
    int flag = 1;
    long filepoint;
    int64_t time_t = 0;
    int64_t nearest_event_time;
    int result = 0;

#ifdef DEBUG
    printf("enter get_requests,  current time:%lld\n", ssd->current_time);
#endif

    filepoint = ftell(ssd->tracefile);
    fgets(buffer, 200, ssd->tracefile);


    result = read_data(ssd, buffer, &time_t, &lsn, &size, &ope);
    if ((device < 0) && (lsn < 0) && (size < 0) && (ope < 0)) {
        return 100;
    }
    if (lsn < ssd->min_lsn)
        ssd->min_lsn = lsn;
    if (lsn > ssd->max_lsn)
        ssd->max_lsn = lsn;
    large_lsn = (int) ((ssd->parameter->subpage_page * ssd->parameter->page_block * ssd->parameter->block_plane *
                        ssd->parameter->plane_die * ssd->parameter->die_chip * ssd->parameter->chip_num) *
                       (1 - ssd->parameter->overprovide));
    lsn = lsn % large_lsn;

    nearest_event_time = find_nearest_event(ssd);
    if (nearest_event_time == MAX_INT64) {
        ssd->current_time = time_t;
    } else {
        if (nearest_event_time < time_t) {
            fseek(ssd->tracefile, filepoint, 0);
            if (ssd->current_time <= nearest_event_time)
                ssd->current_time = nearest_event_time;
            return -1;
        } else {
            if (ssd->request_queue_length >= ssd->parameter->queue_length) {
                fseek(ssd->tracefile, filepoint, 0);
                ssd->current_time = nearest_event_time;
                return -1;
            } else {
                ssd->current_time = time_t;
            }
        }
    }

    if (time_t < 0) {
        printf("error!\n");
        while (1) {
        }
    }

    if (feof(ssd->tracefile)) {
        ssd->current_time = nearest_event_time;
        request1 = NULL;
        return 0;
    }

    request1 = (struct request *) malloc(sizeof(struct request));
    alloc_assert(request1, "request");
    memset(request1, 0, sizeof(struct request));

    request1->time = time_t;
    request1->lsn = lsn;
    request1->size = size;
    request1->operation = ope;
    request1->begin_time = time_t;
    request1->response_time = 0;
    request1->energy_consumption = 0;
    request1->next_node = NULL;
    request1->distri_flag = 0;
    request1->subs = NULL;
    request1->need_distr_flag = NULL;
    request1->complete_lsn_count = 0;
    filepoint = ftell(ssd->tracefile);

    if (ssd->request_queue == NULL) {
        ssd->request_queue = request1;
        ssd->request_tail = request1;
        ssd->request_queue_length++;
    } else {
        (ssd->request_tail)->next_node = request1;
        ssd->request_tail = request1;
        ssd->request_queue_length++;
    }

    if (request1->operation == 1) {
        ssd->ave_read_size =
                (ssd->ave_read_size * ssd->read_request_count + request1->size) / (ssd->read_request_count + 1);
    } else {
        ssd->ave_write_size =
                (ssd->ave_write_size * ssd->write_request_count + request1->size) / (ssd->write_request_count + 1);
    }


    return 1;
}

struct ssd_info *buffer_management(struct ssd_info *ssd) {
    unsigned int j, lsn, lpn, last_lpn, first_lpn, index, complete_flag = 0, state, full_page;
    unsigned int flag = 0, need_distb_flag, lsn_flag, flag1 = 1, active_region_flag = 0;
    struct request *new_request;
    struct buffer_group *buffer_node, key;
    unsigned int mask = 0, offset1 = 0, offset2 = 0;

#ifdef DEBUG
    printf("enter buffer_management,  current time:%lld\n", ssd->current_time);
#endif
    ssd->dram->current_time = ssd->current_time;
    full_page = ~(0xffffffff << ssd->parameter->subpage_page);

    new_request = ssd->request_tail;
    lsn = new_request->lsn;
    lpn = new_request->lsn / ssd->parameter->subpage_page;
    last_lpn = (new_request->lsn + new_request->size - 1) / ssd->parameter->subpage_page;
    first_lpn = new_request->lsn / ssd->parameter->subpage_page;

    new_request->need_distr_flag = (unsigned int *) malloc(
            sizeof(unsigned int) * ((last_lpn - first_lpn + 1) * ssd->parameter->subpage_page / 32 + 1));
    alloc_assert(new_request->need_distr_flag, "new_request->need_distr_flag");
    memset(new_request->need_distr_flag, 0,
           sizeof(unsigned int) * ((last_lpn - first_lpn + 1) * ssd->parameter->subpage_page / 32 + 1));

    if (new_request->operation == READ) {
        while (lpn <= last_lpn) {


            need_distb_flag = full_page;
            key.group = lpn;
            buffer_node = (struct buffer_group *) avlTreeFind(ssd->dram->buffer, (TREE_NODE *) &key);

            while ((buffer_node != NULL) && (lsn < (lpn + 1) * ssd->parameter->subpage_page) &&
                   (lsn <= (new_request->lsn + new_request->size - 1))) {
                lsn_flag = full_page;
                mask = 1 << (lsn % ssd->parameter->subpage_page);
                if (mask > 31) {
                    printf("the subpage number is larger than 32!add some cases");
                    getchar();
                } else if ((buffer_node->stored & mask) == mask) {
                    flag = 1;
                    lsn_flag = lsn_flag & (~mask);
                }

                if (flag == 1) {
                    if (ssd->dram->buffer->buffer_head != buffer_node) {
                        if (ssd->dram->buffer->buffer_tail == buffer_node) {
                            buffer_node->LRU_link_pre->LRU_link_next = NULL;
                            ssd->dram->buffer->buffer_tail = buffer_node->LRU_link_pre;
                        } else {
                            buffer_node->LRU_link_pre->LRU_link_next = buffer_node->LRU_link_next;
                            buffer_node->LRU_link_next->LRU_link_pre = buffer_node->LRU_link_pre;
                        }
                        buffer_node->LRU_link_next = ssd->dram->buffer->buffer_head;
                        ssd->dram->buffer->buffer_head->LRU_link_pre = buffer_node;
                        buffer_node->LRU_link_pre = NULL;
                        ssd->dram->buffer->buffer_head = buffer_node;
                    }
                    ssd->dram->buffer->read_hit++;
                    new_request->complete_lsn_count++;
                } else if (flag == 0) {
                    ssd->dram->buffer->read_miss_hit++;
                }

                need_distb_flag = need_distb_flag & lsn_flag;

                flag = 0;
                lsn++;
            }

            index = (lpn - first_lpn) / (32 / ssd->parameter->subpage_page);
            new_request->need_distr_flag[index] = new_request->need_distr_flag[index] | (need_distb_flag
                    << (((lpn - first_lpn) % (32 / ssd->parameter->subpage_page)) * ssd->parameter->subpage_page));
            lpn++;
        }
    } else if (new_request->operation == WRITE) {
        while (lpn <= last_lpn) {
            need_distb_flag = full_page;
            mask = ~(0xffffffff << (ssd->parameter->subpage_page));
            state = mask;

            if (lpn == first_lpn) {
                offset1 = ssd->parameter->subpage_page - ((lpn + 1) * ssd->parameter->subpage_page - new_request->lsn);
                state = state & (0xffffffff << offset1);
            }
            if (lpn == last_lpn) {
                offset2 = ssd->parameter->subpage_page -
                          ((lpn + 1) * ssd->parameter->subpage_page - (new_request->lsn + new_request->size));
                state = state & (~(0xffffffff << offset2));
            }

            ssd = insert2buffer(ssd, lpn, state, NULL, new_request);

            lpn++;
        }
    }
    complete_flag = 1;
    for (j = 0; j <= (last_lpn - first_lpn + 1) * ssd->parameter->subpage_page / 32; j++) {
        if (new_request->need_distr_flag[j] != 0) {
            complete_flag = 0;
        }
    }


    if ((complete_flag == 1) && (new_request->subs == NULL)) {
        new_request->begin_time = ssd->current_time;
        new_request->response_time = ssd->current_time + 1000;
    }

    return ssd;
}


unsigned int lpn2ppn(struct ssd_info *ssd, unsigned int lsn) {
    int lpn, ppn;
    struct entry *p_map = ssd->dram->map->map_entry;
#ifdef DEBUG
    printf("enter lpn2ppn,  current time:%lld\n", ssd->current_time);
#endif
    lpn = lsn / ssd->parameter->subpage_page;
    ppn = (p_map[lpn]).pn;
    return ppn;
}


struct ssd_info *distribute(struct ssd_info *ssd) {
    unsigned int start, end, first_lsn, last_lsn, lpn, flag = 0, flag_attached = 0, full_page;
    unsigned int j, k, sub_size;
    int i = 0;
    struct request *req;
    struct sub_request *sub;
    int *complt;

#ifdef DEBUG
    printf("enter distribute,  current time:%lld\n", ssd->current_time);
#endif
    full_page = ~(0xffffffff << ssd->parameter->subpage_page);

    req = ssd->request_tail;
    if (req->response_time != 0) {
        return ssd;
    }
    if (req->operation == WRITE) {
        return ssd;
    }

    if (req != NULL) {
        if (req->distri_flag == 0) {

            if (req->complete_lsn_count != ssd->request_tail->size) {
                first_lsn = req->lsn;
                last_lsn = first_lsn + req->size;
                complt = req->need_distr_flag;
                start = first_lsn - first_lsn % ssd->parameter->subpage_page;
                end = (last_lsn / ssd->parameter->subpage_page + 1) * ssd->parameter->subpage_page;
                i = (end - start) / 32;

                while (i >= 0) {


                    for (j = 0; j < 32 / ssd->parameter->subpage_page; j++) {
                        k = (complt[((end - start) / 32 - i)] >> (ssd->parameter->subpage_page * j)) & full_page;
                        if (k != 0) {
                            lpn = start / ssd->parameter->subpage_page +
                                  ((end - start) / 32 - i) * 32 / ssd->parameter->subpage_page + j;
                            sub_size = transfer_size(ssd, k, lpn, req);
                            if (sub_size == 0) {
                                continue;
                            } else {
                                sub = creat_sub_request(ssd, lpn, sub_size, 0, req, req->operation);
                            }
                        }
                    }
                    i = i - 1;
                }
            } else {
                req->begin_time = ssd->current_time;
                req->response_time = ssd->current_time + 1000;
            }
        }
    }
    return ssd;
}


void trace_output(struct ssd_info *ssd) {
    int flag = 1;
    int64_t start_time, end_time;
    struct request *req, *pre_node;
    struct sub_request *sub, *tmp;

#ifdef DEBUG
    printf("enter trace_output,  current time:%lld\n", ssd->current_time);
#endif

    pre_node = NULL;
    req = ssd->request_queue;
    start_time = 0;
    end_time = 0;

    if (req == NULL)
        return;

    while (req != NULL) {
        sub = req->subs;
        flag = 1;
        start_time = 0;
        end_time = 0;
        if (req->response_time != 0) {
            fprintf(ssd->outputfile, "%16lld %10d %6d %2d %16lld %16lld %10lld\n", req->time, req->lsn, req->size,
                    req->operation, req->begin_time, req->response_time, req->response_time - req->time);
            fflush(ssd->outputfile);

            if (req->response_time - req->begin_time == 0) {
                printf("the response time is 0?? \n");
                getchar();
            }

            if (req->operation == READ) {
                ssd->read_request_count++;
                ssd->read_avg = ssd->read_avg + (req->response_time - req->time);
            } else {
                ssd->write_request_count++;
                ssd->write_avg = ssd->write_avg + (req->response_time - req->time);
            }

            if (pre_node == NULL) {
                if (req->next_node == NULL) {
                    free(req->need_distr_flag);
                    req->need_distr_flag = NULL;
                    free(req);
                    req = NULL;
                    ssd->request_queue = NULL;
                    ssd->request_tail = NULL;
                    ssd->request_queue_length--;
                } else {
                    ssd->request_queue = req->next_node;
                    pre_node = req;
                    req = req->next_node;
                    free(pre_node->need_distr_flag);
                    pre_node->need_distr_flag = NULL;
                    free((void *) pre_node);
                    pre_node = NULL;
                    ssd->request_queue_length--;
                }
            } else {
                if (req->next_node == NULL) {
                    pre_node->next_node = NULL;
                    free(req->need_distr_flag);
                    req->need_distr_flag = NULL;
                    free(req);
                    req = NULL;
                    ssd->request_tail = pre_node;
                    ssd->request_queue_length--;
                } else {
                    pre_node->next_node = req->next_node;
                    free(req->need_distr_flag);
                    req->need_distr_flag = NULL;
                    free((void *) req);
                    req = pre_node->next_node;
                    ssd->request_queue_length--;
                }
            }
        } else {
            flag = 1;
            while (sub != NULL) {
                if (start_time == 0)
                    start_time = sub->begin_time;
                if (start_time > sub->begin_time)
                    start_time = sub->begin_time;
                if (end_time < sub->complete_time)
                    end_time = sub->complete_time;
                if ((sub->current_state == SR_COMPLETE) ||
                    ((sub->next_state == SR_COMPLETE) && (sub->next_state_predict_time <= ssd->current_time))) {
                    sub = sub->next_subs;
                } else {
                    flag = 0;
                    break;
                }
            }

            if (flag == 1) {

                fprintf(ssd->outputfile, "%16lld %10d %6d %2d %16lld %16lld %10lld\n", req->time, req->lsn, req->size,
                        req->operation, start_time, end_time, end_time - req->time);
                fflush(ssd->outputfile);

                if (end_time - start_time == 0) {
                    printf("the response time is 0?? \n");
                    getchar();
                }

                if (req->operation == READ) {
                    ssd->read_request_count++;
                    ssd->read_avg = ssd->read_avg + (end_time - req->time);
                } else {
                    ssd->write_request_count++;
                    ssd->write_avg = ssd->write_avg + (end_time - req->time);
                }

                while (req->subs != NULL) {
                    tmp = req->subs;
                    req->subs = tmp->next_subs;
                    if (tmp->update != NULL) {
                        free(tmp->update->location);
                        tmp->update->location = NULL;
                        free(tmp->update);
                        tmp->update = NULL;
                    }
                    free(tmp->location);
                    tmp->location = NULL;
                    free(tmp);
                    tmp = NULL;
                }

                if (pre_node == NULL) {
                    if (req->next_node == NULL) {
                        free(req->need_distr_flag);
                        req->need_distr_flag = NULL;
                        free(req);
                        req = NULL;
                        ssd->request_queue = NULL;
                        ssd->request_tail = NULL;
                        ssd->request_queue_length--;
                    } else {
                        ssd->request_queue = req->next_node;
                        pre_node = req;
                        req = req->next_node;
                        free(pre_node->need_distr_flag);
                        pre_node->need_distr_flag = NULL;
                        free(pre_node);
                        pre_node = NULL;
                        ssd->request_queue_length--;
                    }
                } else {
                    if (req->next_node == NULL) {
                        pre_node->next_node = NULL;
                        free(req->need_distr_flag);
                        req->need_distr_flag = NULL;
                        free(req);
                        req = NULL;
                        ssd->request_tail = pre_node;
                        ssd->request_queue_length--;
                    } else {
                        pre_node->next_node = req->next_node;
                        free(req->need_distr_flag);
                        req->need_distr_flag = NULL;
                        free(req);
                        req = pre_node->next_node;
                        ssd->request_queue_length--;
                    }
                }
            } else {
                pre_node = req;
                req = req->next_node;
            }
        }
    }
}


void statistic_output(struct ssd_info *ssd) {
    unsigned int lpn_count = 0, i, j, k, m, erase = 0, plane_erase = 0;
    double gc_energy = 0.0;

    for (i = 0; i < ssd->parameter->channel_number; i++) {
        for (j = 0; j < ssd->parameter->die_chip; j++) {
            for (k = 0; k < ssd->parameter->plane_die; k++) {
                plane_erase = 0;
                for (m = 0; m < ssd->parameter->block_plane; m++) {
                    if (ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].blk_head[m].erase_count > 0) {
                        erase = erase +
                                ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].blk_head[m].erase_count;
                        plane_erase += ssd->channel_head[i].chip_head[0].die_head[j].plane_head[k].blk_head[m].erase_count;
                    }
                }
                fprintf(ssd->outputfile, "the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n", i,
                        j, k, m, plane_erase);
                fprintf(ssd->statisticfile, "the %d channel, %d chip, %d die, %d plane has : %13d erase operations\n",
                        i, j, k, m, plane_erase);
            }
        }
    }


    fprintf(ssd->outputfile, "\n");
    fprintf(ssd->outputfile, "\n");
    fprintf(ssd->outputfile, "---------------------------statistic data---------------------------\n");
    fprintf(ssd->outputfile, "min lsn: %13d\n", ssd->min_lsn);
    fprintf(ssd->outputfile, "max lsn: %13d\n", ssd->max_lsn);
    fprintf(ssd->outputfile, "read count: %13d\n", ssd->read_count);
    fprintf(ssd->outputfile, "program count: %13d", ssd->program_count);
    fprintf(ssd->outputfile, "                        include the flash write count leaded by read requests\n");
    fprintf(ssd->outputfile, "the read operation leaded by un-covered update count: %13d\n", ssd->update_read_count);
    fprintf(ssd->outputfile, "erase count: %13d\n", ssd->erase_count);
    fprintf(ssd->outputfile, "direct erase count: %13d\n", ssd->direct_erase_count);
    fprintf(ssd->outputfile, "copy back count: %13d\n", ssd->copy_back_count);
    fprintf(ssd->outputfile, "multi-plane program count: %13d\n", ssd->m_plane_prog_count);
    fprintf(ssd->outputfile, "multi-plane read count: %13d\n", ssd->m_plane_read_count);
    fprintf(ssd->outputfile, "interleave write count: %13d\n", ssd->interleave_count);
    fprintf(ssd->outputfile, "interleave read count: %13d\n", ssd->interleave_read_count);
    fprintf(ssd->outputfile, "interleave two plane and one program count: %13d\n", ssd->inter_mplane_prog_count);
    fprintf(ssd->outputfile, "interleave two plane count: %13d\n", ssd->inter_mplane_count);
    fprintf(ssd->outputfile, "gc copy back count: %13d\n", ssd->gc_copy_back);
    fprintf(ssd->outputfile, "write flash count: %13d\n", ssd->write_flash_count);
    fprintf(ssd->outputfile, "interleave erase count: %13d\n", ssd->interleave_erase_count);
    fprintf(ssd->outputfile, "multiple plane erase count: %13d\n", ssd->mplane_erase_conut);
    fprintf(ssd->outputfile, "interleave multiple plane erase count: %13d\n", ssd->interleave_mplane_erase_count);
    fprintf(ssd->outputfile, "read request count: %13d\n", ssd->read_request_count);
    fprintf(ssd->outputfile, "write request count: %13d\n", ssd->write_request_count);
    fprintf(ssd->outputfile, "read request average size: %13f\n", ssd->ave_read_size);
    fprintf(ssd->outputfile, "write request average size: %13f\n", ssd->ave_write_size);
    if (ssd->read_request_count != 0) {
        fprintf(ssd->outputfile, "read request average response time: %lld\n", ssd->read_avg / ssd->read_request_count);
    }
    if (ssd->write_request_count != 0) {

        fprintf(ssd->outputfile, "write request average response time: %lld\n",
                ssd->write_avg / ssd->write_request_count);
    }
    fprintf(ssd->outputfile, "buffer read hits: %13d\n", ssd->dram->buffer->read_hit);
    fprintf(ssd->outputfile, "buffer read miss: %13d\n", ssd->dram->buffer->read_miss_hit);
    fprintf(ssd->outputfile, "buffer write hits: %13d\n", ssd->dram->buffer->write_hit);
    fprintf(ssd->outputfile, "buffer write miss: %13d\n", ssd->dram->buffer->write_miss_hit);
    fprintf(ssd->outputfile, "erase: %13d\n", erase);
    fflush(ssd->outputfile);

    fclose(ssd->outputfile);

    fprintf(ssd->statisticfile, "\n");
    fprintf(ssd->statisticfile, "\n");
    fprintf(ssd->statisticfile, "---------------------------statistic data---------------------------\n");
    fprintf(ssd->statisticfile, "min lsn: %13d\n", ssd->min_lsn);
    fprintf(ssd->statisticfile, "max lsn: %13d\n", ssd->max_lsn);
    fprintf(ssd->statisticfile, "read count: %13d\n", ssd->read_count);
    fprintf(ssd->statisticfile, "program count: %13d", ssd->program_count);
    fprintf(ssd->statisticfile, "                        include the flash write count leaded by read requests\n");
    fprintf(ssd->statisticfile, "the read operation leaded by un-covered update count: %13d\n", ssd->update_read_count);
    fprintf(ssd->statisticfile, "erase count: %13d\n", ssd->erase_count);
    fprintf(ssd->statisticfile, "direct erase count: %13d\n", ssd->direct_erase_count);
    fprintf(ssd->statisticfile, "copy back count: %13d\n", ssd->copy_back_count);
    fprintf(ssd->statisticfile, "multi-plane program count: %13d\n", ssd->m_plane_prog_count);
    fprintf(ssd->statisticfile, "multi-plane read count: %13d\n", ssd->m_plane_read_count);
    fprintf(ssd->statisticfile, "interleave count: %13d\n", ssd->interleave_count);
    fprintf(ssd->statisticfile, "interleave read count: %13d\n", ssd->interleave_read_count);
    fprintf(ssd->statisticfile, "interleave two plane and one program count: %13d\n", ssd->inter_mplane_prog_count);
    fprintf(ssd->statisticfile, "interleave two plane count: %13d\n", ssd->inter_mplane_count);
    fprintf(ssd->statisticfile, "gc copy back count: %13d\n", ssd->gc_copy_back);
    fprintf(ssd->statisticfile, "write flash count: %13d\n", ssd->write_flash_count);
    fprintf(ssd->statisticfile, "waste page count: %13d\n", ssd->waste_page_count);
    fprintf(ssd->statisticfile, "interleave erase count: %13d\n", ssd->interleave_erase_count);
    fprintf(ssd->statisticfile, "multiple plane erase count: %13d\n", ssd->mplane_erase_conut);
    fprintf(ssd->statisticfile, "interleave multiple plane erase count: %13d\n", ssd->interleave_mplane_erase_count);
    fprintf(ssd->statisticfile, "read request count: %13d\n", ssd->read_request_count);
    fprintf(ssd->statisticfile, "write request count: %13d\n", ssd->write_request_count);
    fprintf(ssd->statisticfile, "read request average size: %13f\n", ssd->ave_read_size);
    fprintf(ssd->statisticfile, "write request average size: %13f\n", ssd->ave_write_size);

    if (ssd->read_request_count != 0) {
        fprintf(ssd->statisticfile, "read request average response time: %lld\n",
                ssd->read_avg / ssd->read_request_count);
    }
    if (ssd->write_request_count != 0) {
        fprintf(ssd->statisticfile, "write request average response time: %lld\n",
                ssd->write_avg / ssd->write_request_count);
    }

    fprintf(ssd->statisticfile, "buffer read hits: %13d\n", ssd->dram->buffer->read_hit);
    fprintf(ssd->statisticfile, "buffer read miss: %13d\n", ssd->dram->buffer->read_miss_hit);
    fprintf(ssd->statisticfile, "buffer write hits: %13d\n", ssd->dram->buffer->write_hit);
    fprintf(ssd->statisticfile, "buffer write miss: %13d\n", ssd->dram->buffer->write_miss_hit);
    fprintf(ssd->statisticfile, "erase: %13d\n", erase);
    fflush(ssd->statisticfile);

    fclose(ssd->statisticfile);
}


unsigned int size(unsigned int stored) {
    unsigned int i, total = 0, mask = 0x80000000;

#ifdef DEBUG
    printf("enter size\n");
#endif
    for (i = 1; i <= 32; i++) {
        if (stored & mask)
            total++;
        stored <<= 1;
    }
#ifdef DEBUG
    printf("leave size\n");
#endif
    return total;
}


unsigned int transfer_size(struct ssd_info *ssd, int need_distribute, unsigned int lpn, struct request *req) {
    unsigned int first_lpn, last_lpn, state, trans_size;
    unsigned int mask = 0, offset1 = 0, offset2 = 0;

    first_lpn = req->lsn / ssd->parameter->subpage_page;
    last_lpn = (req->lsn + req->size - 1) / ssd->parameter->subpage_page;

    mask = ~(0xffffffff << (ssd->parameter->subpage_page));
    state = mask;
    if (lpn == first_lpn) {
        offset1 = ssd->parameter->subpage_page - ((lpn + 1) * ssd->parameter->subpage_page - req->lsn);
        state = state & (0xffffffff << offset1);
    }
    if (lpn == last_lpn) {
        offset2 = ssd->parameter->subpage_page - ((lpn + 1) * ssd->parameter->subpage_page - (req->lsn + req->size));
        state = state & (~(0xffffffff << offset2));
    }

    trans_size = size(state & need_distribute);

    return trans_size;
}

int64_t find_nearest_event(struct ssd_info *ssd) {
    unsigned int i, j;
    int64_t time = MAX_INT64;
    int64_t time1 = MAX_INT64;
    int64_t time2 = MAX_INT64;

    for (i = 0; i < ssd->parameter->channel_number; i++) {
        if (ssd->channel_head[i].next_state == CHANNEL_IDLE)
            if (time1 > ssd->channel_head[i].next_state_predict_time)
                if (ssd->channel_head[i].next_state_predict_time > ssd->current_time)
                    time1 = ssd->channel_head[i].next_state_predict_time;
        for (j = 0; j < ssd->parameter->chip_channel[i]; j++) {
            if ((ssd->channel_head[i].chip_head[j].next_state == CHIP_IDLE) ||
                (ssd->channel_head[i].chip_head[j].next_state == CHIP_DATA_TRANSFER))
                if (time2 > ssd->channel_head[i].chip_head[j].next_state_predict_time)
                    if (ssd->channel_head[i].chip_head[j].next_state_predict_time > ssd->current_time)
                        time2 = ssd->channel_head[i].chip_head[j].next_state_predict_time;
        }
    }

    time = (time1 > time2) ? time2 : time1;
    return time;
}


void free_all_node(struct ssd_info *ssd) {
    unsigned int i, j, k, l, n;
    struct buffer_group *pt = NULL;
    struct direct_erase *erase_node = NULL;
    for (i = 0; i < ssd->parameter->channel_number; i++) {
        for (j = 0; j < ssd->parameter->chip_channel[0]; j++) {
            for (k = 0; k < ssd->parameter->die_chip; k++) {
                for (l = 0; l < ssd->parameter->plane_die; l++) {
                    for (n = 0; n < ssd->parameter->block_plane; n++) {
                        free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head);
                        ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[n].page_head = NULL;
                    }
                    free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head);
                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head = NULL;
                    while (ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node != NULL) {
                        erase_node = ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
                        ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node = erase_node->next_node;
                        free(erase_node);
                        erase_node = NULL;
                    }
                }

                free(ssd->channel_head[i].chip_head[j].die_head[k].plane_head);
                ssd->channel_head[i].chip_head[j].die_head[k].plane_head = NULL;
            }
            free(ssd->channel_head[i].chip_head[j].die_head);
            ssd->channel_head[i].chip_head[j].die_head = NULL;
        }
        free(ssd->channel_head[i].chip_head);
        ssd->channel_head[i].chip_head = NULL;
    }
    free(ssd->channel_head);
    ssd->channel_head = NULL;

    avlTreeDestroy(ssd->dram->buffer);
    ssd->dram->buffer = NULL;

    free(ssd->dram->map->map_entry);
    ssd->dram->map->map_entry = NULL;
    free(ssd->dram->map);
    ssd->dram->map = NULL;
    free(ssd->dram);
    ssd->dram = NULL;
    free(ssd->parameter);
    ssd->parameter = NULL;

    free(ssd);
    ssd = NULL;
}

struct ssd_info *make_aged(struct ssd_info *ssd) {
    unsigned int i, j, k, l, m, n, ppn;
    int threshould, flag = 0;
    int valid_tag = 0;
    unsigned int lpn, lsn, full_page, sub_size, largest_lsn;
    int invalid_num = 0;
    int valid_num = 0;
    int threshould_block = 0;
    int rand_value;
    char buffer[20];
    long filepoint;
    unsigned int active_blk0, active_blk1;
    unsigned int count = 101;
    unsigned int invalid_count = 0;
    float rand_rat;
    int rand_mark;

    struct direct_erase *direct_erase_node, *new_direct_erase, *new_direct_erase2;
    struct gc_operation *gc_node, *gc_node2;

    full_page = ~(0xffffffffffffffffffffffffffffffff << (ssd->parameter->subpage_page));
    srand((int) time(0));
    largest_lsn = (unsigned int) ((ssd->parameter->chip_num * ssd->parameter->die_chip * ssd->parameter->plane_die *
                                   ssd->parameter->block_plane * ssd->parameter->page_block *
                                   ssd->parameter->subpage_page) * (1 - ssd->parameter->overprovide));

    if (ssd->parameter->aged == 1) {
        for (i = 0; i < ssd->parameter->channel_number; i++) {
            for (j = 0; j < ssd->parameter->chip_channel[i]; j++) {
                threshould = (int) (ssd->parameter->block_plane * ssd->parameter->page_block *
                                    ssd->parameter->aged_ratio);
                for (k = 0; k < ssd->parameter->die_chip; k++) {
                    for (l = 0; l < ssd->parameter->plane_die; l++) {
                        flag = 0;
                        for (m = 0; m < ssd->parameter->block_plane; m++) {
                            if (flag > threshould) {
                                break;
                            }
                            rand_value = rand() % 10;
                            rand_value = rand_value / 10;
                            ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].erase_count =
                                    ssd->parameter->ers_limit * ssd->parameter->aged_ratio * rand_value;
                            for (n = 0; n < ssd->parameter->page_block; n++) {
                                rand_value = rand() % 10;
                                valid_tag = ((rand_value > 2) ? 1 : 0);
                                if ((valid_tag == 0)) {

                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].valid_state = 0;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].free_state = 0;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].lpn = 0;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_page_num--;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].invalid_page_num++;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_page++;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page--;
                                    flag++;
                                    invalid_num++;
                                    ppn = find_ppn(ssd, i, j, k, l, m, n);
                                } else {

                                    lpn = count;
                                    count++;
                                    lsn = lpn * ssd->parameter->subpage_page;
                                    lsn = lsn % largest_lsn;
                                    sub_size = ssd->parameter->subpage_page - (lsn % ssd->parameter->subpage_page);

                                    ppn = find_ppn(ssd, i, j, k, l, m, n);
                                    ssd->channel_head[i].program_count++;
                                    ssd->channel_head[i].chip_head[j].program_count++;
                                    ssd->dram->map->map_entry[lpn].pn = ppn;
                                    ssd->dram->map->map_entry[lpn].state = set_entry_state(ssd, lsn, sub_size);
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].lpn = lpn;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].valid_state = ssd->dram->map->map_entry[lpn].state;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].free_state = (
                                            (~ssd->dram->map->map_entry[lpn].state) & full_page);
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].page_head[n].make_aged = 1;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].free_page_num--;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].last_write_page++;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page--;
                                    flag++;
                                    valid_num++;
                                }
                                if (flag > threshould) {
                                    break;
                                }
                            }

                            if (ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].blk_head[m].invalid_page_num ==
                                ssd->parameter->page_block) {
                                new_direct_erase = (struct direct_erase *) malloc(sizeof(struct direct_erase));
                                alloc_assert(new_direct_erase, "new_direct_erase");
                                memset(new_direct_erase, 0, sizeof(struct direct_erase));

                                new_direct_erase->block = m;
                                new_direct_erase->next_node = NULL;
                                direct_erase_node = ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
                                if (direct_erase_node == NULL) {
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node = new_direct_erase;
                                } else {
                                    new_direct_erase->next_node = ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node;
                                    ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].erase_node = new_direct_erase;
                                }
                                ssd->tmp_count++;
                                ssd->total_gc++;
                            }
                        }

                        if (ssd->channel_head[i].chip_head[j].die_head[k].plane_head[l].free_page <
                            (ssd->parameter->page_block * ssd->parameter->block_plane *
                             ssd->parameter->gc_hard_threshold)) {
                            gc_node = (struct gc_operation *) malloc(sizeof(struct gc_operation));
                            alloc_assert(gc_node, "gc_node");
                            memset(gc_node, 0, sizeof(struct gc_operation));

                            gc_node->next_node = NULL;
                            gc_node->chip = j;
                            gc_node->die = k;
                            gc_node->plane = l;
                            gc_node->block = 0xffffffffffffffffffffffffffffffff;
                            gc_node->page = 0;
                            gc_node->state = GC_WAIT;
                            gc_node->priority = GC_UNINTERRUPT;
                            gc_node->next_node = ssd->channel_head[i].gc_command;
                            ssd->channel_head[i].gc_command = gc_node;
                            ssd->gc_request++;
                            ssd->total_gc++;
                        }
                    }
                }
            }
        }
    } else {
        return ssd;
    }

    printf("aging is completed!\n");
    return ssd;
}

struct ssd_info *no_buffer_distribute(struct ssd_info *ssd) {
    unsigned int lsn, lpn, last_lpn, first_lpn, complete_flag = 0, state;
    unsigned int flag = 0, flag1 = 1, active_region_flag = 0;
    struct request *req = NULL;
    struct sub_request *sub = NULL, *sub_r = NULL, *update = NULL;
    struct local *loc = NULL;
    struct channel_info *p_ch = NULL;

    unsigned int mask = 0;
    unsigned int offset1 = 0, offset2 = 0;
    unsigned int sub_size = 0;
    unsigned int sub_state = 0;

    ssd->dram->current_time = ssd->current_time;
    req = ssd->request_tail;
    lsn = req->lsn;
    lpn = req->lsn / ssd->parameter->subpage_page;
    last_lpn = (req->lsn + req->size - 1) / ssd->parameter->subpage_page;
    first_lpn = req->lsn / ssd->parameter->subpage_page;

    if (req->operation == READ) {
        while (lpn <= last_lpn) {
            sub_state = (ssd->dram->map->map_entry[lpn].state & 0x7fffffff);
            sub_size = size(sub_state);
            sub = creat_sub_request(ssd, lpn, sub_size, sub_state, req, req->operation);
            lpn++;
        }
    } else if (req->operation == WRITE) {
        while (lpn <= last_lpn) {
            mask = ~(0xffffffff << (ssd->parameter->subpage_page));
            state = mask;
            if (lpn == first_lpn) {
                offset1 = ssd->parameter->subpage_page - ((lpn + 1) * ssd->parameter->subpage_page - req->lsn);
                state = state & (0xffffffff << offset1);
            }
            if (lpn == last_lpn) {
                offset2 = ssd->parameter->subpage_page -
                          ((lpn + 1) * ssd->parameter->subpage_page - (req->lsn + req->size));
                state = state & (~(0xffffffff << offset2));
            }
            sub_size = size(state);

            sub = creat_sub_request(ssd, lpn, sub_size, state, req, req->operation);
            lpn++;
        }
    }

    return ssd;
}
