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
#include "intertask_interface.h"
#include "xnap_common.h"
#include "xnap_gNB_task.h"
#include "xnap_gNB_generate_messages.h"
#include "XNAP_ProtocolIE-Field.h"
#include "XNAP_GlobalgNB-ID.h"
#include "XNAP_ServedCells-NR-Item.h"
#include "XNAP_ServedCellInformation-NR.h"
#include "XNAP_NRFrequencyBandItem.h"
#include "xnap_gNB_itti_messaging.h"
#include "XNAP_ServedCells-NR.h"
#include "assertions.h"
#include "conversions.h"
#include "XNAP_BroadcastPLMNinTAISupport-Item.h"
#include "XNAP_TAISupport-Item.h"
#include "XNAP_GlobalAMF-Region-Information.h"
#include "XNAP_TargetCellList-Item.h"
#include "XNAP_GlobalAMF-Region-Information.h"
#include "XNAP_NRModeInfoFDD.h"
#include "XNAP_NRModeInfoTDD.h"
#include "openair2/RRC/NR/nr_rrc_defs.h"
#include "xnap_gNB_defs.h"
#include "XNAP_PDUSessionResourcesToBeSetup-Item.h"
#include "XNAP_QoSFlowsToBeSetup-Item.h"
#include "XNAP_LastVisitedCell-Item.h"
#include "XNAP_GTPtunnelTransportLayerInformation.h"
#include "XNAP_NonDynamic5QIDescriptor.h"
#include "XNAP_Dynamic5QIDescriptor.h"
#include "xnap_ids.h"
#include "NR_HandoverCommand.h"
#include "NR_CellGroupConfig.h"
#include "NR_RRCReconfiguration-IEs.h"
#include "NR_SpCellConfig.h"
#include "NR_ReconfigurationWithSync.h"
#include "NR_DL-DCCH-Message.h"
#include "SIMULATION/TOOLS/sim.h"
#include "NR_RACH-ConfigGeneric.h"
#include "XNAP_QoSFlowsAdmitted-Item.h"
#include "XNAP_PDUSessionResourcesAdmitted-Item.h"
#include "XNAP_DRBsSubjectToStatusTransfer-List.h"
#include "NGAP_LastVisitedNGRANCellInformation.h"
#include "NGAP_NGRAN-CGI.h"
#include "NGAP_NR-CGI.h"

XNAP_XnAP_PDU_t* encode_xn_handover_preparation_failure(const xnap_handover_preparation_failure_t *msg){
  
  XNAP_XnAP_PDU_t *pdu = malloc_or_fail(sizeof(*pdu));

  pdu->present = XNAP_XnAP_PDU_PR_unsuccessfulOutcome;
  asn1cCalloc(pdu->choice.unsuccessfulOutcome, head);
  head->procedureCode =  XNAP_ProcedureCode_id_handoverPreparation;
  head->criticality = XNAP_Criticality_reject;
  head->value.present = XNAP_UnsuccessfulOutcome__value_PR_HandoverPreparationFailure;
  XNAP_HandoverPreparationFailure_t *out = &head->value.choice.HandoverPreparationFailure;
  
  //SourceNG-RANnodeUEXnAPID
  {
   asn1cSequenceAdd(out->protocolIEs.list, XNAP_HandoverPreparationFailure_IEs_t, ie);
   ie->id = XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID;
   ie->criticality = XNAP_Criticality_reject;
   ie->value.present = XNAP_HandoverPreparationFailure_IEs__value_PR_NG_RANnodeUEXnAPID; 
   ie->value.choice.NG_RANnodeUEXnAPID =  msg->ng_node_ue_xnap_id;
  }
  
  //Cause
  {
   asn1cSequenceAdd(out->protocolIEs.list, XNAP_HandoverPreparationFailure_IEs_t, ie);
   ie->id = XNAP_ProtocolIE_ID_id_Cause;
   ie->criticality = XNAP_Criticality_reject;
   ie->value.present =  XNAP_HandoverPreparationFailure_IEs__value_PR_Cause;
   xnap_gNB_set_cause(&ie->value.choice.Cause, &msg->cause);   
  }

  return pdu; 
}

