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

/*! \file xnap_gNB_interface_management.c
 * \brief xnap handling interface procedures for gNB
 * \author Sreeshma Shiv <sreeshmau@iisc.ac.in>
 * \date Dec 2023
 * \version 1.0
 */

#include <stdint.h>
#include "intertask_interface.h"
#include "xnap_common.h"
#include "xnap_gNB_defs.h"
#include "xnap_gNB_interface_management.h"
#include "xnap_gNB_handler.h"
#include "assertions.h"
#include "conversions.h"
#include "XNAP_GlobalgNB-ID.h"
#include "XNAP_ServedCells-NR-Item.h"
#include "XNAP_NRFrequencyBandItem.h"
#include "XNAP_GlobalNG-RANNode-ID.h"
#include "XNAP_NRModeInfoFDD.h"
#include "XNAP_NRModeInfoTDD.h"
#include "XNAP_SupportedSULBandList.h"
#include "XNAP_TAISupport-Item.h"
#include "XNAP_BroadcastPLMNinTAISupport-Item.h"
#include "xnap_gNB_management_procedures.h"
#include "XNAP_PDUSessionResourcesToBeSetup-Item.h"
#include "XNAP_QoSFlowsToBeSetup-Item.h"
#include "XNAP_GTPtunnelTransportLayerInformation.h"
#include "XNAP_NonDynamic5QIDescriptor.h"
#include "XNAP_Dynamic5QIDescriptor.h"
#include "XNAP_LastVisitedCell-Item.h"
#include "XNAP_QoSFlowsAdmitted-Item.h"
#include "XNAP_PDUSessionResourcesAdmitted-Item.h"
#include "NR_HandoverPreparationInformation.h"
#include "XNAP_BroadcastPLMNinTAISupport-Item.h"
#include "XNAP_GlobalAMF-Region-Information.h"
#include "NGAP_LastVisitedNGRANCellInformation.h"
#include "NGAP_NGRAN-CGI.h"
#include "NGAP_NR-CGI.h"

int decode_xnap_handover_preparation_failure(xnap_handover_preparation_failure_t *out, XNAP_XnAP_PDU_t *pdu){
    DevAssert(pdu != NULL);
    XNAP_HandoverPreparationFailure_t *container =  &pdu->choice.unsuccessfulOutcome->value.choice.HandoverPreparationFailure;
    XNAP_HandoverPreparationFailure_IEs_t *ie;

    //NG RAN UE XNAP ID
    XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverPreparationFailure_IEs_t, ie, container, XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID, true);
    out->ng_node_ue_xnap_id = ie->value.choice.NG_RANnodeUEXnAPID;
    
    //Cause
    XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverPreparationFailure_IEs_t, ie, container, XNAP_ProtocolIE_ID_id_Cause, true);
    out->cause = decode_xnap_cause(&ie->value.choice.Cause);
   
    return 0; 
}

