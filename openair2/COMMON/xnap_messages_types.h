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

/*! \file xnap_messages_types.h
 * \author Sreeshma Shiv <sreeshmau@iisc.ac.in>
 * \date August 2023
 * \version 1.0
 */
#ifndef XNAP_MESSAGES_TYPES_H_
#define XNAP_MESSAGES_TYPES_H_

#include "common/5g_platform_types.h"
#include "common/utils/ds/byte_array.h"
#include "s1ap_messages_types.h"
#include "f1ap_messages_types.h"
#include "ngap_messages_types.h"

#define XNAP_REGISTER_GNB_REQ(mSGpTR) (mSGpTR)->ittiMsg.xnap_register_gnb_req
#define XNAP_SETUP_REQ(mSGpTR) (mSGpTR)->ittiMsg.xnap_setup_req
#define XNAP_SETUP_RESP(mSGpTR) (mSGpTR)->ittiMsg.xnap_setup_resp
#define XNAP_SETUP_FAILURE(mSGpTR) (mSGpTR)->ittiMsg.xnap_setup_failure
#define XNAP_HANDOVER_REQ(mSGpTR) (mSGpTR)->ittiMsg.xnap_handover_req
#define XNAP_HANDOVER_PREPARATION_FAILURE(mSGpTR) (mSGpTR)->ittiMsg.xnap_handover_preparation_failure
#define XNAP_HANDOVER_REQ_ACK(mSGpTR) (mSGpTR)->ittiMsg.xnap_handover_req_ack
#define XNAP_UE_CONTEXT_RELEASE(mSGpTR) (mSGpTR)->ittiMsg.xnap_ue_context_release
#define XNAP_MAX_NB_GNB_IP_ADDRESS 4

#define XNAP_SN_STATUS_TRANSFER(mSGpTR) (mSGpTR)->ittiMsg.xnap_sn_status_transfer /* Xn code for SN status transfer procedures */
#define XNAP_LOST_CONNECTION(mSGpTR) (mSGpTR)->ittiMsg.xnap_lost_connection

#define MAX_NSSAI_SUPPORTED 1024
#define MAX_PLMN_SUPPORTED   12
#define MAX_TAI_SUPPORTED    256
#define MAX_AMF_REGION_IDS   16

// XNAP layer -> RRC layer
typedef struct xnap_lost_connection_s{
  sctp_assoc_t assoc_id;
} xnap_lost_connection_t;

// gNB application layer -> XNAP messages
typedef struct xnap_net_ip_address_s {
  unsigned ipv4: 1;
  unsigned ipv6: 1;
  char ipv4_address[16];
  char ipv6_address[46];
} xnap_net_ip_address_t;

typedef struct xnap_sctp_s {
  uint16_t sctp_in_streams;
  uint16_t sctp_out_streams;
} xnap_sctp_t;

typedef struct xnap_net_config_t {
  uint8_t nb_xn;
  char* gnb_xn_interface_ip_address;
  char* target_gnb_xn_ip_address[XNAP_MAX_NB_GNB_IP_ADDRESS];
  uint32_t gnb_port_for_XNC;
  xnap_sctp_t sctp_streams;
} xnap_net_config_t;

typedef struct xnap_amf_region_info_s {
  plmn_id_t plmn;
  uint8_t amf_region_id;
} xnap_amf_region_info_t;

typedef enum xnap_mode_t { XNAP_MODE_TDD = 0, XNAP_MODE_FDD = 1 } xnap_mode_t;

typedef struct xnap_nr_frequency_info_t {
  uint32_t arfcn;
  int band;
} xnap_nr_frequency_info_t;

typedef struct xnap_transmission_bandwidth_t {
  uint8_t scs;
  uint16_t nrb;
} xnap_transmission_bandwidth_t;

typedef struct xnap_fdd_info_t {
  xnap_nr_frequency_info_t ul_freqinfo;
  xnap_nr_frequency_info_t dl_freqinfo;
  xnap_transmission_bandwidth_t ul_tbw;
  xnap_transmission_bandwidth_t dl_tbw;
} xnap_fdd_info_t;

typedef struct xnap_tdd_info_t {
  xnap_nr_frequency_info_t freqinfo;
  xnap_transmission_bandwidth_t tbw;
} xnap_tdd_info_t;

typedef struct xnap_snssai_s {
  uint8_t sst;
  uint32_t sd;
} xnap_snssai_t;

