#ifndef _AML_SAP_H_
#define _AML_SAP_H_

#include "aml_defs.h"

#define AML_SCC_FRMBUF_MAXLEN   1000

extern u8 bcn_save[AML_SCC_FRMBUF_MAXLEN];

void aml_save_bcn_buf(u8 *bcn_buf, size_t len);
u8* aml_get_beacon_ie_addr(u8* buf, int frame_len,u8 eid);
u8* aml_get_probe_rsp_ie_addr(u8* buf, int frame_len,u8 eid);

#endif /* _AML_SAP_H_ */