int xnap_gNB_handle_xn_setup_request(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, XNAP_XnAP_PDU_t *pdu)
{
  XNAP_XnSetupRequest_t *xnSetupRequest;
  XNAP_XnSetupRequest_IEs_t *ie;

  DevAssert(pdu != NULL);
  xnSetupRequest = &pdu->choice.initiatingMessage->value.choice.XnSetupRequest;
  if (stream != 0) { /* Xn Setup: Non UE related procedure ->stream 0 */
    LOG_E(XNAP, "Received new XN setup request on stream != 0\n");
    /* Send a xn setup failure with protocol cause unspecified */
    MessageDef *message_p = itti_alloc_new_message(TASK_XNAP, 0, XNAP_SETUP_FAILURE);
    message_p->ittiMsgHeader.originInstance = assoc_id;
    xnap_setup_failure_t *fail = &XNAP_SETUP_FAILURE(message_p);
    fail->cause.type = XNAP_CAUSE_PROTOCOL;
    fail->cause.value = 6;
    itti_send_msg_to_task(TASK_XNAP, 0, message_p);
  }
  MessageDef *message_p = itti_alloc_new_message(TASK_XNAP, 0, XNAP_SETUP_REQ);
  message_p->ittiMsgHeader.originInstance = assoc_id;
  xnap_setup_req_t *req = &XNAP_SETUP_REQ(message_p);

    /* Global NG-RAN Node ID */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_XnSetupRequest_IEs_t, ie, xnSetupRequest, XNAP_ProtocolIE_ID_id_GlobalNG_RAN_node_ID, true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_GlobalNG_RAN_node_ID is NULL pointer \n");
    return -1;
  } else {
    if (ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.present == XNAP_GNB_ID_Choice_PR_gnb_ID) {
      uint8_t *gNB_id_buf = ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.choice.gnb_ID.buf;
      if (ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.choice.gnb_ID.size != 28) {
        // TODO: handle case where size != 28 -> notify ? reject ?
      }
      req->gNB_id = (gNB_id_buf[0] << 20) + (gNB_id_buf[1] << 12) + (gNB_id_buf[2] << 4) + ((gNB_id_buf[3] & 0xf0) >> 4);
    } else {
      // TODO if NSA setup
    }
  }

  /* TAI Support list */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_XnSetupRequest_IEs_t, ie, xnSetupRequest, XNAP_ProtocolIE_ID_id_TAISupport_list, true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_TAISupport_list is NULL pointer \n");
    return -1;
  } else {
    int tai_count = ie->value.choice.TAISupport_List.list.count;
    req->num_tai = tai_count;
    for (int m = 0; m < tai_count; m++) {
        // TAC Decode
        OCTET_STRING_TO_INT24(&ie->value.choice.TAISupport_List.list.array[m]->tac,
                              req->tai_support[m].tac);
        req->tai_support[m].num_plmn = ie->value.choice.TAISupport_List.list.array[m]->broadcastPLMNs.list.count;
        for (int j = 0; j < req->tai_support[m].num_plmn; j++) {
            XNAP_BroadcastPLMNinTAISupport_Item_t *broadcast_plmns =
                      ie->value.choice.TAISupport_List.list.array[m]->broadcastPLMNs.list.array[j];
            xnap_plmn_support_t *plmn_support =
                &req->tai_support[m].plmn_support[j];
            PLMNID_TO_MCC_MNC(&broadcast_plmns->plmn_id,
                               plmn_support->plmn.mcc,
                               plmn_support->plmn.mnc,
                               plmn_support->plmn.mnc_digit_length);
            plmn_support->num_nssai =
                broadcast_plmns->tAISliceSupport_List.list.count;
            for (int k = 0; k < plmn_support->num_nssai; k++) {
                OCTET_STRING_TO_INT8(
                    &broadcast_plmns->tAISliceSupport_List.list.array[k]->sst,
                    plmn_support->s_nssai[k].sst);
                if (broadcast_plmns->tAISliceSupport_List.list.array[k]->sd) {
                    OCTET_STRING_TO_INT24(
                        broadcast_plmns->tAISliceSupport_List.list.array[k]->sd,
                        plmn_support->s_nssai[k].sd);
                }
            }
        }
    }
  }

  
  /* AMF region info */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_XnSetupRequest_IEs_t, ie, xnSetupRequest, XNAP_ProtocolIE_ID_id_AMF_Region_Information, true); 
  if(ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_AMF_Region_Information is NULL pointer \n");
    return -1;
  } else {
    req->num_amf_regions = ie->value.choice.AMF_Region_Information.list.count;
    for (int a = 0; a < req->num_amf_regions; a++){
        XNAP_GlobalAMF_Region_Information_t *amf_region_info = ie->value.choice.AMF_Region_Information.list.array[a];
        PLMNID_TO_MCC_MNC(&amf_region_info->plmn_ID,
                          req->amf_region_info[a].plmn.mcc,
                          req->amf_region_info[a].plmn.mnc,
                          req->amf_region_info[a].plmn.mnc_digit_length);
        OCTET_STRING_TO_INT8(&amf_region_info->amf_region_id, req->amf_region_info[a].amf_region_id);
    }
  } 
  xnap_gNB_instance_t *instance_p = xnap_gNB_get_instance(instance); 
  instance_p->xn_target_gnb_associated_nb++;
  xnap_gNB_data_t *xnap_gnb_data_p = xnap_get_gNB(instance, assoc_id);
  xnap_gnb_data_p->state = XNAP_GNB_STATE_CONNECTED;
  LOG_I(XNAP, "The Number of Associated Target gNBs: %d\n", instance_p->xn_target_gnb_associated_nb);
  itti_send_msg_to_task(TASK_RRC_GNB, instance_p->instance, message_p);
  return 0;
}

int xnap_gNB_handle_xn_setup_response(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, XNAP_XnAP_PDU_t *pdu)
{
  XNAP_XnSetupResponse_t *xnSetupResponse = &pdu->choice.successfulOutcome->value.choice.XnSetupResponse;
  XNAP_XnSetupResponse_IEs_t *ie;
  uint32_t gNB_id = 0;
  MessageDef *msg = itti_alloc_new_message(TASK_XNAP, 0, XNAP_SETUP_RESP);
  msg->ittiMsgHeader.originInstance = assoc_id;
  xnap_setup_resp_t *resp = &XNAP_SETUP_RESP(msg);
  xnap_gNB_instance_t *instance_p = xnap_gNB_get_instance(instance);
  xnap_gNB_data_t *xnap_gnb_data_p = xnap_get_gNB(instance, assoc_id);
  xnap_gnb_data_p->state = XNAP_GNB_STATE_CONNECTED;

  /* Global NG-RAN Node ID */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_XnSetupResponse_IEs_t, ie, xnSetupResponse, XNAP_ProtocolIE_ID_id_GlobalNG_RAN_node_ID, true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_GlobalNG_RAN_node_ID is NULL pointer \n");
    return -1;
  } else {
    if (ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.present == XNAP_GNB_ID_Choice_PR_gnb_ID) {
      uint8_t *gNB_id_buf = ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.choice.gnb_ID.buf;
      if (ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.choice.gnb_ID.size != 28) {
        // TODO: handle case where size != 28 -> notify ? reject ?
      }
      resp->gNB_id = (gNB_id_buf[0] << 20) + (gNB_id_buf[1] << 12) + (gNB_id_buf[2] << 4) + ((gNB_id_buf[3] & 0xf0) >> 4);
    } else {
      // TODO if NSA setup
    }
  }

  /* TAI Support list */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_XnSetupResponse_IEs_t, ie, xnSetupResponse, XNAP_ProtocolIE_ID_id_TAISupport_list, true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_TAISupport_list is NULL pointer \n");
    return -1;
  } else {
    int tai_count = ie->value.choice.TAISupport_List.list.count;
    resp->num_tai = tai_count;
    for (int m = 0; m < tai_count; m++) {
        // TAC Decode
        OCTET_STRING_TO_INT24(&ie->value.choice.TAISupport_List.list.array[m]->tac,
                              resp->tai_support[m].tac);
        resp->tai_support[m].num_plmn = ie->value.choice.TAISupport_List.list.array[m]->broadcastPLMNs.list.count;
        for (int j = 0; j < resp->tai_support[m].num_plmn; j++) {
            XNAP_BroadcastPLMNinTAISupport_Item_t *broadcast_plmns =
                      ie->value.choice.TAISupport_List.list.array[m]->broadcastPLMNs.list.array[j];
            xnap_plmn_support_t *plmn_support =
                &resp->tai_support[m].plmn_support[j];
            PLMNID_TO_MCC_MNC(&broadcast_plmns->plmn_id,
                               plmn_support->plmn.mcc,
                               plmn_support->plmn.mnc,
                               plmn_support->plmn.mnc_digit_length);
            plmn_support->num_nssai =
                broadcast_plmns->tAISliceSupport_List.list.count;
            for (int k = 0; k < plmn_support->num_nssai; k++) {
                OCTET_STRING_TO_INT8(
                    &broadcast_plmns->tAISliceSupport_List.list.array[k]->sst,
                    plmn_support->s_nssai[k].sst);
                if (broadcast_plmns->tAISliceSupport_List.list.array[k]->sd) {
                    OCTET_STRING_TO_INT24(
                        broadcast_plmns->tAISliceSupport_List.list.array[k]->sd,
                        plmn_support->s_nssai[k].sd);
                }
            }
        }
    }
  }

  instance_p->xn_target_gnb_associated_nb++;
  itti_send_msg_to_task(TASK_RRC_GNB, instance_p->instance, msg);
  return 0;
}