typedef struct xnap_served_cell_info_t {
  plmn_id_t plmn;
  uint64_t nr_cellid; // NR Global Cell Id
  uint16_t nr_pci; // NR Physical Cell Ids
  /* Tracking area code */
  uint32_t tac;
  xnap_mode_t mode;
  union {
    xnap_fdd_info_t fdd;
    xnap_tdd_info_t tdd;
  };
  char *measurement_timing_information;
} xnap_served_cell_info_t;

typedef struct xnap_plmn_support_s{
  plmn_id_t plmn;
  uint16_t num_nssai;
  xnap_snssai_t s_nssai[MAX_NSSAI_SUPPORTED];
} xnap_plmn_support_t;

typedef struct xnap_tai_support_s{
  uint32_t tac;
  uint8_t num_plmn;
  xnap_plmn_support_t plmn_support[MAX_PLMN_SUPPORTED];
} xnap_tai_support_t;

typedef struct xnap_setup_info_s {
  uint32_t gNB_id;
  plmn_id_t plmn;
  uint16_t num_tai;
  xnap_tai_support_t tai_support[MAX_TAI_SUPPORTED];
  uint8_t  num_amf_regions;
  xnap_amf_region_info_t amf_region_info[MAX_AMF_REGION_IDS];
  uint8_t num_cells_available;
  xnap_served_cell_info_t info;
} xnap_setup_info_t;
  
typedef xnap_setup_info_t xnap_setup_req_t;
typedef xnap_setup_info_t xnap_setup_resp_t;

typedef struct xnap_register_gnb_req_s {
  xnap_setup_info_t setup_info;
  xnap_net_config_t net_config;
  char *gNB_name;
} xnap_register_gnb_req_t;

typedef enum xnap_cause_group_e {
  XNAP_CAUSE_NOTHING, /* No components present */
  XNAP_CAUSE_RADIO_NETWORK,
  XNAP_CAUSE_TRANSPORT,
  XNAP_CAUSE_PROTOCOL,
  XNAP_CAUSE_MISC,
} xnap_cause_group_t;

typedef struct xnap_cause_s {
  xnap_cause_group_t type;
  uint8_t value;
} xnap_cause_t;

typedef enum xnap_cause_radio_network_e {
    XNAP_CAUSE_RADIO_NETWORK_LAYER_CELL_NOT_AVAILABLE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_HANDOVER_DESIRABLE_FOR_RADIO_REASONS,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_HANDOVER_TARGET_NOT_ALLOWED,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_INVALID_AMF_SET_ID,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_NO_RADIO_RESOURCES_AVAILABLE_IN_TARGET_CELL,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_PARTIAL_HANDOVER,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_REDUCE_LOAD_IN_SERVING_CELL,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_RESOURCE_OPTIMISATION_HANDOVER,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_TIME_CRITICAL_HANDOVER,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_TXN_RELOCOVERALL_EXPIRY,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_TXN_RELOCPREP_EXPIRY,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UNKNOWN_GUAMI_ID,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UNKNOWN_LOCAL_NG_RAN_NODE_UE_XNAP_ID,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_INCONSISTENT_REMOTE_NG_RAN_NODE_UE_XNAP_ID,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_ENCRYPTION_AND_OR_INTEGRITY_PROTECTION_ALGORITHMS_NOT_SUPPORTED,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_PROTECTION_ALGORITHMS_NOT_SUPPORTED,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_MULTIPLE_PDU_SESSION_ID_INSTANCES,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UNKNOWN_PDU_SESSION_ID,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UNKNOWN_QOS_FLOW_ID,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_MULTIPLE_QOS_FLOW_ID_INSTANCES,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_SWITCH_OFF_ONGOING,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_NOT_SUPPORTED_5QI_VALUE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_TXN_DCOVERALL_EXPIRY,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_TXN_DCPREP_EXPIRY,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_ACTION_DESIRABLE_FOR_RADIO_REASONS,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_REDUCE_LOAD,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_RESOURCE_OPTIMISATION,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_TIME_CRITICAL_ACTION,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_TARGET_NOT_ALLOWED,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_NO_RADIO_RESOURCES_AVAILABLE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_INVALID_QOS_COMBINATION,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_ENCRYPTION_ALGORITHMS_NOT_SUPPORTED,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_PROCEDURE_CANCELLED,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_RRM_PURPOSE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_IMPROVE_USER_BIT_RATE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_USER_INACTIVITY,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_RADIO_CONNECTION_WITH_UE_LOST,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_FAILURE_IN_THE_RADIO_INTERFACE_PROCEDURE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_BEARER_OPTION_NOT_SUPPORTED,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UP_INTEGRITY_PROTECTION_NOT_POSSIBLE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UP_CONFIDENTIALITY_PROTECTION_NOT_POSSIBLE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_RESOURCES_NOT_AVAILABLE_FOR_THE_SLICE_S,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UE_MAX_IP_DATA_RATE_REASON,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_CP_INTEGRITY_PROTECTION_FAILURE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UP_INTEGRITY_PROTECTION_FAILURE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_SLICE_NOT_SUPPORTED_BY_NG_RAN,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_MN_MOBILITY,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_SN_MOBILITY,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_COUNT_REACHES_MAX_VALUE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UNKNOWN_OLD_NG_RAN_NODE_UE_XNAP_ID,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_PDCP_OVERLOAD,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_DRB_ID_NOT_AVAILABLE,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UNSPECIFIED,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_UE_CONTEXT_ID_NOT_KNOWN,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_NON_RELOCATION_OF_CONTEXT,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_CHO_CPC_RESOURCES_TOBECHANGED,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_RSN_NOT_AVAILABLE_FOR_THE_UP,
    XNAP_CAUSE_RADIO_NETWORK_LAYER_NPN_ACCESS_DENIED
} xnap_cause_radio_network_t;

