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

/*! \file rrc_gNB_XNAP.c
 * \brief rrc XNAP procedures for gNB
 * \author Rakesh BB

 * \version 0.1
 * \email: rakeshbb108@gmail.com
 */
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include "assertions.h"
#include "common/utils/LOG/log.h"
#include "tree.h"
#include "rrc_gNB_XNAP.h"
#include "rrc_cell_management.h"
#include "rrc_gNB_NGAP.h"
#include "openair2/RRC/NR/rrc_gNB_radio_bearers.h"

extern struct timespec ho_start_time;
extern int ho_timer_running;

static sctp_assoc_t get_target_assoc_id(gNB_RRC_INST *rrc, uint32_t gNB_ID){
  if (RB_EMPTY(&rrc->neighs)) {
    printf("No neighbors found in RRC neigh tree.\n");
    return -1;
  }

  struct nr_rrc_neighcells_container_t *neigh = NULL;

  RB_FOREACH(neigh, rrc_neigh_cell_tree, &rrc->neighs) {
    if (neigh && neigh->gNB_id == gNB_ID) {
      return neigh->assoc_id;
    }
  }
  return -1;
}

static int neigh_compare(const nr_rrc_neighcells_container_t *a, const nr_rrc_neighcells_container_t *b)
{
  if (a->gNB_id > b->gNB_id)
    return 1;
  if (a->gNB_id == b->gNB_id)
    return 0;
  return -1; /* a->assoc_id < b->assoc_id */
}

/* Tree management functions */
RB_GENERATE(rrc_neigh_cell_tree, nr_rrc_neighcells_container_t, entries, neigh_compare);

void rrc_gNB_send_XNAP_SETUP_RESPONSE(instance_t instance, gNB_RRC_INST *rrc, sctp_assoc_t assoc_id){
   LOG_I(NR_RRC, "[XnHO] Preparing XNAP setup resopose\n");
   MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, XNAP_SETUP_RESP);
   msg->ittiMsgHeader.originInstance = assoc_id;
   itti_send_msg_to_task(TASK_XNAP, 0, msg);
}

void rrc_gNB_process_XNAP_SETUP_REQUEST(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id, xnap_setup_req_t *m, instance_t instance){
  if (rrc->num_neighs > MAX_NUM_NR_NEIGH_CELLs) {
    LOG_E(NR_RRC, "[XnHO] Can't process Xn setup request, maximum number of neighbouring cell exceeded \n");
    MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, XNAP_SETUP_FAILURE);
    msg->ittiMsgHeader.originInstance = assoc_id;
    xnap_setup_failure_t *xnap_msg = &XNAP_SETUP_FAILURE(msg);
    xnap_msg->cause.type = XNAP_CAUSE_PROTOCOL;
    xnap_msg->cause.value = XNAP_CAUSE_RADIO_NETWORK_LAYER_REDUCE_LOAD_IN_SERVING_CELL;
    itti_send_msg_to_task(TASK_XNAP, 0, msg);
  }
  // handle mismatch of AMF region ID 
  nr_rrc_neighcells_container_t *neigh_cell = malloc(sizeof(*neigh_cell));
  AssertFatal(neigh_cell, "out of memory\n");
  neigh_cell->assoc_id = assoc_id;
  neigh_cell->gNB_id = m->gNB_id;
  
  nr_rrc_neighcells_container_t *existing =
        (nr_rrc_neighcells_container_t *) RB_INSERT(rrc_neigh_cell_tree, &rrc->neighs, neigh_cell);

  if (existing != NULL) {
      LOG_W(NR_RRC, "[XnHO] Duplicate gNB id found %u\n", neigh_cell->gNB_id);
      existing->assoc_id = neigh_cell->assoc_id;
      LOG_I(NR_RRC, "[XnHO] Updated RB-Tree gNB_ID[%u]<->assoc_id[%d]\n",
            existing->gNB_id, existing->assoc_id);
      free(neigh_cell);
  } else {
      LOG_I(NR_RRC, "[XnHO] Insert successful: Cell ID %u\n", neigh_cell->gNB_id);
      rrc->num_neighs++;
  }
  rrc_gNB_send_XNAP_SETUP_RESPONSE(instance, rrc, assoc_id);
}