int xnap_gNB_handle_xn_setup_failure(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, XNAP_XnAP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);
  XNAP_XnSetupFailure_t *xnSetupFailure;
  XNAP_XnSetupFailure_IEs_t *ie;
  xnap_gNB_data_t *xnap_gNB_data;

  xnSetupFailure = &pdu->choice.unsuccessfulOutcome->value.choice.XnSetupFailure;
  /*
   * We received a new valid XN Setup Failure on a stream != 0.
   * * * * This should not happen -> reject gNB xn setup failure.
   */
  if (stream != 0) {
    LOG_W(XNAP, "[SCTP %d] Received xn setup failure on stream != 0 (%d)\n", assoc_id, stream);
  }
  if ((xnap_gNB_data = xnap_get_gNB(instance, assoc_id)) == NULL) {
    LOG_E(XNAP,
          "[SCTP %d] Received XN setup failure for non existing "
          "gNB context\n",
          assoc_id);
    return -1;
  }
  if ((xnap_gNB_data->state == XNAP_GNB_STATE_CONNECTED) || (xnap_gNB_data->state == XNAP_GNB_STATE_READY)) {
    LOG_E(XNAP, "Received Unexpexted XN Setup Failure Message\n");
    return -1;
  }

  /* Cause */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_XnSetupFailure_IEs_t, ie, xnSetupFailure, XNAP_ProtocolIE_ID_id_Cause, true);
  if (ie == NULL) {
    LOG_E(XNAP, "%s %d: ie is a NULL pointer \n", __FILE__, __LINE__);
    return -1;
  }
  if ((ie->value.choice.Cause.present == XNAP_Cause_PR_misc)
      && (ie->value.choice.Cause.choice.misc == XNAP_CauseMisc_unspecified)) {
    LOG_E(XNAP, "Received XN setup failure for gNB ... gNB is not ready\n");
    exit(1);
  } else {
    LOG_E(XNAP, "Received xn setup failure for gNB... please check your parameters\n");
    exit(1);
  }
  xnap_gNB_data->state = XNAP_GNB_STATE_WAITING;
  xnap_handle_xn_setup_message(instance, assoc_id, 0);
  return 0;
}