typedef struct xnap_setup_failure_s {
  xnap_cause_t cause;
  uint16_t time_to_wait;
  uint16_t criticality_diagnostics;
} xnap_setup_failure_t;

typedef struct xnap_allocation_retention_priority_s {
  uint16_t priority_level;
  preemption_capability_t preemption_capability;
  preemption_vulnerability_t preemption_vulnerability;
} xnap_allocation_retention_priority_t;

typedef struct xnap_qos_characteristics_s {
  union {
    struct {
      long fiveqi;
      long qos_priority_level;
    } non_dynamic;
    struct {
      long fiveqi; // -1 -> optional
      long qos_priority_level;
      long packet_delay_budget;
      struct {
        long per_scalar;
        long per_exponent;
      } packet_error_rate;
    } dynamic;
  };
  fiveQI_t qos_type;
} xnap_qos_characteristics_t;

typedef struct xnap_qos_tobe_setup_item_s {
  long qfi;
  xnap_qos_characteristics_t qos_params;
  xnap_allocation_retention_priority_t allocation_retention_priority;
} xnap_qos_tobe_setup_item_t;

typedef struct xnap_qos_tobe_setup_list_s {
  uint8_t num_qos;
  xnap_qos_tobe_setup_item_t qos[QOSFLOW_MAX_VALUE]; // QOSFLOW_MAX_VALUE= 64 Put this?
} xnap_qos_tobe_setup_list_t;

typedef struct xnap_pdusession_tobe_setup_item_s {
  long pdusession_id;
  xnap_snssai_t snssai;
  gtpu_tunnel_t n3_incoming;
  pdu_session_type_t pdu_session_type;
  xnap_qos_tobe_setup_list_t qos_list;
} xnap_pdusession_tobe_setup_item_t;

typedef struct xnap_pdusession_tobe_setup_list_s {
  uint8_t num_pdu;
  xnap_pdusession_tobe_setup_item_t pdu[NGAP_MAX_PDU_SESSION]; // Is the limit ok?
} xnap_pdusession_tobe_setup_list_t;

typedef struct xnap_qos_admitted_item_s {
  long qfi;
} xnap_qos_admitted_item_t;

typedef struct xnap_qos_admitted_list_s {
  uint8_t num_qos;
  xnap_qos_admitted_item_t qos[QOSFLOW_MAX_VALUE]; // QOSFLOW_MAX_VALUE= 64 Put this?
} xnap_qos_admitted_list_t;

typedef struct xnap_pdusession_admitted_item_s {
  long pdusession_id;
  xnap_qos_admitted_list_t qos_list;
} xnap_pdusession_admitted_item_t;

typedef struct xnap_pdusession_admitted_list_s {
  uint8_t num_pdu;
  xnap_pdusession_admitted_item_t pdu[NGAP_MAX_PDU_SESSION]; // Is the limit ok?
} xnap_pdusession_admitted_list_t;