void rrc_gNB_process_XNAP_SETUP_RESPONSE(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id, xnap_setup_resp_t *m, instance_t instance)
{
  if (rrc->num_neighs > MAX_NUM_NR_NEIGH_CELLs) {
    LOG_E(NR_RRC, "Error: number of neighbouring cells is exceeded \n");
    return;
  }
  nr_rrc_neighcells_container_t *neigh_cell = malloc(sizeof(*neigh_cell));
  AssertFatal(neigh_cell, "out of memory\n");
  neigh_cell->assoc_id = assoc_id;
  neigh_cell->gNB_id = m->gNB_id;
  nr_rrc_neighcells_container_t *existing =
        (nr_rrc_neighcells_container_t *) RB_INSERT(rrc_neigh_cell_tree, &rrc->neighs, neigh_cell);

  if (existing != NULL) {
      LOG_W(NR_RRC, "[XNAP] Duplicate gNB id found %u\n", neigh_cell->gNB_id);
      existing->assoc_id = neigh_cell->assoc_id;
      LOG_I(NR_RRC, "[XNAP] Updated RB-Tree gNB_ID[%u]<->assoc_id[%d]",
            existing->gNB_id, existing->assoc_id);
      free(neigh_cell);
  } else {
      LOG_I(NR_RRC, "[XNAP] Insert successful: Cell ID %u\n", neigh_cell->gNB_id);
      rrc->num_neighs++;
  }
}

void rrc_gNB_process_XNAP_LOST_CONNECTION(gNB_RRC_INST *rrc, xnap_lost_connection_t *m)
{
    LOG_I(NR_RRC, "[XNAP] Received XNAP LOST CONNECTION for assoc_id : %u\n", m->assoc_id);


    nr_rrc_neighcells_container_t key = {
        .assoc_id = m->assoc_id
    };
    nr_rrc_neighcells_container_t *removed = RB_REMOVE(rrc_neigh_cell_tree, &rrc->neighs, &key);
    if (!removed) {
        LOG_W(NR_RRC, "[XNAP] No matching neighbor entry found for assoc_id=%u\n", m->assoc_id);
        return;
    }

    // If there is a counter, decrement it
    if (rrc->num_neighs > 0)
        rrc->num_neighs--;

    LOG_I(NR_RRC, "[XNAP] Successfully removed neighbor record (assoc_id=%u)\n", m->assoc_id);
}

/** @brief Callback for XnAP Handover Required message (3GPP TS 38.423 9.2.3.1)
 * Direction: source gNB -> target gNB */