int xnap_gNB_handle_handover_preparation(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, XNAP_XnAP_PDU_t *pdu)
{
  XNAP_HandoverRequest_t *xnHandoverRequest;
  XNAP_HandoverRequest_IEs_t *ie;
  xnap_gNB_instance_t *instance_p;
  xnap_id_manager *id_manager;
  int ue_id;
  XNAP_PDUSessionResourcesToBeSetup_Item_t *pdu_session_resources;
  XNAP_QoSFlowsToBeSetup_Item_t *qos_flows;
  XNAP_LastVisitedCell_Item_t *lastVisitedCell_Item;
  instance_p = xnap_gNB_get_instance(instance);
  updateXninst(0, NULL, NULL, assoc_id);

  DevAssert(pdu != NULL);
  xnHandoverRequest = &pdu->choice.initiatingMessage->value.choice.HandoverRequest;
  if (stream != 0) {
    LOG_E(XNAP, "Received new XN handover request on stream != 0\n");
    // sending handover failed
    MessageDef *message_p = itti_alloc_new_message(TASK_XNAP, 0, XNAP_HANDOVER_PREPARATION_FAILURE);
    message_p->ittiMsgHeader.originInstance = assoc_id;
    xnap_handover_preparation_failure_t *fail = &XNAP_HANDOVER_PREPARATION_FAILURE(message_p);
    fail->cause.type = XNAP_CAUSE_PROTOCOL;
    fail->cause.value = 6;
    itti_send_msg_to_task(TASK_XNAP, 0, message_p);
  }

  MessageDef *message_p = itti_alloc_new_message(TASK_XNAP, 0, XNAP_HANDOVER_REQ);
  message_p->ittiMsgHeader.originInstance = assoc_id;
  xnap_handover_req_t *req = &XNAP_HANDOVER_REQ(message_p);

  /* Source NG-RAN node UE XnAP ID reference */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverRequest_IEs_t,
                             ie,
                             xnHandoverRequest,
                             XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID,
                             true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID, is NULL pointer \n");
    return -1;
  } else {
    req->s_ng_node_ue_xnap_id = ie->value.choice.NG_RANnodeUEXnAPID;
  }
  id_manager = &instance_p->id_manager;
  xnap_id_manager_init(id_manager);
  ue_id = xnap_allocate_new_id(id_manager);
  if (ue_id == -1) {
    LOG_E(XNAP, "could not allocate a new XNAP UE ID\n");
    exit(1);
  }
  req->ue_id = ue_id;
  req->t_ng_node_ue_xnap_id = ue_id;

  /* Target Cell Global ID */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverRequest_IEs_t, ie, xnHandoverRequest, XNAP_ProtocolIE_ID_id_targetCellGlobalID, true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_TargetCellCGI, is NULL pointer \n");
    return -1;
  } else {
    PLMNID_TO_MCC_MNC(&ie->value.choice.Target_CGI.choice.nr->plmn_id,
                      req->target_cgi.plmn_id.mcc,
                      req->target_cgi.plmn_id.mnc,
                      req->target_cgi.plmn_id.mnc_digit_length);
    BIT_STRING_TO_NR_CELL_IDENTITY(&ie->value.choice.Target_CGI.choice.nr->nr_CI, req->target_cgi.nrcell_id);
    LOG_D(XNAP, "XNAP_ProtocolIE_ID_id_TargetCellCGI, not a null pointer \n");
  }

  /* GUAMI */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverRequest_IEs_t, ie, xnHandoverRequest, XNAP_ProtocolIE_ID_id_GUAMI, true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_GUAMI is NULL pointer \n");
    return -1;
  } else {
    LOG_D(XNAP, "XNAP_ProtocolIE_ID_id_GUAMI not a null pointer \n");
    PLMNID_TO_MCC_MNC(&ie->value.choice.GUAMI.plmn_ID, req->guami.plmn.mcc, req->guami.plmn.mnc, req->guami.plmn.mnc_digit_length);
    req->guami.amf_region_id = BIT_STRING_to_uint8(&ie->value.choice.GUAMI.amf_region_id);
    req->guami.amf_set_id = BIT_STRING_to_uint16(&ie->value.choice.GUAMI.amf_set_id);
    req->guami.amf_pointer = BIT_STRING_to_uint8(&ie->value.choice.GUAMI.amf_pointer);
  }

  /* UE context Information */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverRequest_IEs_t, ie, xnHandoverRequest, XNAP_ProtocolIE_ID_id_UEContextInfoHORequest, true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_UEContextInfoHORequest, is NULL pointer \n");
    return -1;
  }
  {
    /* NG-C UE associated Signalling reference - AMF UE NGAP ID */
    asn_INTEGER2uint64(&ie->value.choice.UEContextInfoHORequest.ng_c_UE_reference, &req->ue_context.ngc_ue_sig_ref);

    /* Signalling TNL association address at source NG-C side - CP Transport Layer Information */
    BIT_STRING_TO_TRANSPORT_LAYER_ADDRESS_IPv4(&ie->value.choice.UEContextInfoHORequest.cp_TNL_info_source.choice.endpointIPAddress,
                                               *(long *)req->ue_context.tnl_ip_source.buffer);

    /* AS iSecurity */
    if ((ie->value.choice.UEContextInfoHORequest.securityInformation.key_NG_RAN_Star.buf)
        && (ie->value.choice.UEContextInfoHORequest.securityInformation.key_NG_RAN_Star.size == 32)) {
      memcpy(&req->ue_context.as_security_key_ranstar,
             ie->value.choice.UEContextInfoHORequest.securityInformation.key_NG_RAN_Star.buf,
             32);
    } else {
      LOG_E(XNAP, "Size of key star does not match the expected value\n");
    }
    
    if (ie->value.choice.UEContextInfoHORequest.securityInformation.ncc > 0) {
      req->ue_context.as_security_ncc = ie->value.choice.UEContextInfoHORequest.securityInformation.ncc;
    } else {
      req->ue_context.as_security_ncc = 0;
    }
   
    /* UESecurityCapabilities */
    req->ue_context.security_capabilities.nRencryption_algorithms =
        BIT_STRING_to_uint16(&ie->value.choice.UEContextInfoHORequest.ueSecurityCapabilities.nr_EncyptionAlgorithms);
    req->ue_context.security_capabilities.nRintegrity_algorithms =
        BIT_STRING_to_uint16(&ie->value.choice.UEContextInfoHORequest.ueSecurityCapabilities.nr_IntegrityProtectionAlgorithms);
    req->ue_context.security_capabilities.eUTRAencryption_algorithms =
        BIT_STRING_to_uint16(&ie->value.choice.UEContextInfoHORequest.ueSecurityCapabilities.e_utra_EncyptionAlgorithms);
    req->ue_context.security_capabilities.eUTRAintegrity_algorithms =
        BIT_STRING_to_uint16(&ie->value.choice.UEContextInfoHORequest.ueSecurityCapabilities.e_utra_IntegrityProtectionAlgorithms);

    /* RRC Context */
    OCTET_STRING_t *rrc_context = &ie->value.choice.UEContextInfoHORequest.rrc_Context;
    
    req->ue_context.rrc_context = create_byte_array(rrc_context->size, rrc_context->buf);
    NR_HandoverPreparationInformation_t *hoPrepInformation = NULL;
    asn_dec_rval_t hoPrep_dec_rval = uper_decode_complete(NULL,
                                                      &asn_DEF_NR_HandoverPreparationInformation,
                                                      (void **)&hoPrepInformation,
                                                      rrc_context->buf,
                                                      rrc_context->size);
    AssertFatal(hoPrep_dec_rval.code == RC_OK && hoPrep_dec_rval.consumed > 0, "Handover Prep Info decode error\n");
    if (hoPrep_dec_rval.code != RC_OK && !hoPrep_dec_rval.consumed) {
      LOG_E(XNAP, "Failed to decode HandoverPreparationInformation, abort Handover Request decoding\n");
      free_byte_array(req->ue_context.rrc_context);
      ASN_STRUCT_FREE(asn_DEF_NR_HandoverPreparationInformation, hoPrepInformation);
      return -1;
    }
   
    xer_fprint(stdout, &asn_DEF_NR_HandoverPreparationInformation, hoPrepInformation);
    if (LOG_DEBUGFLAG(DEBUG_ASN1))
       xer_fprint(stdout, &asn_DEF_NR_HandoverPreparationInformation, hoPrepInformation);

    // Decode UE capabilities and store
    NR_HandoverPreparationInformation_IEs_t *hpi = hoPrepInformation->criticalExtensions.choice.c1->choice.handoverPreparationInformation;
    const NR_UE_CapabilityRAT_ContainerList_t *ue_CapabilityRAT_ContainerList = &hpi->ue_CapabilityRAT_List;
    req->ue_context.ue_cap.len = uper_encode_to_new_buffer(&asn_DEF_NR_UE_CapabilityRAT_ContainerList,
                                              NULL,
                                              ue_CapabilityRAT_ContainerList,
                                              (void **)&req->ue_context.ue_cap.buf);

    ASN_STRUCT_FREE(asn_DEF_NR_HandoverPreparationInformation, hoPrepInformation);
    if (req->ue_context.ue_cap.len <= 0) {
        free_byte_array(req->ue_context.rrc_context);
        free_byte_array(req->ue_context.ue_cap);
        LOG_E(XNAP, "could not encode UE-CapabilityRAT-ContainerList\n");
        return -1;
    }

    /* PDU session resources to be setup list */
    if (ie->value.choice.UEContextInfoHORequest.pduSessionResourcesToBeSetup_List.list.count > 0) {
      req->ue_context.pdusession_tobe_setup_list.num_pdu =
          ie->value.choice.UEContextInfoHORequest.pduSessionResourcesToBeSetup_List.list.count;
      for (int i = 0; i < ie->value.choice.UEContextInfoHORequest.pduSessionResourcesToBeSetup_List.list.count; i++) {
        pdu_session_resources = ie->value.choice.UEContextInfoHORequest.pduSessionResourcesToBeSetup_List.list.array[i];
        /* PDU Session id */
        req->ue_context.pdusession_tobe_setup_list.pdu[i].pdusession_id = pdu_session_resources->pduSessionId;
        /* SSNSAI */
        OCTET_STRING_TO_INT8(&pdu_session_resources->s_NSSAI.sst, req->ue_context.pdusession_tobe_setup_list.pdu[i].snssai.sst);
        /* UP TNL Information */
        BIT_STRING_TO_TRANSPORT_LAYER_ADDRESS_IPv4(&pdu_session_resources->uL_NG_U_TNLatUPF.choice.gtpTunnel->tnl_address,
                                                   *(long *)req->ue_context.pdusession_tobe_setup_list.pdu[i].n3_incoming.addr.buffer);
        OCTET_STRING_TO_INT32(&pdu_session_resources->uL_NG_U_TNLatUPF.choice.gtpTunnel->gtp_teid,
                              req->ue_context.pdusession_tobe_setup_list.pdu[i].n3_incoming.teid);
        /* PDU session type */
        req->ue_context.pdusession_tobe_setup_list.pdu[i].pdu_session_type = pdu_session_resources->pduSessionType;
        /* QOS flows to be setup */
        req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.num_qos =
            pdu_session_resources->qosFlowsToBeSetup_List.list.count;
        for (int j = 0; j < pdu_session_resources->qosFlowsToBeSetup_List.list.count; j++) {
          qos_flows = ie->value.choice.UEContextInfoHORequest.pduSessionResourcesToBeSetup_List.list.array[i]
                          ->qosFlowsToBeSetup_List.list.array[j];
          /* QFI */
          req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qfi = qos_flows->qfi;
          if(qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.non_dynamic){
            /*non dynamic*/
            req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.qos_type = NON_DYNAMIC;
            req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.non_dynamic.fiveqi = 
                qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.non_dynamic->fiveQI;
          }else{
            /*dynamic*/
            req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.qos_type = DYNAMIC;
          req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.dynamic.fiveqi =
             *qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->fiveQI;
          req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.dynamic.qos_priority_level =
              qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->priorityLevelQoS;
          req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.dynamic.packet_delay_budget =
              qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->packetDelayBudget;
          req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.dynamic.packet_error_rate.per_scalar = 
              qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->packetErrorRate.pER_Scalar;
          req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.dynamic.packet_error_rate.per_exponent =
              qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->packetErrorRate.pER_Exponent;
         }
         req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].allocation_retention_priority.priority_level =
           qos_flows->qosFlowLevelQoSParameters.allocationAndRetentionPrio.priorityLevel;
         req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].allocation_retention_priority.preemption_capability =
           qos_flows->qosFlowLevelQoSParameters.allocationAndRetentionPrio.pre_emption_capability;
         req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].allocation_retention_priority.preemption_vulnerability =
           qos_flows->qosFlowLevelQoSParameters.allocationAndRetentionPrio.pre_emption_vulnerability;
        }
      }
    }
  }

  /* UE History Information */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverRequest_IEs_t, ie, xnHandoverRequest, XNAP_ProtocolIE_ID_id_UEHistoryInformation, true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_UEHistoryInformation is NULL pointer \n");
    return -1;
  } else {
    if (ie->value.choice.UEHistoryInformation.list.count > 0) {
      for (int i = 0; i < ie->value.choice.UEHistoryInformation.list.count; i++) {
       lastVisitedCell_Item = ie->value.choice.UEHistoryInformation.list.array[i];
       //OCTET_STRING_TO_INT32(&lastVisitedCell_Item->choice.nG_RAN_Cell, req->uehistory_info.last_visited_cgi.nrcell_id);
       XNAP_LastVisitedNGRANCellInformation_t *nrInfo =
           &lastVisitedCell_Item->choice.nG_RAN_Cell;
       
       NGAP_LastVisitedNGRANCellInformation_t *decoded = NULL;
       
       asn_dec_rval_t dres = aper_decode_complete(
           NULL,
           &asn_DEF_NGAP_LastVisitedNGRANCellInformation,
           (void **)&decoded,
           nrInfo->buf,
           nrInfo->size
       );
       if(dres.code != RC_OK) {
           LOG_E(XNAP, "Failed to decode NGAP_LastVisitedNGRANCellInformation\n");
           return -1;
       }
       xer_fprint(stdout,
           &asn_DEF_NGAP_LastVisitedNGRANCellInformation,
           decoded);

       /* Cell type */
       req->uehistory_info.cell_type = decoded->cellType.cellSize;
       
       /* CGI */
       NGAP_NR_CGI_t *cgi = decoded->globalCellID.choice.nR_CGI;
       PLMNID_TO_MCC_MNC(&cgi->pLMNIdentity,
                            req->uehistory_info.last_visited_cgi.plmn_id.mcc,
                            req->uehistory_info.last_visited_cgi.plmn_id.mnc,
                            req->uehistory_info.last_visited_cgi.plmn_id.mnc_digit_length);
       BIT_STRING_TO_NR_CELL_IDENTITY(&cgi->nRCellIdentity, req->uehistory_info.last_visited_cgi.nrcell_id);
       
       /* Stay time */
       req->uehistory_info.time_UE_StayedInCell =
             decoded->timeUEStayedInCell;
       
       ASN_STRUCT_FREE(asn_DEF_NGAP_LastVisitedNGRANCellInformation, decoded);
      }
    } else {
      LOG_D(XNAP, "XNAP_ProtocolIE_ID_id_UEHistoryInformation not a null pointer \n");
    }
  }

  itti_send_msg_to_task(TASK_RRC_GNB, instance_p->instance, message_p);
  return 0;
}

