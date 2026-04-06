/* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdio.h>
#include <stdint.h>

#ifndef XNAP_H_
#define XNAP_H_

#define XNAP_SCTP_PPID (61) ///< XNAP SCTP Payload Protocol Identifier (PPID)
#include "xnap_gNB_defs.h"
#include "XNAP_XnAP-PDU.h" 

int xnap_gNB_init_sctp(instance_t instance_p, xnap_net_config_t *nc, char *my_addr);
XNAP_XnAP_PDU_t* encode_xn_handover_preparation_failure(const xnap_handover_preparation_failure_t *msg);
void xnap_gNB_itti_send_sctp_data_req(sctp_assoc_t assoc_id, uint8_t *buffer, uint32_t buffer_length, uint16_t stream);

void *xnap_task(void *arg);
int is_xnap_enabled(void);

#endif /* XNAP_H_ */

/**
 * @}
 */