void rrc_gNB_send_XNAP_HANDOVER_REQUEST(gNB_RRC_INST *rrc,
                                        gNB_RRC_UE_t *UE,
                                        const nr_neighbour_cell_t *neighbour,
                                        const byte_array_t hoPrepInfo)
{
  LOG_I(NR_RRC, "Handover Preparation: send Handover Request (target gNB ID=%d, PCI=%d)\n", neighbour->gNB_ID, neighbour->physicalCellId);

  const xnap_ngran_cgi_t target = {.plmn_id = neighbour->plmn,
                                   .nrcell_id = neighbour->nrcell_id};
  
  nr_rrc_cell_container_t *p_cell = rrc_get_pcell_for_ue(rrc, UE);

  const xnap_ngran_cgi_t source = {.plmn_id = p_cell->info.plmn,
                                   .nrcell_id = p_cell->info.cell_id};
  
  xnap_security_capabilities_t sec_cap = {.nRencryption_algorithms = UE->security_capabilities.nRencryption_algorithms,
                                          .nRintegrity_algorithms = UE->security_capabilities.nRintegrity_algorithms,
                                          .eUTRAencryption_algorithms = UE->security_capabilities.eUTRAencryption_algorithms,
                                          .eUTRAintegrity_algorithms = UE->security_capabilities.eUTRAintegrity_algorithms,}; 

  xnap_pdusession_tobe_setup_list_t pdu_list = {0};
  FOR_EACH_SEQ_ARR(rrc_pdu_session_param_t*, pduSession, &UE->pduSessions) {
    if (pduSession->status != PDU_SESSION_STATUS_DONE &&
        pduSession->status != PDU_SESSION_STATUS_ESTABLISHED)
      continue;

    DevAssert(pdu_list.num_pdu < NGAP_MAX_PDU_SESSION);
    xnap_pdusession_tobe_setup_item_t *pdu = &pdu_list.pdu[pdu_list.num_pdu++];

    pdusession_t *session = &pduSession->param;
    // Basic PDU session info
    pdu->pdusession_id = session->pdusession_id;
    pdu->snssai.sst    = session->nssai.sst;
    pdu->pdu_session_type = session->pdu_session_type;
    pdu->n3_incoming   = session->n3_incoming;
    // QoS handling
    pdu->qos_list.num_qos = 0;
    FOR_EACH_SEQ_ARR(nr_rrc_qos_t*, qos_entry, &session->qos) {
      DevAssert(pdu->qos_list.num_qos < QOSFLOW_MAX_VALUE);
      xnap_qos_tobe_setup_item_t *qos_item = &pdu->qos_list.qos[pdu->qos_list.num_qos++];
      qos_item->qfi = qos_entry->qos.qfi;
      // QoS type
      qos_item->qos_params.qos_type = qos_entry->qos.fiveQI_type;
      if (qos_entry->qos.fiveQI_type == NON_DYNAMIC) {
        qos_item->qos_params.qos_type = NON_DYNAMIC;
        qos_item->qos_params.non_dynamic.fiveqi = qos_entry->qos.fiveQI;
        qos_item->qos_params.non_dynamic.qos_priority_level = qos_entry->qos.qos_priority;
      } else {
        qos_item->qos_params.qos_type = DYNAMIC;
        qos_item->qos_params.dynamic.fiveqi = qos_entry->qos.fiveQI;
        qos_item->qos_params.dynamic.qos_priority_level = qos_entry->qos.qos_priority;
        qos_item->qos_params.dynamic.packet_delay_budget = 10; // todo
        qos_item->qos_params.dynamic.packet_error_rate.per_scalar = 10; //todo
        qos_item->qos_params.dynamic.packet_error_rate.per_exponent = 10; //todo
      }
      // Allocation Retention Priority
      qos_item->allocation_retention_priority.priority_level = qos_entry->qos.arp.priority_level;
      qos_item->allocation_retention_priority.preemption_capability = qos_entry->qos.arp.pre_emp_capability;
      qos_item->allocation_retention_priority.preemption_vulnerability = qos_entry->qos.arp.pre_emp_vulnerability;
    }
  }

  xnap_ue_context_info_t ue_context_info = {.ngc_ue_sig_ref = UE->amf_ue_ngap_id,
                                            .security_capabilities = sec_cap,
                                            .as_security_ncc = UE->nh_ncc,
                                            .ue_ambr.br_ul  = UE->ambr.ul_br,
                                            .ue_ambr.br_dl  = UE->ambr.dl_br,
                                            .pdusession_tobe_setup_list = pdu_list,
                                            .rrc_context = copy_byte_array(hoPrepInfo),
                                            .tnl_ip_source = UE->amf_ng_ip,
                                            };

  LOG_I(NR_RRC, "HO Req CP Tunnel Address %s\n", inet_ntoa(*(struct in_addr *)UE->amf_ng_ip.buffer));
  nr_derive_key_ng_ran_star(neighbour->physicalCellId, neighbour->absoluteFrequencySSB, 
                            UE->nh_ncc > 0 ? UE->nh : UE->kgnb, ue_context_info.as_security_key_ranstar);

  const xnap_uehistory_info_t ue_history_info = {.last_visited_cgi = source,
			                         .cell_type = CELL_MACRO_GNB,
                                                 .time_UE_StayedInCell = min(time(NULL) - UE->last_seen, 4095)};

  // Temp implementation, 
  // In actual, at XNAP target assoc id should be taken from target NR_cellid(gNB_id(22-32 ?) + cell id)
  const sctp_assoc_t tar_assocID = get_target_assoc_id(rrc, neighbour->gNB_ID);
  xnap_handover_req_t msg = {.s_ng_node_ue_xnap_id = UE->rrc_ue_id,
			     .cause.type = XNAP_CAUSE_RADIO_NETWORK,  
                             .cause.value = XNAP_CAUSE_RADIO_NETWORK_LAYER_HANDOVER_DESIRABLE_FOR_RADIO_REASONS,
                             .target_cgi = target,
                             .guami = UE->ue_guami,
                             .ue_context = ue_context_info,
			     .uehistory_info = ue_history_info, 
                             .target_assoc_id = tar_assocID,
  };
  
  // TXn Reloc Timer Implementation is still pending
  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_GNB, 0, XNAP_HANDOVER_REQ);
  XNAP_HANDOVER_REQ(msg_p) = msg;
  itti_send_msg_to_task(TASK_XNAP, 0, msg_p);
}