int xnap_gNB_handle_handover_preparation_response(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, XNAP_XnAP_PDU_t *pdu)
{
  XNAP_HandoverRequestAcknowledge_t *xnHandoverRequestAck;
  XNAP_HandoverRequestAcknowledge_IEs_t *ie;
  xnap_gNB_instance_t *instance_p;
  xnap_id_manager *id_manager;
  instance_p = xnap_gNB_get_instance(0);
  id_manager = &instance_p->id_manager;
  XNAP_PDUSessionResourcesAdmitted_Item_t *pdu_session_resources;
  XNAP_QoSFlowsAdmitted_Item_t *qos_flows;

  DevAssert(pdu != NULL);
  xnHandoverRequestAck = &pdu->choice.successfulOutcome->value.choice.HandoverRequestAcknowledge;
  MessageDef *message_p = itti_alloc_new_message(TASK_XNAP, 0, XNAP_HANDOVER_REQ_ACK);
  message_p->ittiMsgHeader.originInstance = assoc_id;
  xnap_handover_req_ack_t *ack = &XNAP_HANDOVER_REQ_ACK(message_p);

  /* Source NG-RAN node UE XnAP ID */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverRequestAcknowledge_IEs_t,
                             ie,
                             xnHandoverRequestAck,
                             XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID,
                             true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID, is NULL pointer \n");
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }
  ack->s_ng_node_ue_xnap_id = ie->value.choice.NG_RANnodeUEXnAPID;

  /* Target NG-RAN node UE XnAP ID */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverRequestAcknowledge_IEs_t,
                             ie,
                             xnHandoverRequestAck,
 			     XNAP_ProtocolIE_ID_id_targetNG_RANnodeUEXnAPID,
                             true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID, is NULL pointer \n");
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }
  ack->t_ng_node_ue_xnap_id = ie->value.choice.NG_RANnodeUEXnAPID_1;

  /*
  ack->ue_id = xnap_id_get_cu_ueid(id_manager,ack->s_ng_node_ue_xnap_id);
  xnap_set_ids(id_manager,
               ack->s_ng_node_ue_xnap_id,
               ack->ue_id,
               ack->s_ng_node_ue_xnap_id,
               ack->t_ng_node_ue_xnap_id);
  */

  /* PDU Session Resources Admitted List */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverRequestAcknowledge_IEs_t,
                             ie,
                             xnHandoverRequestAck,
                             XNAP_ProtocolIE_ID_id_PDUSessionResourcesAdmitted_List,
                             true);
  if (ie == NULL) {
    LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID, is NULL pointer \n");
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  } else {
    if (ie->value.choice.PDUSessionResourcesAdmitted_List.list.count > 0) {
      ack->pdusession_admitted_list.num_pdu = ie->value.choice.PDUSessionResourcesAdmitted_List.list.count;
      for (int i = 0; i < ie->value.choice.PDUSessionResourcesAdmitted_List.list.count; i++) {
        pdu_session_resources = ie->value.choice.PDUSessionResourcesAdmitted_List.list.array[i];
        /* PDU Session id */
        ack->pdusession_admitted_list.pdu[i].pdusession_id = pdu_session_resources->pduSessionId;
        /* QOS flows to be setup */
        ack->pdusession_admitted_list.pdu[i].qos_list.num_qos =
            pdu_session_resources->pduSessionResourceAdmittedInfo.qosFlowsAdmitted_List.list.count;
        for (int j = 0; j < pdu_session_resources->pduSessionResourceAdmittedInfo.qosFlowsAdmitted_List.list.count; j++) {
          qos_flows = ie->value.choice.PDUSessionResourcesAdmitted_List.list.array[i]
                          ->pduSessionResourceAdmittedInfo.qosFlowsAdmitted_List.list.array[j];
          /* QFI */
          ack->pdusession_admitted_list.pdu[i].qos_list.qos[j].qfi = qos_flows->qfi;
        }
      }
    }
  }

  /* Target NG-RAN node To Source NG-RAN node Transparent Container */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_HandoverRequestAcknowledge_IEs_t,
                             ie,
                             xnHandoverRequestAck,
                             XNAP_ProtocolIE_ID_id_Target2SourceNG_RANnodeTranspContainer,
                             true);
  if (ie == NULL) {
    LOG_E(XNAP, "iXNAP_ProtocolIE_ID_id_Target2SourceNG_RANnodeTranspContainer is NULL pointer \n");
    itti_free(ITTI_MSG_ORIGIN_ID(message_p), message_p);
    return -1;
  }

  OCTET_STRING_t *ho_command = &ie->value.choice.OCTET_STRING;
  ack->target2source = create_byte_array(ho_command->size, ho_command->buf);

  itti_send_msg_to_task(TASK_RRC_GNB, instance_p->instance, message_p);
  return 0;
}

