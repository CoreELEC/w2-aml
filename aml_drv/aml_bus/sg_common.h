#ifndef SG_COMMON_H_
#define SG_COMMON_H_

#include <linux/scatterlist.h>

#define SG_PAGE_MAX         80                  /* max page per sdio write operation */
#define MAXSG_SIZE        (SG_PAGE_MAX * 2)
#define MAX_SG_ENTRIES    (MAXSG_SIZE+2)

struct amlw_hif_scatter_item {
    int len;
    int page_num;
    void *packet;
};

struct amlw_hif_scatter_req {
    /* address for the read/write operation */
    unsigned int addr;
    /* total length of entire transfer */
    unsigned int len;

    bool free;
    int result;
    int scat_count;

    struct scatterlist sgentries[MAX_SG_ENTRIES];
    struct amlw_hif_scatter_item scat_list[MAX_SG_ENTRIES];
};

#endif /* SG_COMMON_H_ */