/* @brief Sends the XNAP Handover Preparation Failure from Target to Source */
void rrc_gNB_send_XNAP_HANDOVER_PREPARATION_FAILURE(gNB_RRC_INST *rrc, handover_failure_t *msg, sctp_assoc_t assoc_id)
{
  LOG_I(NR_RRC, "Send Xn Handover Preparation Failure message (ue_xnap_id %ld) with cause %d \n ", msg->ue_id, msg->cause.value);
  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_GNB, 0, XNAP_HANDOVER_PREPARATION_FAILURE);
  xnap_handover_preparation_failure_t fail = {.ng_node_ue_xnap_id = msg->ue_id,
					      .cause.type         = msg->cause.type,
                                              .cause.value        = msg->cause.value
                                             };
  msg_p->ittiMsgHeader.destinationInstance = assoc_id;
  XNAP_HANDOVER_PREPARATION_FAILURE(msg_p) = fail;
  itti_send_msg_to_task(TASK_XNAP, rrc->module_id, msg_p);
} 


/* @brief Process XNAP Handover Request message (8.2.1.2 3GPP TS 38.423) */
int rrc_gNB_process_XNAP_HANDOVER_PREPARATION(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id, xnap_handover_req_t *msg)
{
  nr_rrc_cell_container_t *cell = get_cell_by_cell_id(&rrc->cells, msg->target_cgi.nrcell_id);
  if (cell == NULL) {
    /* Cell Not Found! Return HO Request Failure*/
    LOG_E(RRC, "Failed to process Handover Request: no cell found with nRCellIdentity %lu \n", msg->target_cgi.nrcell_id);
    handover_failure_t fail = {
        .ue_id = msg->s_ng_node_ue_xnap_id,
        .cause.type = XNAP_CAUSE_RADIO_NETWORK,
        .cause.value = XNAP_CAUSE_RADIO_NETWORK_LAYER_NO_RADIO_RESOURCES_AVAILABLE_IN_TARGET_CELL,
    };
    rrc_gNB_send_XNAP_HANDOVER_PREPARATION_FAILURE(rrc, &fail, assoc_id);
    return -1; 
  }

  struct nr_rrc_du_container_t *du = get_du_by_assoc_id(rrc, cell->assoc_id);
  if(du == NULL) {
    LOG_E(NR_RRC, "Failed to process Handover Request: no DU found with assoc_id=%d\n", cell->assoc_id);
    handover_failure_t fail = {
        .ue_id = msg->s_ng_node_ue_xnap_id,
        .cause.type = XNAP_CAUSE_RADIO_NETWORK,
        .cause.value = XNAP_CAUSE_RADIO_NETWORK_LAYER_HANDOVER_TARGET_NOT_ALLOWED,
    };
    rrc_gNB_send_XNAP_HANDOVER_PREPARATION_FAILURE(rrc, &fail, assoc_id);
    return -1;
  }

  // Validate PLMN from GUAMI against allowed PLMN list
  const plmn_id_t *serving_plmn = get_serving_plmn(rrc, &msg->guami.plmn);
  if (!serving_plmn) {
    LOG_E(NR_RRC, "PLMN from GUAMI not supported - rejecting handover\n");
    handover_failure_t fail = {
        .ue_id = msg->s_ng_node_ue_xnap_id,
        .cause.type = XNAP_CAUSE_RADIO_NETWORK,
        .cause.value = XNAP_CAUSE_RADIO_NETWORK_LAYER_HANDOVER_TARGET_NOT_ALLOWED,
    };
    rrc_gNB_send_XNAP_HANDOVER_PREPARATION_FAILURE(rrc, &fail, assoc_id);
    return -1;
  }

  uint16_t pci = cell->info.pci;
  LOG_I(NR_RRC, "[XnHO] Received Handover Request (on NR Cell ID=%lu, PCI=%u) \n", msg->target_cgi.nrcell_id, pci);

  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_create_ue_context(du->assoc_id, UINT16_MAX, rrc, UINT64_MAX, UINT32_MAX);
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  
  UE->ho_context = alloc_ho_ctx(HO_CTX_TARGET);
  UE->ho_context->target->src_ue_xnap_id = msg->s_ng_node_ue_xnap_id;
  UE->ho_context->target->src_assoc_id = assoc_id;
  UE->ho_context->target->cell = cell;
  UE->ho_context->target->ho_trigger = nr_rrc_trigger_xn_ho_target;

  // Store IDs in UE context
  UE->amf_ue_ngap_id = msg->ue_context.ngc_ue_sig_ref;
  // Store the serving PLMN
  UE->serving_plmn  = *serving_plmn;
  UE->ue_guami      = msg->guami;
  UE->ambr.dl_br    = msg->ue_context.ue_ambr.br_dl;
  UE->ambr.ul_br    = msg->ue_context.ue_ambr.br_ul;			
  UE->amf_ng_ip     = msg->ue_context.tnl_ip_source;

  LOG_I(NR_RRC, "HO Req Processing CP tnl address IPv4: %s.\n", inet_ntoa(*(struct in_addr *)UE->amf_ng_ip.buffer));

  UE->ho_context->target->ue_ho_prep_info = copy_byte_array(msg->ue_context.rrc_context);
  // store the received UE Security Capabilities in the UE context
  FREE_AND_ZERO_BYTE_ARRAY(UE->ue_cap_buffer);
  UE->ue_cap_buffer = copy_byte_array(msg->ue_context.ue_cap);

  set_UE_security_algos(rrc, UE, &msg->ue_context.security_capabilities);
  UE->nh_ncc = msg->ue_context.as_security_ncc;
  memcpy(UE->kgnb, &msg->ue_context.as_security_key_ranstar, SECURITY_KEY_LENGTH);
  UE->as_security_active = true;
  // Activate SRBs
  activate_srb(UE, SRB1);
  activate_srb(UE, SRB2);

  nr_rrc_pdcp_config_security(UE, true);
 
  DevAssert(msg->ue_context.pdusession_tobe_setup_list.num_pdu <= NGAP_MAX_PDU_SESSION);
  pdusession_t to_setup[NGAP_MAX_PDU_SESSION];
  int nb_pdu = msg->ue_context.pdusession_tobe_setup_list.num_pdu;
  for (int i = 0; i < nb_pdu; i++) {
    const xnap_pdusession_tobe_setup_item_t *src = &msg->ue_context.pdusession_tobe_setup_list.pdu[i];
    pdusession_t *pdu = &to_setup[i];
    // Basic info
    pdu->pdusession_id    = src->pdusession_id;
    pdu->nssai.sst        = src->snssai.sst;
    pdu->pdu_session_type = src->pdu_session_type;
    pdu->n3_incoming      = src->n3_incoming;

    // QoS
    seq_arr_init(&pdu->qos, sizeof(nr_rrc_qos_t));
    DevAssert(src->qos_list.num_qos <= QOSFLOW_MAX_VALUE); 
    for (int j = 0; j < src->qos_list.num_qos; j++) {
      const xnap_qos_tobe_setup_item_t *qsrc = &src->qos_list.qos[j];
      nr_rrc_qos_t elem = {0};
      // Fill QoS params
      elem.qos.qfi = qsrc->qfi;
      elem.qos.fiveQI_type = qsrc->qos_params.qos_type;
      if (qsrc->qos_params.qos_type == NON_DYNAMIC) {
        elem.qos.fiveQI = qsrc->qos_params.non_dynamic.fiveqi;
        elem.qos.qos_priority = qsrc->qos_params.non_dynamic.qos_priority_level;
      } else {
        elem.qos.fiveQI = qsrc->qos_params.dynamic.fiveqi;
        elem.qos.qos_priority = qsrc->qos_params.dynamic.qos_priority_level; 
        // TODO: handle later if needed
        // qsrc->qos_params.dynamic.packet_delay_budget
        // qsrc->qos_params.dynamic.packet_error_rate
      }
      // ARP
      elem.qos.arp.priority_level = qsrc->allocation_retention_priority.priority_level;
      elem.qos.arp.pre_emp_capability = qsrc->allocation_retention_priority.preemption_capability;
      elem.qos.arp.pre_emp_vulnerability = qsrc->allocation_retention_priority.preemption_vulnerability;
    
      // Push into sequence
      seq_arr_push_back(&pdu->qos, &elem, sizeof(nr_rrc_qos_t));
    }
  }
  if (!trigger_bearer_setup(rrc, UE, nb_pdu, to_setup, msg->ue_context.ue_ambr.br_dl)) {
    LOG_E(NR_RRC, "XNAP HO: Failed to establish PDU session\n");
    handover_failure_t fail = {
        .ue_id = msg->s_ng_node_ue_xnap_id,
        .cause.type = XNAP_CAUSE_RADIO_NETWORK,
        .cause.value = XNAP_CAUSE_RADIO_NETWORK_LAYER_NO_RADIO_RESOURCES_AVAILABLE_IN_TARGET_CELL,
    };
    rrc_gNB_send_XNAP_HANDOVER_PREPARATION_FAILURE(rrc, &fail, assoc_id);
    rrc_remove_ue(rrc, ue_context_p);
    return -1;
  }

  return 0;
}