int xnap_gNB_handle_ue_context_release(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, XNAP_XnAP_PDU_t *pdu)
{
  DevAssert(pdu != NULL);

  XNAP_UEContextRelease_t *uerelease;
  XNAP_UEContextRelease_IEs_t *ie;
 
  xnap_gNB_instance_t *instance_p = xnap_gNB_get_instance(instance);
  DevAssert(instance_p != NULL);

  MessageDef *msg_p = itti_alloc_new_message(TASK_XNAP, 0, XNAP_UE_CONTEXT_RELEASE);
  xnap_ue_context_release_t *msg = &XNAP_UE_CONTEXT_RELEASE(msg_p);
  

  uerelease = &pdu->choice.initiatingMessage->value.choice.UEContextRelease;
  if (stream != 0) {
    LOG_E(XNAP, "Received new xn ue context release on stream == 0\n");
    /* TODO: send a xn failure response */
    return 0;
  }

  /* Source NG-RAN node UE XnAP ID */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_UEContextRelease_IEs_t, ie, uerelease, XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID, true);
  if (ie == NULL) {
    LOG_E(XNAP, "%s %d: ie is a NULL pointer \n", __FILE__, __LINE__);
    itti_free(TASK_XNAP, msg_p);
    return -1;
  }
  msg->s_ng_node_ue_xnap_id = ie->value.choice.NG_RANnodeUEXnAPID;

  /* Target NG-RAN node UE XnAP ID */
  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_UEContextRelease_IEs_t, ie, uerelease, XNAP_ProtocolIE_ID_id_targetNG_RANnodeUEXnAPID, true);
  if (ie == NULL) {
    LOG_E(XNAP, "%s %d: ie is a NULL pointer \n", __FILE__, __LINE__);
    itti_free(TASK_XNAP, msg_p);
    return -1;
  }
  msg->t_ng_node_ue_xnap_id = ie->value.choice.NG_RANnodeUEXnAPID_1;
  itti_send_msg_to_task(TASK_RRC_GNB, instance_p->instance, msg_p);
  return 0;
}