/* Xn code for SN status transfer **  SN STATUS TRANSFER MESSAGE GENERATION */
int xnap_gNB_generate_sn_status_transfer (sctp_assoc_t assoc_id, xnap_sn_status_transfer_t *xnap_sn_status_transfer)
{
  XNAP_XnAP_PDU_t pdu;
  XNAP_SNStatusTransfer_t *snstatustran;
  XNAP_SNStatusTransfer_IEs_t *ie;

  uint8_t *buffer;
  uint32_t len;
  int      ret = 0;

  LOG_I(XNAP, "Generating XNAP SN status Transfer for UE:%d\n", xnap_sn_status_transfer->s_ng_node_ue_xnap_id);

  /* Prepare the XnAP SN Status transfer message to encode */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = XNAP_XnAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (XNAP_InitiatingMessage_t *)calloc(1, sizeof(XNAP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = XNAP_ProcedureCode_id_sNStatusTransfer;
  pdu.choice.initiatingMessage->criticality = XNAP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = XNAP_InitiatingMessage__value_PR_SNStatusTransfer;
  snstatustran = &pdu.choice.initiatingMessage->value.choice.SNStatusTransfer;

  /* mandatory */
  ie = (XNAP_SNStatusTransfer_IEs_t *)calloc(1, sizeof(XNAP_SNStatusTransfer_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_SNStatusTransfer_IEs__value_PR_NG_RANnodeUEXnAPID;
  ie->value.choice.NG_RANnodeUEXnAPID = xnap_sn_status_transfer->s_ng_node_ue_xnap_id;// value to be added.
  asn1cSeqAdd(&snstatustran->protocolIEs.list, ie);

  /* mandatory */
  ie = (XNAP_SNStatusTransfer_IEs_t *)calloc(1, sizeof(XNAP_SNStatusTransfer_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_targetNG_RANnodeUEXnAPID;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_SNStatusTransfer_IEs__value_PR_NG_RANnodeUEXnAPID_1;
  ie->value.choice.NG_RANnodeUEXnAPID_1 = xnap_sn_status_transfer->t_ng_node_ue_xnap_id;// value to be added.
  asn1cSeqAdd(&snstatustran->protocolIEs.list, ie);

  /* mandatory */
  ie = (XNAP_SNStatusTransfer_IEs_t *)calloc(1, sizeof(XNAP_SNStatusTransfer_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_DRBsSubjectToStatusTransfer_List;
  ie->criticality = XNAP_Criticality_ignore;
  ie->value.present = XNAP_SNStatusTransfer_IEs__value_PR_DRBsSubjectToStatusTransfer_List;

  XNAP_DRBsSubjectToStatusTransfer_List_t *container = &ie->value.choice.DRBsSubjectToStatusTransfer_List;

  for (int i = 0; i < xnap_sn_status_transfer->ran_status.nb_drb; ++i) {
    const xnap_drb_status_t *s = &xnap_sn_status_transfer->ran_status.drb_status_list[i];
    asn1cSequenceAdd(container->list, XNAP_DRBsSubjectToStatusTransfer_Item_t, drb_item);
    drb_item->drbID = s->drb_id;

    // UL COUNT
    if (s->ul_count.sn_len == XNAP_SN_LENGTH_18) {
      drb_item->pdcpStatusTransfer_UL.present = XNAP_DRBBStatusTransferChoice_PR_pdcp_sn_18bits;
      asn1cCalloc(drb_item->pdcpStatusTransfer_UL.choice.pdcp_sn_18bits, ul18);
      ul18->cOUNTValue.pdcp_SN18 = s->ul_count.pdcp_sn;
      ul18->cOUNTValue.hfn_PDCP_SN18 = s->ul_count.hfn;
    } else {
      drb_item->pdcpStatusTransfer_UL.present = XNAP_DRBBStatusTransferChoice_PR_pdcp_sn_12bits;
      asn1cCalloc(drb_item->pdcpStatusTransfer_UL.choice.pdcp_sn_12bits, ul12);
      ul12->cOUNTValue.pdcp_SN12 = s->ul_count.pdcp_sn;
      ul12->cOUNTValue.hfn_PDCP_SN12 = s->ul_count.hfn;
    }

    // DL COUNT
    if (s->dl_count.sn_len == XNAP_SN_LENGTH_18) {
      drb_item->pdcpStatusTransfer_DL.present = XNAP_DRBBStatusTransferChoice_PR_pdcp_sn_18bits;
      asn1cCalloc(drb_item->pdcpStatusTransfer_DL.choice.pdcp_sn_18bits, dl18);
      dl18->cOUNTValue.pdcp_SN18 = s->dl_count.pdcp_sn;
      dl18->cOUNTValue.hfn_PDCP_SN18 = s->dl_count.hfn;
    } else {
      drb_item->pdcpStatusTransfer_DL.present = XNAP_DRBBStatusTransferChoice_PR_pdcp_sn_12bits;
      asn1cCalloc(drb_item->pdcpStatusTransfer_DL.choice.pdcp_sn_12bits, dl12);
      dl12->cOUNTValue.pdcp_SN12 = s->dl_count.pdcp_sn;
      dl12->cOUNTValue.hfn_PDCP_SN12 = s->dl_count.hfn;
    }
  }

  asn1cSeqAdd(&snstatustran->protocolIEs.list,ie);

  if (xnap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
     LOG_E(XNAP, "Failed to encode Xn SN Status Transfer message\n");
     return -1;
  }

  xnap_gNB_itti_send_sctp_data_req(assoc_id, buffer, len, 0); 
  return ret;
}


int xnap_gNB_generate_xn_setup_request(sctp_assoc_t assoc_id, xnap_setup_req_t *req)
{
  XNAP_XnAP_PDU_t pdu;
  XNAP_XnSetupRequest_t *out;
  XNAP_XnSetupRequest_IEs_t *ie;
  XNAP_BroadcastPLMNinTAISupport_Item_t *e_BroadcastPLMNinTAISupport_ItemIE;
  XNAP_TAISupport_Item_t *TAISupport_ItemIEs;
  XNAP_S_NSSAI_t *nssai;
  XNAP_GlobalAMF_Region_Information_t *e_GlobalAMF_Region_Information_ItemIEs;
  uint8_t *buffer;
  uint32_t len;
  int ret = 0;
  
  /* Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = XNAP_XnAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (XNAP_InitiatingMessage_t *)calloc(1, sizeof(XNAP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = XNAP_ProcedureCode_id_xnSetup;
  pdu.choice.initiatingMessage->criticality = XNAP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = XNAP_InitiatingMessage__value_PR_XnSetupRequest;
  out = &pdu.choice.initiatingMessage->value.choice.XnSetupRequest;

  /* mandatory */
  /* Global NG-RAN Node ID */
  ie = (XNAP_XnSetupRequest_IEs_t *)calloc(1, sizeof(XNAP_XnSetupRequest_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_GlobalNG_RAN_node_ID;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_XnSetupRequest_IEs__value_PR_GlobalNG_RANNode_ID;
  ie->value.choice.GlobalNG_RANNode_ID.present = XNAP_GlobalNG_RANNode_ID_PR_gNB;
  ie->value.choice.GlobalNG_RANNode_ID.choice.gNB = (XNAP_GlobalgNB_ID_t *)calloc(1, sizeof(XNAP_GlobalgNB_ID_t));
  MCC_MNC_TO_PLMNID(req->plmn.mcc,
                    req->plmn.mnc,
                    req->plmn.mnc_digit_length,
                    &ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->plmn_id);
  ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.present = XNAP_GNB_ID_Choice_PR_gnb_ID;
  MACRO_GNB_ID_TO_BIT_STRING(req->gNB_id, &ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.choice.gnb_ID); // 28 bits
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  /* mandatory */
  /* TAI Support list */
  ie = (XNAP_XnSetupRequest_IEs_t *)calloc(1, sizeof(XNAP_XnSetupRequest_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_TAISupport_list;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_XnSetupRequest_IEs__value_PR_TAISupport_List;
  for(int i = 0; i< req->num_tai; i++){
     TAISupport_ItemIEs = (XNAP_TAISupport_Item_t *)calloc(1, sizeof(XNAP_TAISupport_Item_t));
     INT24_TO_OCTET_STRING(req->tai_support[i].tac, &TAISupport_ItemIEs->tac);
     {
       for (int j = 0; j < req->tai_support[i].num_plmn; j++) {
         xnap_plmn_support_t *plmn_support = &req->tai_support[i].plmn_support[j];
         plmn_id_t  *plmn_id = &req->tai_support[i].plmn_support[j].plmn;
         e_BroadcastPLMNinTAISupport_ItemIE =
             (XNAP_BroadcastPLMNinTAISupport_Item_t *)calloc(1, sizeof(XNAP_BroadcastPLMNinTAISupport_Item_t));
         MCC_MNC_TO_PLMNID(plmn_id->mcc, plmn_id->mnc, plmn_id->mnc_digit_length,
                           &e_BroadcastPLMNinTAISupport_ItemIE->plmn_id);
         for (int k = 0; k < plmn_support->num_nssai; k++) {
           nssai = (XNAP_S_NSSAI_t *)calloc(1, sizeof(XNAP_S_NSSAI_t));
           INT8_TO_OCTET_STRING(plmn_support->s_nssai[k].sst, &nssai->sst);
           nssai->sd = calloc(1, sizeof(OCTET_STRING_t));
           if (!nssai->sd) return -1;
           INT24_TO_OCTET_STRING(plmn_support->s_nssai[k].sd, nssai->sd);
           asn1cSeqAdd(&e_BroadcastPLMNinTAISupport_ItemIE->tAISliceSupport_List.list, nssai);
         }
         asn1cSeqAdd(&TAISupport_ItemIEs->broadcastPLMNs.list, e_BroadcastPLMNinTAISupport_ItemIE);
       }
     }
     asn1cSeqAdd(&ie->value.choice.TAISupport_List.list, TAISupport_ItemIEs);
  }
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  /* mandatory */
  /* AMF Region Information */
  ie = (XNAP_XnSetupRequest_IEs_t *)calloc(1, sizeof(XNAP_XnSetupRequest_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_AMF_Region_Information;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_XnSetupRequest_IEs__value_PR_AMF_Region_Information;
  for(int i = 0; i < req->num_amf_regions; i++){
     xnap_amf_region_info_t *amf_region_info = &req->amf_region_info[i];
     e_GlobalAMF_Region_Information_ItemIEs =
         (XNAP_GlobalAMF_Region_Information_t *)calloc(1, sizeof(XNAP_GlobalAMF_Region_Information_t));

     MCC_MNC_TO_PLMNID(amf_region_info->plmn.mcc,
                       amf_region_info->plmn.mnc,
                       amf_region_info->plmn.mnc_digit_length,
                       &e_GlobalAMF_Region_Information_ItemIEs->plmn_ID);
     INT8_TO_OCTET_STRING(amf_region_info->amf_region_id, &e_GlobalAMF_Region_Information_ItemIEs->amf_region_id);
     asn1cSeqAdd(&ie->value.choice.AMF_Region_Information.list, e_GlobalAMF_Region_Information_ItemIEs);
  }
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  if (xnap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(XNAP, "Failed to encode Xn setup request\n");
    return -1;
  }
  xnap_gNB_itti_send_sctp_data_req(assoc_id, buffer, len, 0);
  return ret;
}

int xnap_gNB_generate_xn_setup_failure(sctp_assoc_t assoc_id, xnap_setup_failure_t *fail)
{
  XNAP_XnAP_PDU_t pdu;
  XNAP_XnSetupFailure_t *out;
  XNAP_XnSetupFailure_IEs_t *ie;
  uint8_t *buffer;
  uint32_t len;
  int ret = 0;

  /* Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = XNAP_XnAP_PDU_PR_unsuccessfulOutcome;
  pdu.choice.unsuccessfulOutcome = (XNAP_UnsuccessfulOutcome_t *)calloc(1, sizeof(XNAP_UnsuccessfulOutcome_t));
  pdu.choice.unsuccessfulOutcome->procedureCode = XNAP_ProcedureCode_id_xnSetup;
  pdu.choice.unsuccessfulOutcome->criticality = XNAP_Criticality_reject;
  pdu.choice.unsuccessfulOutcome->value.present = XNAP_UnsuccessfulOutcome__value_PR_XnSetupFailure;
  out = &pdu.choice.unsuccessfulOutcome->value.choice.XnSetupFailure;

  /* mandatory */
  /* Cause */
  ie = (XNAP_XnSetupFailure_IEs_t *)calloc(1, sizeof(XNAP_XnSetupFailure_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_Cause;
  ie->criticality = XNAP_Criticality_ignore;
  ie->value.present = XNAP_XnSetupFailure_IEs__value_PR_Cause;
  xnap_gNB_set_cause(&ie->value.choice.Cause, &fail->cause);
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  if (xnap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(XNAP, "Failed to encode Xn setup failure\n");
    return -1;
  }
  xnap_gNB_itti_send_sctp_data_req(assoc_id, buffer, len, 0);
  return ret;
}

int xnap_gNB_generate_xn_setup_response(sctp_assoc_t assoc_id, xnap_setup_resp_t *resp)
{
  XNAP_XnAP_PDU_t pdu;
  uint8_t *buffer = NULL;
  uint32_t len = 0;
  int ret = 0;
  XNAP_XnSetupResponse_t *out;
  XNAP_XnSetupResponse_IEs_t *ie;
  XNAP_BroadcastPLMNinTAISupport_Item_t *e_BroadcastPLMNinTAISupport_ItemIE;
  XNAP_TAISupport_Item_t *TAISupport_ItemIEs;
  XNAP_S_NSSAI_t *nssai;

  /* Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = XNAP_XnAP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (XNAP_SuccessfulOutcome_t *)calloc(1, sizeof(XNAP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome->procedureCode = XNAP_ProcedureCode_id_xnSetup;
  pdu.choice.successfulOutcome->criticality = XNAP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = XNAP_SuccessfulOutcome__value_PR_XnSetupResponse;
  out = &pdu.choice.successfulOutcome->value.choice.XnSetupResponse;

  /* mandatory */
  /* Global NG-RAN Node ID */
  ie = (XNAP_XnSetupResponse_IEs_t *)calloc(1, sizeof(XNAP_XnSetupResponse_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_GlobalNG_RAN_node_ID;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_XnSetupResponse_IEs__value_PR_GlobalNG_RANNode_ID;
  ie->value.choice.GlobalNG_RANNode_ID.present = XNAP_GlobalNG_RANNode_ID_PR_gNB;
  ie->value.choice.GlobalNG_RANNode_ID.choice.gNB = (XNAP_GlobalgNB_ID_t *)calloc(1, sizeof(XNAP_GlobalgNB_ID_t));
  MCC_MNC_TO_PLMNID(resp->plmn.mcc,
                    resp->plmn.mnc,
                    resp->plmn.mnc_digit_length,
                    &ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->plmn_id);
  ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.present = XNAP_GNB_ID_Choice_PR_gnb_ID;
  MACRO_GNB_ID_TO_BIT_STRING(resp->gNB_id, &ie->value.choice.GlobalNG_RANNode_ID.choice.gNB->gnb_id.choice.gnb_ID);
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  /* mandatory */
  /* TAI Support list */
  ie = (XNAP_XnSetupResponse_IEs_t *)calloc(1, sizeof(XNAP_XnSetupResponse_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_TAISupport_list;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_XnSetupResponse_IEs__value_PR_TAISupport_List;
  for(int i = 0; i< resp->num_tai; i++){
     TAISupport_ItemIEs = (XNAP_TAISupport_Item_t *)calloc(1, sizeof(XNAP_TAISupport_Item_t));
     INT24_TO_OCTET_STRING(resp->tai_support[i].tac, &TAISupport_ItemIEs->tac);
     {
       for (int j = 0; j < resp->tai_support[i].num_plmn; j++) {
         xnap_plmn_support_t *plmn_support = &resp->tai_support[i].plmn_support[j];
         plmn_id_t  *plmn_id = &resp->tai_support[i].plmn_support[j].plmn;
         e_BroadcastPLMNinTAISupport_ItemIE =
             (XNAP_BroadcastPLMNinTAISupport_Item_t *)calloc(1, sizeof(XNAP_BroadcastPLMNinTAISupport_Item_t));
         MCC_MNC_TO_PLMNID(plmn_id->mcc, plmn_id->mnc, plmn_id->mnc_digit_length,
                           &e_BroadcastPLMNinTAISupport_ItemIE->plmn_id);
         for (int k = 0; k < plmn_support->num_nssai; k++) {
           nssai = (XNAP_S_NSSAI_t *)calloc(1, sizeof(XNAP_S_NSSAI_t));
           INT8_TO_OCTET_STRING(plmn_support->s_nssai[k].sst, &nssai->sst);
           nssai->sd = calloc(1, sizeof(OCTET_STRING_t));
           if (!nssai->sd) return -1;
           INT24_TO_OCTET_STRING(plmn_support->s_nssai[k].sd, nssai->sd);
           asn1cSeqAdd(&e_BroadcastPLMNinTAISupport_ItemIE->tAISliceSupport_List.list, nssai);
         }
         asn1cSeqAdd(&TAISupport_ItemIEs->broadcastPLMNs.list, e_BroadcastPLMNinTAISupport_ItemIE);
       }
     }
     asn1cSeqAdd(&ie->value.choice.TAISupport_List.list, TAISupport_ItemIEs);
  }
  asn1cSeqAdd(&out->protocolIEs.list, ie);

  if (xnap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(XNAP, "Failed to encode Xn setup response\n");
    return -1;
  }
  xnap_gNB_itti_send_sctp_data_req(assoc_id, buffer, len, 0);
  return ret;
}

int xnap_gNB_generate_handover_request(sctp_assoc_t assoc_id, xnap_handover_req_t *xnap_handover_req)
{
  XNAP_XnAP_PDU_t pdu;
  XNAP_HandoverRequest_t *xnhandoverreq;
  XNAP_HandoverRequest_IEs_t *ie;
  XNAP_PDUSessionResourcesToBeSetup_Item_t *pdu_session_resources;
  XNAP_QoSFlowsToBeSetup_Item_t *qos_flows;
  XNAP_LastVisitedCell_Item_t *lastVisitedCell_Item;
  uint8_t *buffer;
  uint32_t len;
  int ret = 0;

  /* Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = XNAP_XnAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (XNAP_InitiatingMessage_t *)calloc(1, sizeof(XNAP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = XNAP_ProcedureCode_id_handoverPreparation;
  pdu.choice.initiatingMessage->criticality = XNAP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = XNAP_InitiatingMessage__value_PR_HandoverRequest;
  xnhandoverreq = &pdu.choice.initiatingMessage->value.choice.HandoverRequest;

  /* mandatory */
  /* Source NG-RAN node UE XnAP ID reference */
  ie = (XNAP_HandoverRequest_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequest_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_HandoverRequest_IEs__value_PR_NG_RANnodeUEXnAPID;
  ie->value.choice.NG_RANnodeUEXnAPID = xnap_handover_req->s_ng_node_ue_xnap_id; //// value to be added.
  asn1cSeqAdd(&xnhandoverreq->protocolIEs.list, ie);

  /* mandatory */
  /* Cause */
  ie = (XNAP_HandoverRequest_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequest_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_Cause;
  ie->criticality = XNAP_Criticality_ignore;
  ie->value.present = XNAP_HandoverRequest_IEs__value_PR_Cause;
  ie->value.choice.Cause.present = XNAP_Cause_PR_radioNetwork;
  ie->value.choice.Cause.choice.radioNetwork = 1; // Xnap_CauseRadioNetwork_handover_desirable_for_radio_reasons;
  asn1cSeqAdd(&xnhandoverreq->protocolIEs.list, ie);

  /* mandatory */
  /* Target Cell Global ID */
  ie = (XNAP_HandoverRequest_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequest_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_targetCellGlobalID;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_HandoverRequest_IEs__value_PR_Target_CGI;
  ie->value.choice.Target_CGI.present = XNAP_Target_CGI_PR_nr;
  ie->value.choice.Target_CGI.choice.nr = (XNAP_NR_CGI_t *)calloc(1, sizeof(XNAP_NR_CGI_t));
  MCC_MNC_TO_PLMNID(xnap_handover_req->target_cgi.plmn_id.mcc, /// correct
                    xnap_handover_req->target_cgi.plmn_id.mnc,
                    xnap_handover_req->target_cgi.plmn_id.mnc_digit_length,
                    &ie->value.choice.Target_CGI.choice.nr->plmn_id);
  NR_CELL_ID_TO_BIT_STRING(xnap_handover_req->target_cgi.nrcell_id,
                           &ie->value.choice.Target_CGI.choice.nr->nr_CI); // bit string
  asn1cSeqAdd(&xnhandoverreq->protocolIEs.list, ie);

  /* mandatory */
  /* GUAMI */
  ie = (XNAP_HandoverRequest_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequest_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_GUAMI;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_HandoverRequest_IEs__value_PR_GUAMI;
  MCC_MNC_TO_PLMNID(xnap_handover_req->guami.plmn.mcc,
                    xnap_handover_req->guami.plmn.mnc,
                    xnap_handover_req->guami.plmn.mnc_digit_length,
                    &ie->value.choice.GUAMI.plmn_ID);
  AMF_REGION_TO_BIT_STRING(xnap_handover_req->guami.amf_region_id, &ie->value.choice.GUAMI.amf_region_id);
  AMF_SETID_TO_BIT_STRING(xnap_handover_req->guami.amf_set_id, &ie->value.choice.GUAMI.amf_set_id);
  AMF_POINTER_TO_BIT_STRING(xnap_handover_req->guami.amf_pointer, &ie->value.choice.GUAMI.amf_pointer);
  asn1cSeqAdd(&xnhandoverreq->protocolIEs.list, ie);

  /* mandatory */
  /* UE context Information */
  ie = (XNAP_HandoverRequest_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequest_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_UEContextInfoHORequest;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_HandoverRequest_IEs__value_PR_UEContextInfoHORequest;
  {
    /* NG-C UE associated Signalling reference - AMF UE NGAP ID */
    asn_uint642INTEGER(&ie->value.choice.UEContextInfoHORequest.ng_c_UE_reference, xnap_handover_req->ue_context.ngc_ue_sig_ref);

    /* Signalling TNL association address at source NG-C side - CP Transport Layer Information */
    ie->value.choice.UEContextInfoHORequest.cp_TNL_info_source.present = XNAP_CPTransportLayerInformation_PR_endpointIPAddress;
    TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(
        *(long *)xnap_handover_req->ue_context.tnl_ip_source.buffer,
        &ie->value.choice.UEContextInfoHORequest.cp_TNL_info_source.choice.endpointIPAddress);

    /* AS Security */
    uint8_t AsSecurityKey[32] = {0};
    memcpy(AsSecurityKey,
           xnap_handover_req->ue_context.as_security_key_ranstar,
           sizeof(xnap_handover_req->ue_context.as_security_key_ranstar));
    KENB_STAR_TO_BIT_STRING(xnap_handover_req->ue_context.as_security_key_ranstar,
                            &ie->value.choice.UEContextInfoHORequest.securityInformation.key_NG_RAN_Star);
    if (xnap_handover_req->ue_context.as_security_ncc > 0) {
      ie->value.choice.UEContextInfoHORequest.securityInformation.ncc = xnap_handover_req->ue_context.as_security_ncc;
    } else {
      ie->value.choice.UEContextInfoHORequest.securityInformation.ncc = 0;
    }

    /* UESecurityCapabilities */
    ENCRALG_TO_BIT_STRING(xnap_handover_req->ue_context.security_capabilities.nRencryption_algorithms,
                          &ie->value.choice.UEContextInfoHORequest.ueSecurityCapabilities.nr_EncyptionAlgorithms);
    INTPROTALG_TO_BIT_STRING(xnap_handover_req->ue_context.security_capabilities.nRintegrity_algorithms,
                             &ie->value.choice.UEContextInfoHORequest.ueSecurityCapabilities.nr_IntegrityProtectionAlgorithms);
    ENCRALG_TO_BIT_STRING(xnap_handover_req->ue_context.security_capabilities.eUTRAencryption_algorithms,
                          &ie->value.choice.UEContextInfoHORequest.ueSecurityCapabilities.e_utra_EncyptionAlgorithms);
    INTPROTALG_TO_BIT_STRING(xnap_handover_req->ue_context.security_capabilities.eUTRAintegrity_algorithms,
                             &ie->value.choice.UEContextInfoHORequest.ueSecurityCapabilities.e_utra_IntegrityProtectionAlgorithms);

    /* RRC Context */
    OCTET_STRING_fromBuf(&ie->value.choice.UEContextInfoHORequest.rrc_Context,
                         (const char *)xnap_handover_req->ue_context.rrc_context.buf,
                         xnap_handover_req->ue_context.rrc_context.len);

    /* UE AMBR */
    UEAGMAXBITRTD_TO_ASN_PRIMITIVES(xnap_handover_req->ue_context.ue_ambr.br_dl,
                                    &ie->value.choice.UEContextInfoHORequest.ue_AMBR.dl_UE_AMBR);
    UEAGMAXBITRTU_TO_ASN_PRIMITIVES(xnap_handover_req->ue_context.ue_ambr.br_ul,
                                    &ie->value.choice.UEContextInfoHORequest.ue_AMBR.ul_UE_AMBR);

    /* PDU session resources to be setup list */
    for (int i = 0; i < xnap_handover_req->ue_context.pdusession_tobe_setup_list.num_pdu; i++) {
      pdu_session_resources =
          (XNAP_PDUSessionResourcesToBeSetup_Item_t *)calloc(1, sizeof(XNAP_PDUSessionResourcesToBeSetup_Item_t));
      /* PDU Session id */
      pdu_session_resources->pduSessionId = xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].pdusession_id;
      /* SSNSAI */
      INT8_TO_OCTET_STRING(xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].snssai.sst,
                           &pdu_session_resources->s_NSSAI.sst);
      /* UP TNL Information */
      pdu_session_resources->uL_NG_U_TNLatUPF.present = XNAP_UPTransportLayerInformation_PR_gtpTunnel;
      pdu_session_resources->uL_NG_U_TNLatUPF.choice.gtpTunnel =
          (XNAP_GTPtunnelTransportLayerInformation_t *)calloc(1, sizeof(XNAP_GTPtunnelTransportLayerInformation_t));
      TRANSPORT_LAYER_ADDRESS_IPv4_TO_BIT_STRING(*(long *)xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].n3_incoming.addr.buffer,
                                                 &pdu_session_resources->uL_NG_U_TNLatUPF.choice.gtpTunnel->tnl_address);
      INT32_TO_OCTET_STRING(xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].n3_incoming.teid,
                            &pdu_session_resources->uL_NG_U_TNLatUPF.choice.gtpTunnel->gtp_teid);
      /* PDU session type */
      pdu_session_resources->pduSessionType = xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].pdu_session_type;
      /* QOS flows to be setup */
      {
        for (int j = 0; j < xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.num_qos; j++) {
          qos_flows = (XNAP_QoSFlowsToBeSetup_Item_t *)calloc(1, sizeof(XNAP_QoSFlowsToBeSetup_Item_t));
          /* QFI */
          qos_flows->qfi = xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qfi;
          /* QOS flow level QOS parameters */
          if(xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.qos_type == NON_DYNAMIC){
          /* non-dynamic */
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.present = XNAP_QoSCharacteristics_PR_non_dynamic;
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.non_dynamic =
              (XNAP_NonDynamic5QIDescriptor_t *)calloc(1, sizeof(XNAP_NonDynamic5QIDescriptor_t));
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.non_dynamic->fiveQI =
              xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.non_dynamic.fiveqi;
          }else{
          /* dynamic */
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.present = XNAP_QoSCharacteristics_PR_dynamic;
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic =
                 (XNAP_Dynamic5QIDescriptor_t *)calloc(1, sizeof(XNAP_Dynamic5QIDescriptor_t));
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->fiveQI =
                 &xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.dynamic.fiveqi;
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->priorityLevelQoS =
                 xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].qos_params.dynamic.qos_priority_level;
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->packetDelayBudget =
                 xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i]
                     .qos_list.qos[j]
                     .qos_params.dynamic.packet_delay_budget;
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->packetErrorRate.pER_Scalar =
                 xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i]
                     .qos_list.qos[j]
                     .qos_params.dynamic.packet_error_rate.per_scalar;
             qos_flows->qosFlowLevelQoSParameters.qos_characteristics.choice.dynamic->packetErrorRate.pER_Exponent =
                 xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i]
                     .qos_list.qos[j]
                     .qos_params.dynamic.packet_error_rate.per_exponent; 
          }
          qos_flows->qosFlowLevelQoSParameters.allocationAndRetentionPrio.priorityLevel = 
              xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].allocation_retention_priority.priority_level;
          qos_flows->qosFlowLevelQoSParameters.allocationAndRetentionPrio.pre_emption_capability =
              xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].allocation_retention_priority.preemption_capability;
          qos_flows->qosFlowLevelQoSParameters.allocationAndRetentionPrio.pre_emption_vulnerability =
              xnap_handover_req->ue_context.pdusession_tobe_setup_list.pdu[i].qos_list.qos[j].allocation_retention_priority.preemption_vulnerability;
    
          asn1cSeqAdd(&pdu_session_resources->qosFlowsToBeSetup_List.list, qos_flows);
        }
      }
      asn1cSeqAdd(&ie->value.choice.UEContextInfoHORequest.pduSessionResourcesToBeSetup_List.list, pdu_session_resources);
    }
  }
  asn1cSeqAdd(&xnhandoverreq->protocolIEs.list, ie);

  /* mandatory */
  /* UE History Information */
  ie = (XNAP_HandoverRequest_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequest_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_UEHistoryInformation;
  ie->criticality = XNAP_Criticality_ignore;
  ie->value.present = XNAP_HandoverRequest_IEs__value_PR_UEHistoryInformation;
  {
    lastVisitedCell_Item = (XNAP_LastVisitedCell_Item_t *)calloc(1, sizeof(XNAP_LastVisitedCell_Item_t));
    lastVisitedCell_Item->present = XNAP_LastVisitedCell_Item_PR_nG_RAN_Cell;
    
    XNAP_LastVisitedNGRANCellInformation_t *nrInfo = &lastVisitedCell_Item->choice.nG_RAN_Cell;
    /********************************************************************
    * Build NGAP LastVisitedNGRANCellInformation for PER encoding
    ********************************************************************/
    NGAP_LastVisitedNGRANCellInformation_t ngap_info;
    memset(&ngap_info, 0, sizeof(ngap_info));
    
    /* Cell Type (M) */
    ngap_info.cellType.cellSize = xnap_handover_req->uehistory_info.cell_type;
    
    /* Global Cell ID (M) */
    ngap_info.globalCellID.present = NGAP_NGRAN_CGI_PR_nR_CGI;
    asn1cCalloc(ngap_info.globalCellID.choice.nR_CGI, ngap_cgi);

    MCC_MNC_TO_PLMNID(xnap_handover_req->uehistory_info.last_visited_cgi.plmn_id.mcc, /// correct
                    xnap_handover_req->uehistory_info.last_visited_cgi.plmn_id.mnc,
                    xnap_handover_req->uehistory_info.last_visited_cgi.plmn_id.mnc_digit_length,
                    &ngap_cgi->pLMNIdentity);
    NR_CELL_ID_TO_BIT_STRING(xnap_handover_req->uehistory_info.last_visited_cgi.nrcell_id,
                           &ngap_cgi->nRCellIdentity); // bit string
 
    
    /* Time UE stayed in cell (M) capped at 4095 */
    ngap_info.timeUEStayedInCell = xnap_handover_req->uehistory_info.time_UE_StayedInCell;

    /********************************************************************
     * PER ENCODE → produce OCTET STRING for XNAP
     ********************************************************************/
    uint8_t per_buffer[64];
    asn_enc_rval_t enc_ret = aper_encode_to_buffer(
        &asn_DEF_NGAP_LastVisitedNGRANCellInformation,
        NULL,
        &ngap_info,
        per_buffer,
        sizeof(per_buffer)
    );
    
    if (enc_ret.encoded <= 0) {
        LOG_E(XNAP, "Failed to encode LastVisitedNGRANCellInformation\n");
    }
    
    /* Convert encoded bits → bytes */
    int encoded_bytes = (enc_ret.encoded + 7) / 8;
    
    /********************************************************************
     * Fill XNAP OCTET STRING
     ********************************************************************/
    nrInfo->buf = malloc(encoded_bytes);
    nrInfo->size = encoded_bytes;
    memcpy(nrInfo->buf, per_buffer, encoded_bytes);
    asn1cSeqAdd(&ie->value.choice.UEHistoryInformation.list, lastVisitedCell_Item);
  }
  asn1cSeqAdd(&xnhandoverreq->protocolIEs.list, ie);

  if (xnap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(XNAP, "Failed to encode XN handover request\n");
    return -1;
  }

  xnap_gNB_itti_send_sctp_data_req(assoc_id, buffer, len, 0);
  return ret;
}

int xnap_gNB_generate_handover_request_ack(sctp_assoc_t assoc_id, xnap_handover_req_ack_t *xnap_handover_req_ack)
{
  LOG_D(XNAP, "Inside xnap_gNB_generate_handover_request_ack assoc id:%d \n", assoc_id);
  XNAP_XnAP_PDU_t pdu;
  XNAP_HandoverRequestAcknowledge_t *xnhandoverreqAck;
  XNAP_HandoverRequestAcknowledge_IEs_t *ie;
  XNAP_PDUSessionResourcesAdmitted_Item_t *pdu_session_resources;
  XNAP_QoSFlowsAdmitted_Item_t *qos_flows;
  uint8_t *buffer;
  uint32_t len;
  int ret = 0;

  /* Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = XNAP_XnAP_PDU_PR_successfulOutcome;
  pdu.choice.successfulOutcome = (XNAP_SuccessfulOutcome_t *)calloc(1, sizeof(XNAP_SuccessfulOutcome_t));
  pdu.choice.successfulOutcome->procedureCode = XNAP_ProcedureCode_id_handoverPreparation;
  pdu.choice.successfulOutcome->criticality = XNAP_Criticality_reject;
  pdu.choice.successfulOutcome->value.present = XNAP_SuccessfulOutcome__value_PR_HandoverRequestAcknowledge;
  xnhandoverreqAck = &pdu.choice.successfulOutcome->value.choice.HandoverRequestAcknowledge;

  /* mandatory */
  /* Source NG-RAN node UE XnAP ID */
  ie = (XNAP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequestAcknowledge_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID;
  ie->criticality = XNAP_Criticality_ignore;
  ie->value.present = XNAP_HandoverRequest_IEs__value_PR_NG_RANnodeUEXnAPID;
  ie->value.choice.NG_RANnodeUEXnAPID = xnap_handover_req_ack->s_ng_node_ue_xnap_id; // id_source;
  asn1cSeqAdd(&xnhandoverreqAck->protocolIEs.list, ie);

  /* mandatory */
  /* Target NG-RAN node UE XnAP ID */
  ie = (XNAP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequestAcknowledge_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_targetNG_RANnodeUEXnAPID;
  ie->criticality = XNAP_Criticality_ignore;
  ie->value.present = XNAP_HandoverRequestAcknowledge_IEs__value_PR_NG_RANnodeUEXnAPID_1;
  ie->value.choice.NG_RANnodeUEXnAPID_1 = xnap_handover_req_ack->t_ng_node_ue_xnap_id; // id_target;
  asn1cSeqAdd(&xnhandoverreqAck->protocolIEs.list, ie);

  /* mandatory */
  /* PDU Session Resources Admitted List */
  ie = (XNAP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequestAcknowledge_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_PDUSessionResourcesAdmitted_List;
  ie->criticality = XNAP_Criticality_ignore;
  ie->value.present = XNAP_HandoverRequestAcknowledge_IEs__value_PR_PDUSessionResourcesAdmitted_List;
  {
    for (int i = 0; i < xnap_handover_req_ack->pdusession_admitted_list.num_pdu; i++) {
      pdu_session_resources = (XNAP_PDUSessionResourcesAdmitted_Item_t *)calloc(1, sizeof(XNAP_PDUSessionResourcesAdmitted_Item_t));
      /* PDU Session id */
      pdu_session_resources->pduSessionId = xnap_handover_req_ack->pdusession_admitted_list.pdu[i].pdusession_id;
      /* QOS flows to be setup */
      for (int j = 0; j < xnap_handover_req_ack->pdusession_admitted_list.pdu[i].qos_list.num_qos; j++) {
        qos_flows = (XNAP_QoSFlowsAdmitted_Item_t *)calloc(1, sizeof(XNAP_QoSFlowsAdmitted_Item_t));
        /* QFI */
        qos_flows->qfi = xnap_handover_req_ack->pdusession_admitted_list.pdu[i].qos_list.qos[j].qfi;
        asn1cSeqAdd(&pdu_session_resources->pduSessionResourceAdmittedInfo.qosFlowsAdmitted_List.list, qos_flows);
      }
      asn1cSeqAdd(&ie->value.choice.PDUSessionResourcesAdmitted_List.list, pdu_session_resources);
    }
  }
  asn1cSeqAdd(&xnhandoverreqAck->protocolIEs.list, ie);

  /* mandatory */
  /* Target NG-RAN node To Source NG-RAN node Transparent Container */
  ie = (XNAP_HandoverRequestAcknowledge_IEs_t *)calloc(1, sizeof(XNAP_HandoverRequestAcknowledge_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_Target2SourceNG_RANnodeTranspContainer;
  ie->criticality = XNAP_Criticality_ignore;
  ie->value.present = XNAP_HandoverRequestAcknowledge_IEs__value_PR_OCTET_STRING;
  //is this correct ? 
  OCTET_STRING_fromBuf(&ie->value.choice.OCTET_STRING,
                       (char *)xnap_handover_req_ack->target2source.buf,
                       xnap_handover_req_ack->target2source.len);
  asn1cSeqAdd(&xnhandoverreqAck->protocolIEs.list, ie);

  if (xnap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(XNAP, "Failed to encode XN handover request ack\n");
    return -1;
  }
  xnap_gNB_itti_send_sctp_data_req(assoc_id, buffer, len, 0);
  return ret;
}

int xnap_gNB_generate_ue_context_release(sctp_assoc_t assoc_id, xnap_ue_context_release_t *xnap_ue_context_release)
{
  XNAP_XnAP_PDU_t pdu;
  XNAP_UEContextRelease_t *in;
  XNAP_UEContextRelease_IEs_t *ie;
  uint8_t *buffer;
  uint32_t len;

  /* Message Type */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = XNAP_XnAP_PDU_PR_initiatingMessage;
  pdu.choice.initiatingMessage = (XNAP_InitiatingMessage_t *)calloc(1, sizeof(XNAP_InitiatingMessage_t));
  pdu.choice.initiatingMessage->procedureCode = XNAP_ProcedureCode_id_uEContextRelease;
  pdu.choice.initiatingMessage->criticality = XNAP_Criticality_reject;
  pdu.choice.initiatingMessage->value.present = XNAP_InitiatingMessage__value_PR_UEContextRelease;
  in = &pdu.choice.initiatingMessage->value.choice.UEContextRelease;

  /* mandatory */
  /* Source NG-RAN node UE XnAP ID */
  ie = (XNAP_UEContextRelease_IEs_t *)calloc(1, sizeof(XNAP_UEContextRelease_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_sourceNG_RANnodeUEXnAPID;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_UEContextRelease_IEs__value_PR_NG_RANnodeUEXnAPID;
  ie->value.choice.NG_RANnodeUEXnAPID = xnap_ue_context_release->s_ng_node_ue_xnap_id;
  asn1cSeqAdd(&in->protocolIEs.list, ie);

  /* mandatory */
  /* Target NG-RAN node UE XnAP ID */
  ie = (XNAP_UEContextRelease_IEs_t *)calloc(1, sizeof(XNAP_UEContextRelease_IEs_t));
  ie->id = XNAP_ProtocolIE_ID_id_targetNG_RANnodeUEXnAPID;
  ie->criticality = XNAP_Criticality_reject;
  ie->value.present = XNAP_UEContextRelease_IEs__value_PR_NG_RANnodeUEXnAPID_1;
  ie->value.choice.NG_RANnodeUEXnAPID_1 = xnap_ue_context_release->t_ng_node_ue_xnap_id;
  asn1cSeqAdd(&in->protocolIEs.list, ie);

  if (xnap_gNB_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(XNAP, "Failed to encode UE context release\n");
    return -1;
  }
  xnap_gNB_itti_send_sctp_data_req(assoc_id, buffer, len, 0);
  return 0;
}