void rrc_gNB_process_XNAP_HANDOVER_PREPARATION_FAILURE(instance_t instance, xnap_handover_preparation_failure_t *msg)
{
  //TODO
  //Stop TXn Reloc prep timer
}
 
void rrc_gNB_send_XNAP_HANDOVER_REQUEST_ACKNOWLEDGE(gNB_RRC_INST *rrc, gNB_RRC_UE_t *UE, byte_array_t ho_command)
{
  LOG_D(NR_RRC, "Sending Handover Request Acknowledge\n");

  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_GNB, 0, XNAP_HANDOVER_REQ_ACK);
  xnap_handover_req_ack_t *msg = &XNAP_HANDOVER_REQ_ACK(msg_p);
  memset(msg, 0, sizeof(*msg));
  msg_p->ittiMsgHeader.originInstance = UE->ho_context->target->src_assoc_id;
  // SRC UE XNAP ID
  msg->s_ng_node_ue_xnap_id = UE->ho_context->target->src_ue_xnap_id;
  // TAR UE XNAP ID
  msg->t_ng_node_ue_xnap_id = UE->rrc_ue_id;
  // PDU Session Resource Admitted List
  FOR_EACH_SEQ_ARR(rrc_pdu_session_param_t*, pduSession, &UE->pduSessions) {
    pduSession->status = PDU_SESSION_STATUS_ESTABLISHED;

    DevAssert(msg->pdusession_admitted_list.num_pdu < NGAP_MAX_PDU_SESSION);
    xnap_pdusession_admitted_item_t *pdu = &msg->pdusession_admitted_list.pdu[msg->pdusession_admitted_list.num_pdu++];
    pdusession_t *session = &pduSession->param;
    pdu->pdusession_id = session->pdusession_id;
    pdu->qos_list.num_qos = 0;
    FOR_EACH_SEQ_ARR(nr_rrc_qos_t*, qp, &session->qos) {
      DevAssert(pdu->qos_list.num_qos < QOSFLOW_MAX_VALUE);
      xnap_qos_admitted_item_t *qos_item = &pdu->qos_list.qos[pdu->qos_list.num_qos++];
      qos_item->qfi = qp->qos.qfi;
    }
  }
  // Target to Source Transparent Container
  msg->target2source = copy_byte_array(ho_command);
  itti_send_msg_to_task(TASK_XNAP, rrc->module_id, msg_p);
}