/*  Xn code for SN Status transfer ** Handling SN STATUS TRANSFER MESSAGE */
int xnap_gNB_handle_sn_status_transfer(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, XNAP_XnAP_PDU_t *pdu)
{

  XNAP_SNStatusTransfer_t *handlesnstatustran;
  XNAP_SNStatusTransfer_IEs_t *ie;
  xnap_gNB_instance_t      *instance_p;
  instance_p = xnap_gNB_get_instance(instance);
  DevAssert(pdu != NULL);
  handlesnstatustran = &pdu->choice.initiatingMessage->value.choice.SNStatusTransfer;

  if(stream !=0)
      LOG_E(XNAP, "Received SN Status Transfer on stream != 0\n");

  LOG_I(XNAP, "Received SN Status Transfer\n");
  MessageDef *message_p = itti_alloc_new_message(TASK_XNAP, 0, XNAP_SN_STATUS_TRANSFER);
  xnap_sn_status_transfer_t *snstatustran = &XNAP_SN_STATUS_TRANSFER(message_p);

  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_SNStatusTransfer_IEs_t, ie, handlesnstatustran, XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID, true);
  if (ie == NULL) {
     LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID, is NULL pointer \n");
     return -1;
  }else{
     snstatustran->s_ng_node_ue_xnap_id = ie->value.choice.NG_RANnodeUEXnAPID;
  }

  XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_SNStatusTransfer_IEs_t, ie, handlesnstatustran, XNAP_ProtocolIE_ID_id_targetNG_RANnodeUEXnAPID, true);
  if (ie == NULL) {
     LOG_E(XNAP, "XNAP_ProtocolIE_ID_id_targetNG_RANnodeUEXnAPID, is NULL pointer \n");
     return -1;
  }else{
     snstatustran->t_ng_node_ue_xnap_id = ie->value.choice.NG_RANnodeUEXnAPID_1;
  }


   XNAP_FIND_PROTOCOLIE_BY_ID(XNAP_SNStatusTransfer_IEs_t, ie, handlesnstatustran, XNAP_ProtocolIE_ID_id_DRBsSubjectToStatusTransfer_List, true);
   if (ie != NULL) {
       int nb = ie->value.choice.DRBsSubjectToStatusTransfer_List.list.count; // need to update from the received
       for (int i = 0; i < nb; ++i) {
          struct XNAP_DRBsSubjectToStatusTransfer_Item *item = ie->value.choice.DRBsSubjectToStatusTransfer_List.list.array[i];
          xnap_drb_status_t *s = &snstatustran->ran_status.drb_status_list[i];
          s->drb_id = item->drbID;

          // UL COUNT
          switch (item->pdcpStatusTransfer_UL.present) {
            case XNAP_DRBBStatusTransferChoice_PR_pdcp_sn_18bits:
              s->ul_count.sn_len = XNAP_SN_LENGTH_18;
              s->ul_count.pdcp_sn = item->pdcpStatusTransfer_UL.choice.pdcp_sn_18bits->cOUNTValue.pdcp_SN18;
              s->ul_count.hfn = item->pdcpStatusTransfer_UL.choice.pdcp_sn_18bits->cOUNTValue.hfn_PDCP_SN18;
              break;
            case XNAP_DRBBStatusTransferChoice_PR_pdcp_sn_12bits:
              s->ul_count.sn_len = XNAP_SN_LENGTH_12;
              s->ul_count.pdcp_sn = item->pdcpStatusTransfer_UL.choice.pdcp_sn_12bits->cOUNTValue.pdcp_SN12;
              s->ul_count.hfn = item->pdcpStatusTransfer_UL.choice.pdcp_sn_12bits->cOUNTValue.hfn_PDCP_SN12;
              break;
            default:
              LOG_E(XNAP, "Unknown pdcpStatusTransfer_UL.present=%d\n", item->pdcpStatusTransfer_UL.present);
              break;
          }

          // DL COUNT
          switch (item->pdcpStatusTransfer_DL.present) {
            case XNAP_DRBBStatusTransferChoice_PR_pdcp_sn_18bits:
              s->dl_count.sn_len = XNAP_SN_LENGTH_18;
              s->dl_count.pdcp_sn = item->pdcpStatusTransfer_DL.choice.pdcp_sn_18bits->cOUNTValue.pdcp_SN18;
              s->dl_count.hfn = item->pdcpStatusTransfer_DL.choice.pdcp_sn_18bits->cOUNTValue.hfn_PDCP_SN18;
              break;
            case XNAP_DRBBStatusTransferChoice_PR_pdcp_sn_12bits:
              s->dl_count.sn_len = XNAP_SN_LENGTH_12;
              s->dl_count.pdcp_sn = item->pdcpStatusTransfer_DL.choice.pdcp_sn_12bits->cOUNTValue.pdcp_SN12;
              s->dl_count.hfn = item->pdcpStatusTransfer_DL.choice.pdcp_sn_12bits->cOUNTValue.hfn_PDCP_SN12;
              break;
            default:
              LOG_E(XNAP, "Unknown pdcpStatusTransfer_DL.present=%d\n", item->pdcpStatusTransfer_DL.present);
              break;
         }
         snstatustran->ran_status.nb_drb++;
       }
   }

   itti_send_msg_to_task(TASK_RRC_GNB, instance_p->instance, message_p);
   return 0;
}

