/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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

/*! \file rrc_gNB_XNAP.h
 * \brief rrc XNAP procedures for gNB
 * \author RAKESH BB
 * \date 2025
 * \version 0.1
 * \email: rakeshbb108@gmail.com (rakeshbb108%40gmail.com)
*/

#ifndef RRC_GNB_XNAP_H_
#define RRC_GNB_XNAP_H_

#include <assertions.h>
#include <stdint.h>
#include "openair2/RRC/NR/nr_rrc_proto.h"
#include "nr_rrc_defs.h"
#include "rrc_gNB_mobility.h"
#include "rrc_gNB_du.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "openair2/COMMON/xnap_messages_types.h"
#include "openair2/XNAP/xnap_gNB_management_procedures.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair3/SECU/key_nas_deriver.h"

void rrc_gNB_process_XNAP_SETUP_REQUEST(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id, xnap_setup_req_t *m, instance_t instance);
void rrc_gNB_process_XNAP_SETUP_RESPONSE(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id, xnap_setup_resp_t *m, instance_t instance);
int rrc_gNB_process_XNAP_HANDOVER_PREPARATION(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id, xnap_handover_req_t *msg);
void rrc_gNB_send_XNAP_HANDOVER_PREPARATION_FAILURE(gNB_RRC_INST *rrc, handover_failure_t *msg, sctp_assoc_t assoc_id);
void rrc_gNB_process_XNAP_HANDOVER_PREPARATION_FAILURE(instance_t instance, xnap_handover_preparation_failure_t *msg);
void rrc_gNB_send_XNAP_HANDOVER_REQUEST(gNB_RRC_INST *rrc,
                                         gNB_RRC_UE_t *UE,
                                         const nr_neighbour_cell_t *neighbourCellConfiguration,
                                         const byte_array_t hoPrepInfo);
void rrc_gNB_process_XNAP_HANDOVER_REQUEST_ACKNOWLEDGE(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id, const xnap_handover_req_ack_t *msg);
int rrc_gNB_send_XNAP_SN_STATUS_TRANSFER(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, const int n_to_mod, const int *drb_ids, const e1_pdcp_status_info_t *pdcp_status);
int rrc_gNB_process_XNAP_SN_STATUS_TRANSFER(gNB_RRC_INST *rrc, MessageDef *msg_p, instance_t instance);
int rrc_gNB_send_XNAP_UE_CONTEXT_RELEASE(gNB_RRC_INST *rrc,const gNB_RRC_UE_t *UE);
int rrc_gNB_process_XNAP_UE_CONTEXT_RELEASE(gNB_RRC_INST *rrc, instance_t instance, xnap_ue_context_release_t *msg);



#endif