void rrc_gNB_process_XNAP_HANDOVER_REQUEST_ACKNOWLEDGE(gNB_RRC_INST *rrc, sctp_assoc_t assoc_id, const xnap_handover_req_ack_t *msg){
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, msg->s_ng_node_ue_xnap_id);

  if (ue_context_p == NULL) {
    LOG_W(NR_RRC, "Unknown UE context associated to gNB_ue_ngap_id (%u)\n", msg->s_ng_node_ue_xnap_id);
    return;
  }
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  UE->ho_context->source->tar_assoc_id = assoc_id;
  UE->ho_context->source->tar_ue_xnap_id = msg->t_ng_node_ue_xnap_id;

  byte_array_t buffer = doRRCReconfiguration_from_HandoverCommand(msg->target2source);
  if (!buffer.buf || buffer.len == 0) {
    LOG_E(NR_RRC, "Failed to decode/encode RRCReconfiguration from HandoverCommand\n");
    DevAssert(UE->ho_context);
    DevAssert(UE->ho_context->source);
    DevAssert(UE->ho_context->source->ho_cancel);
    //UE->ho_context->source->ho_cancel(rrc, UE);
    return;
  }

  rrc_gNB_trigger_reconfiguration_for_handover(rrc, UE, buffer.buf, buffer.len);
  LOG_A(NR_RRC, "Send reconfiguration (HO Command) to UE %u/RNTI %04x\n", UE->rrc_ue_id, UE->rnti);
  free_byte_array(buffer);
}
  