typedef struct xnap_ngran_cgi_t {
  plmn_id_t plmn_id;
  uint64_t nrcell_id;
} xnap_ngran_cgi_t;

typedef struct xnap_security_capabilities_s {
  uint16_t nRencryption_algorithms;
  uint16_t nRintegrity_algorithms;
  uint16_t eUTRAencryption_algorithms;
  uint16_t eUTRAintegrity_algorithms;
} xnap_security_capabilities_t;

typedef struct xnap_ambr_s {
  uint64_t br_ul;
  uint64_t br_dl;
} xnap_ambr_t;

typedef struct xnap_uehistory_info_s {
  xnap_ngran_cgi_t last_visited_cgi;
  cell_type_t cell_type;
  uint16_t time_UE_StayedInCell;
} xnap_uehistory_info_t;

typedef struct xnap_ue_context_info_s {
  uint64_t ngc_ue_sig_ref;
  transport_layer_addr_t tnl_ip_source;
  uint32_t tnl_port_source;
  xnap_security_capabilities_t security_capabilities;
  uint8_t as_security_key_ranstar[32];
  long as_security_ncc;
  xnap_ambr_t ue_ambr;
  byte_array_t rrc_context;
  byte_array_t ue_cap;
  xnap_pdusession_tobe_setup_list_t pdusession_tobe_setup_list;
} xnap_ue_context_info_t;

typedef struct xnap_ue_context_release_s {
  uint32_t s_ng_node_ue_xnap_id;
  uint32_t t_ng_node_ue_xnap_id;
} xnap_ue_context_release_t;

typedef struct xnap_handover_req_s {
  int ue_id; /* used for RRC->XNAP in source */
  // int xn_id;  /* used for XNAP->RRC in target*/
  uint32_t s_ng_node_ue_xnap_id;
  uint32_t t_ng_node_ue_xnap_id;
  xnap_cause_t cause;
  xnap_ngran_cgi_t target_cgi;
  nr_guami_t guami;
  xnap_ue_context_info_t ue_context;
  xnap_uehistory_info_t uehistory_info;
  sctp_assoc_t target_assoc_id;
} xnap_handover_req_t;

typedef struct xnap_handover_req_ack_s {
  uint32_t s_ng_node_ue_xnap_id;
  uint32_t t_ng_node_ue_xnap_id;
  // PDU Session Resource Admitted List
  xnap_pdusession_admitted_list_t pdusession_admitted_list;
  // Target to Source Transparent Container
  byte_array_t target2source;
} xnap_handover_req_ack_t;

typedef struct xnap_handover_preparation_failure_s {
  uint64_t ng_node_ue_xnap_id;
  xnap_cause_t cause;
  xnap_ngran_cgi_t target_cgi;
} xnap_handover_preparation_failure_t;

/* Xn code for SN Status transfer procedures */
// Indicates PDCP SN length
typedef enum { XNAP_SN_LENGTH_12 = 0, XNAP_SN_LENGTH_18 = 1 } xnap_sn_length_t;
        
typedef struct {
  // PDCP Sequence Number
  uint32_t pdcp_sn;
  // Hyper Frame Number 
  uint32_t hfn;
  // SN length
  xnap_sn_length_t sn_len;
} xnap_drb_count_value_t;

// DRBs Subject to Status Transfer Item
typedef struct {
  // DRB ID
  uint8_t drb_id;
  // UL COUNT value
  xnap_drb_count_value_t ul_count;
  // DL COUNT value
  xnap_drb_count_value_t dl_count;
} xnap_drb_status_t;

typedef struct {
  // Number of DRBs in the list
  uint8_t nb_drb;
  // DRB Status List
  xnap_drb_status_t drb_status_list[MAX_DRBS_PER_UE];
} xnap_ran_status_container_t;

typedef struct xnap_sn_status_transfer_s {
  // Source UE XNAP ID (M)
  uint32_t s_ng_node_ue_xnap_id;
  // Target UE XNAP ID (M)
  uint32_t t_ng_node_ue_xnap_id;
  // DRBs Subject To Status Transfer List (M)
  xnap_ran_status_container_t ran_status;
} xnap_sn_status_transfer_t;

/* Xn code for SN status transfer procedures */

#endif /* XNAP_MESSAGES_TYPES_H_ */