/** @brief Send SN Status Transfer message (9.1.1.4 3GPP TS 38.423)
 * Direction: source NG-RAN node -> target NG-RAN node */

int rrc_gNB_send_XNAP_SN_STATUS_TRANSFER(gNB_RRC_INST *rrc,
                                         gNB_RRC_UE_t *UE, 
                                         const int n_to_mod, 
                                         const int *drb_ids, 
                                         const e1_pdcp_status_info_t *pdcp_status)
{
  AssertFatal(UE != NULL, "UE context is NULL\n");
  DevAssert(n_to_mod <= MAX_DRBS_PER_UE);
  DevAssert(drb_ids);
  
  LOG_I(NR_RRC,
        "Sending SN Status Transfer (Source XNAP UE ID =%u, Target XNAP UE ID=%u)\n",
        UE->rrc_ue_id,
        UE->ho_context->source->tar_ue_xnap_id);

  xnap_sn_status_transfer_t msg = {
      .s_ng_node_ue_xnap_id = UE->rrc_ue_id,
      .t_ng_node_ue_xnap_id = UE->ho_context->source->tar_ue_xnap_id,
  };

  // Loop through DRBs and extract COUNT values
  for (int i = 0; i < n_to_mod; ++i) {
    int drb_id = drb_ids[i];    

    // Find the DRB in the UE's DRB list
    drb_t *drb = get_drb(&UE->drbs, drb_id);
    if (!drb) {
      LOG_E(NR_RRC, "Failed to send UL RAN Status Transfer: DRB %d not found\n", drb_id);
      continue;
    }
   
    bool sn_length_18 = drb->pdcp_config.drb.sn_size == 18;

    DevAssert(msg.ran_status.nb_drb < MAX_DRBS_PER_UE);
    xnap_drb_status_t *item = &msg.ran_status.drb_status_list[msg.ran_status.nb_drb++];
    item->drb_id = drb_id;

    const e1_pdcp_count_t *ul_pdcp = &pdcp_status[i].ul_count;
    const e1_pdcp_count_t *dl_pdcp = &pdcp_status[i].dl_count;

    item->ul_count.pdcp_sn = ul_pdcp->sn;
    item->ul_count.hfn = ul_pdcp->hfn;
    item->ul_count.sn_len = sn_length_18 ? XNAP_SN_LENGTH_18 : XNAP_SN_LENGTH_12;

    item->dl_count.pdcp_sn = dl_pdcp->sn;
    item->dl_count.hfn = dl_pdcp->hfn;
    item->dl_count.sn_len = sn_length_18 ? XNAP_SN_LENGTH_18 : XNAP_SN_LENGTH_12;
  }

  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_GNB, 0, XNAP_SN_STATUS_TRANSFER);
  msg_p->ittiMsgHeader.originInstance = UE->ho_context->source->tar_assoc_id;
  XNAP_SN_STATUS_TRANSFER(msg_p) = msg;
  itti_send_msg_to_task(TASK_XNAP, rrc->module_id, msg_p);

  return 0;
}

int rrc_gNB_process_XNAP_SN_STATUS_TRANSFER(gNB_RRC_INST *rrc, MessageDef *msg_p, instance_t instance)
{
  const xnap_sn_status_transfer_t *msg = &XNAP_SN_STATUS_TRANSFER(msg_p);
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, msg->t_ng_node_ue_xnap_id);

  if (!ue_context_p) {
    LOG_E(NR_RRC, "[gNB %ld] No UE context for target node ue xnap id %u\n", instance, msg->t_ng_node_ue_xnap_id);
    return -1;
  }

  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  LOG_I(NR_RRC,
        "[gNB %ld] XNAP SN Status Transfer from source ue xnap id  %u for target ue xnap id  %u\n",
        instance,
        msg->s_ng_node_ue_xnap_id,
        msg->t_ng_node_ue_xnap_id);

  for (int i = 0; i < msg->ran_status.nb_drb; ++i) {
    const xnap_drb_status_t *s = &msg->ran_status.drb_status_list[i];
    LOG_I(NR_RRC,
          "SN Status Transfer - DRB ID %d:\n"
          "  UL COUNT: PDCP SN = %u, HFN = %u (%s)\n"
          "  DL COUNT: PDCP SN = %u, HFN = %u (%s)\n",
          s->drb_id,
          s->ul_count.pdcp_sn,
          s->ul_count.hfn,
          s->ul_count.sn_len == XNAP_SN_LENGTH_18 ? "18-bit" : "12-bit",
          s->dl_count.pdcp_sn,
          s->dl_count.hfn,
          s->dl_count.sn_len == XNAP_SN_LENGTH_18 ? "18-bit" : "12-bit");

    // Send to PDCP layer
    e1_notify_pdcp_status(rrc, UE, s);
  }

  return 0;
} 
 
int rrc_gNB_send_XNAP_UE_CONTEXT_RELEASE(gNB_RRC_INST *rrc,const gNB_RRC_UE_t *UE){
  AssertFatal(UE != NULL, "UE context is NULL\n");
  AssertFatal(UE->ho_context != NULL, "HO context is NULL\n");
  AssertFatal(UE->ho_context->target != NULL, "Target  context is NULL\n");
  
  LOG_I(NR_RRC, "[XN-HO] Sending UE context release to source gNB \n");
  xnap_ue_context_release_t msg = {.s_ng_node_ue_xnap_id = UE->ho_context->target->src_ue_xnap_id,
                                   .t_ng_node_ue_xnap_id = UE->rrc_ue_id };
  
  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_GNB, 0, XNAP_UE_CONTEXT_RELEASE);
  msg_p->ittiMsgHeader.originInstance = UE->ho_context->target->src_assoc_id;
  XNAP_UE_CONTEXT_RELEASE(msg_p) = msg;
  itti_send_msg_to_task(TASK_XNAP, rrc->module_id, msg_p);
  nr_rrc_finalize_ho(UE);
  LOG_I(NR_RRC, "[XN-HO] Removed handover context at target \n");
  return 0; 
}

int rrc_gNB_process_XNAP_UE_CONTEXT_RELEASE(gNB_RRC_INST *rrc, instance_t instance, xnap_ue_context_release_t *msg){
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc,  msg->s_ng_node_ue_xnap_id);
  if (ue_context_p == NULL) {
    /* Can not associate this message to an UE index */
    LOG_W(NR_RRC, "[gNB %ld] In XNAP_UE_CONTEXT_RELEASE: unknown UE from msg->s_ng_node_ue_xnap_id (%u)\n",
          instance,
          msg->s_ng_node_ue_xnap_id);
    return -1;
  }
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  UE->rrc_release = true;
//#ifdef E2_AGENT
//  signal_rrc_state_changed_to(UE, RRC_IDLE_RRC_STATE_E2SM_RC);
//#endif

  /* a UE might not be associated to a CU-UP if it never requested a PDU
   * session (intentionally, or because of erros) */
  if (ue_associated_to_cuup(rrc, UE)) {
    sctp_assoc_t assoc_id = get_existing_cuup_for_ue(rrc, UE);
    e1ap_cause_t cause = {.type = E1AP_CAUSE_RADIO_NETWORK, .value = E1AP_RADIO_CAUSE_NORMAL_RELEASE};
    e1ap_bearer_release_cmd_t cmd = {
      .gNB_cu_cp_ue_id = UE->rrc_ue_id,
      .gNB_cu_up_ue_id = UE->rrc_ue_id,
      .cause = cause,
    };
    rrc->cucp_cuup.bearer_context_release(assoc_id, &cmd);
  }
  /* special case: the DU might be offline, in which case the f1_ue_data exists
   * but is set to 0 */
  if (cu_exists_f1_ue_data(UE->rrc_ue_id) && cu_get_f1_ue_data(UE->rrc_ue_id).du_assoc_id != 0) {
    rrc_gNB_generate_RRCRelease(rrc, UE);
    /* UE will be freed after UE context release complete */
  } else {
    // the DU is offline already
    rrc_remove_ue(rrc, ue_context_p);
  }

  rrc->xn_handover_success++;
  if (ho_timer_running) {
    struct timespec ho_end_time;
    clock_gettime(CLOCK_MONOTONIC, &ho_end_time);

    double latency_ms =
        (ho_end_time.tv_sec - ho_start_time.tv_sec) * 1000.0 +
        (ho_end_time.tv_nsec - ho_start_time.tv_nsec) / 1e6;

    rrc->ho_xn_latency = latency_ms;

    ho_timer_running = 0;
  }
  LOG_I(NR_RRC,"XN Handover Completed\n");
  return 0;
}
