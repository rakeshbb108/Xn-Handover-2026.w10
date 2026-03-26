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

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include<time.h>
#include <libconfig.h>
#include <sys/mman.h>
#include <fcntl.h>

#define TELNETSERVERCODE
#include "telnetsrv.h"

#include "openair2/RRC/NR/nr_rrc_defs.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.c"
#include "openair2/LAYER2/NR_MAC_UE/mac_defs.h"
#include "openair2/GNB_APP/gnb_config.h"
#include "common/ngran_types.h"
#include "openair2/E1AP/e1ap_common.h"
#include "radio/fhi_72/mplane/subscribe-mplane.h"
#include "openair1/PHY/defs_gNB.h"

#define ERROR_MSG_RET(mSG, aRGS...) do { prnt("FAILURE: " mSG, ##aRGS); return 1; } while (0)

#define ISINITBWP "bwp3gpp:isInitialBwp"
//#define CYCLPREF  "bwp3gpp:cyclicPrefix"
#define NUMRBS    "bwp3gpp:numberOfRBs"
#define STARTRB   "bwp3gpp:startRB"
#define BWPSCS    "bwp3gpp:subCarrierSpacing"

#define SSBFREQ "nrcelldu3gpp:ssbFrequency"
#define ARFCNDL "nrcelldu3gpp:arfcnDL"
#define BWDL    "nrcelldu3gpp:bSChannelBwDL"
#define ARFCNUL "nrcelldu3gpp:arfcnUL"
#define BWUL    "nrcelldu3gpp:bSChannelBwUL"
#define PCI     "nrcelldu3gpp:nRPCI"
#define TAC     "nrcelldu3gpp:nRTAC"
#define MCC     "nrcelldu3gpp:mcc"
#define MNC     "nrcelldu3gpp:mnc"
#define SD      "nrcelldu3gpp:sd"
#define SST     "nrcelldu3gpp:sst"
#define SSB_PERIOD     "nrcelldu3gpp:ssb_period"
#define SSB_OFFSET     "nrcelldu3gpp:ssb_offset"

#define gNBId             "nrgnbcuup3gpp:gNBId"
#define CU_UP_ID          "nrgnbcuup3gpp:cu_up_id"
#define CUUP_MCC          "nrgnbcuup3gpp:mcc"
#define CUUP_MNC          "nrgnbcuup3gpp:mnc"
#define MNC_DIGIT_LENGTH  "nrgnbcuup3gpp:mnc_length"

#define PMAX "nrfreqrel3gpp:pMax"
#define CELLLOCALID "nrcellcu3gpp:cellLocalId"
#define CU_MCC    "nrcellcu3gpp:mcc"
#define CU_MNC "nrcellcu3gpp:mnc"
#define CU_SST    "nrcellcu3gpp:sst"
#define CU_SD "nrcellcu3gpp:sd"
#define NR_FREQ_REF "nrcellcu3gpp:nRFrequencyRef"
int flag = 1;
int node_id = 0;
int pMax = 0;
char nRFrequencyRef[50];
extern uint32_t mplane_enabled;

//extern ru_global_metrics_t ru_global_metrics;

extern uint32_t mplane_enabled;

extern ru_global_metrics_t ru_global_metrics;
typedef struct b {
  long int dl;
  long int ul;
  long int tot_ul_prbs;
  long int tot_dl_prbs;
  long int rx_pdcp_sdu_pkts;
  long int tx_pdcp_sdu_pkts;
} b_t;

typedef struct ue_stat {
  rnti_t rnti;
  b_t thr;
  long int dl_errors;
  long int ul_errors;
  long int dl_ul_tbs;
} ue_stat_t;

#define PRINTLIST_i(len, fmt, ...) \
  { \
    for (int i = 0; i < len; ++i) { \
      if (i != 0) prnt(", "); \
      prnt(fmt, __VA_ARGS__); \
    } \
  } \

static int get_stats(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (buf)
    ERROR_MSG_RET("no parameter allowed\n");

  static ngran_node_t node_type;
  node_type = node_type?node_type:get_node_type();

  if(node_type == ngran_gNB_CUUP){
    const instance_t e1_inst = 0;
    const e1ap_upcp_inst_t *e1inst = getCxtE1(e1_inst);

    prnt("{\n");
    prnt("  \"o1-config\": {\n");
    prnt("     \"GNBCUUP\": {\n");
    prnt("      \""gNBId"\": %d,\n", e1inst->gnb_id);
    prnt("      \""CU_UP_ID"\": %d,\n", e1inst->cuup.setupReq.gNB_cu_up_id);
    prnt("      \""CUUP_MCC"\": \"%03d\",\n", e1inst->cuup.setupReq.plmn[0].id.mcc);
    prnt("      \""CUUP_MNC"\": \"%02d\",\n", e1inst->cuup.setupReq.plmn[0].id.mnc);
    prnt("      \""MNC_DIGIT_LENGTH"\": %d,\n", e1inst->cuup.setupReq.plmn[0].id.mnc_digit_length);
    prnt("      \"vendor\": \"OpenAirInterface\"\n");
    prnt("  }\n");
    prnt(" }\n");
    prnt("}\n");

  }
 if(node_type == ngran_gNB_CUCP || node_type == ngran_gNB_DU || node_type == ngran_gNB){
   const gNB_RRC_INST *rrc = RC.nrrrc[0];
   const nr_rrc_config_t cu = rrc->configuration;

   if(node_type == ngran_gNB_CUCP){
    if(flag == 1){
    node_id = rrc->node_id;
    strcpy(nRFrequencyRef, "AName=JohnDoe"); // Sreeram: should understand this.! Let's discuss this later. 
    flag = 0;
    }

    int rrc_setup_success_rate = 0;
    int xn_ho_success_rate = 0;
    if(rrc->rrc_setup != 0)
      rrc_setup_success_rate = (rrc->rrc_setup_complete / rrc->rrc_setup) * 100;
    if(rrc->xn_handover_triggered != 0)
      xn_ho_success_rate = (rrc->xn_handover_success / rrc->xn_handover_triggered) * 100;
    prnt("{\n");
    prnt("  \"o1-config\": {\n");
    prnt("     \"NRFREQREL\": {\n");
    prnt("      \""PMAX"\": %d\n", pMax);
    prnt("     },\n");
    prnt("     \"NRCELLCU\": {\n");
    prnt("      \""CELLLOCALID"\": %d,\n", node_id);
    prnt("      \""CU_MCC"\": \"%03d\",\n", cu.plmn[0].mcc);
    prnt("      \""CU_MNC"\": \"%02d\",\n", cu.plmn[0].mnc);
    //prnt("      \""CU_SST"\": %d,\n",nssai->sst);
    //prnt("      \""CU_SD"\": %d,\n",nssai->sd);
    prnt("      \""CU_SST"\": %d,\n", 1); // Sreeram: This is hardcoded should be changed to proper values.
    prnt("      \""CU_SD"\": %d,\n", 0); // Sreeram: This is hardcoded should be changed to proper values.
    prnt("      \""NR_FREQ_REF"\": \"%s\"\n", nRFrequencyRef);
    prnt("     },\n");
    prnt("    \"device\": {\n");
    prnt("      \"gNBId\": %d,\n",node_id);
    prnt("      \"gnbCUName\": \"%s\"\n", rrc->node_name);
    prnt("    }\n");
    prnt("  },\n");
    prnt("  \"O1-Operational\": {\n");
    prnt("    \"NUM_DUS\": %d,\n",rrc->num_dus);
    prnt("    \"rrc_setup\": %d,\n",rrc->rrc_setup);
    prnt("    \"rrc_setup_complete\": %d,\n",rrc->rrc_setup_complete);
    prnt("    \"rrc_setup_success_rate\": %d,\n",rrc_setup_success_rate);
    prnt("    \"Max_XN_HO\": %d,\n",rrc->xn_handover_success);
    prnt("    \"XN_HO_Success_rate\": %d,\n",xn_ho_success_rate);
    prnt("    \"XN_HO_Latency\": %d,\n",rrc->ho_xn_latency);
    prnt("    \"NUM_CUUPS\": %d,\n",rrc->num_cuups);
    prnt("    \"vendor\": \"OpenAirInterface\"\n");
    prnt("  }\n");
    prnt("}\n");

  }

  if(node_type == ngran_gNB_DU || node_type == ngran_gNB){
  const gNB_MAC_INST *mac = RC.nrmac[0];
  PHY_VARS_gNB *gNB = RC.gNB[0];
  float ra_success_rate = 0;
  int ra_successful = mac->ra_successful;
  int ra_procedure = gNB->ra_procedure;
  if(ra_procedure != 0)
    ra_success_rate = (ra_successful / ra_procedure) * 100;
  AssertFatal(mac != NULL, "need MAC\n");
  NR_SCHED_LOCK(&mac->sched_lock);

  const f1ap_setup_req_t *sr = mac->f1_config.setup_req;
  const f1ap_served_cell_info_t *cell_info = &sr->cell[0].info;

  const NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;
  const NR_FrequencyInfoDL_t *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
  const NR_FrequencyInfoUL_t *frequencyInfoUL = scc->uplinkConfigCommon->frequencyInfoUL;
  frame_type_t frame_type = get_frame_type(*frequencyInfoDL->frequencyBandList.list.array[0], *scc->ssbSubcarrierSpacing);
  const NR_BWP_t *initialDL = &scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters;
  const NR_BWP_t *initialUL = &scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;

  const NR_TDD_UL_DL_Pattern_t *tdd = scc->tdd_UL_DL_ConfigurationCommon ? &scc->tdd_UL_DL_ConfigurationCommon->pattern1 : NULL;

  int scs = initialDL->subcarrierSpacing;
  AssertFatal(scs == initialUL->subcarrierSpacing, "different SCS for UL/DL not supported!\n");
  int band = *frequencyInfoDL->frequencyBandList.list.array[0];
  int nrb = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
  AssertFatal(nrb == frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth, "different BW for UL/DL not supported!\n");
  frequency_range_t fr = band > 256 ? FR2 : FR1;
  int bw_index = get_supported_band_index(scs, fr, nrb);
  int bw_mhz = get_supported_bw_mhz(fr, bw_index);

  const mac_stats_t *stat = &mac->mac_stats;
  static mac_stats_t last = {0};
  int diff_used = stat->used_prb_aggregate - last.used_prb_aggregate;
  int diff_total = stat->total_prb_aggregate - last.total_prb_aggregate;
  int load = diff_total > 0 ? 100 * diff_used / diff_total : 0;
  last = *stat;

  static struct timespec tp_last = {0};
  struct timespec tp_now;
  clock_gettime(CLOCK_MONOTONIC, &tp_now);
  size_t diff_msec = (tp_now.tv_sec - tp_last.tv_sec) * 1000 + (tp_now.tv_nsec - tp_last.tv_nsec) / 1000000;
  tp_last = tp_now;

  const int srb_flag = 0;
  const int rb_id = 1;
  static b_t last_total[MAX_MOBILES_PER_GNB] = {0}; // TODO: hash table?
  ue_stat_t ue_stat[MAX_MOBILES_PER_GNB] = {0};
  int num_ues = 0;

  UE_iterator((NR_UE_info_t **)mac->UE_info.connected_ue_list, it) {
    nr_rlc_statistics_t rlc = {0};
    nr_rlc_get_statistics(it->rnti, srb_flag, rb_id, &rlc);
    b_t *lt = &last_total[num_ues];
    ue_stat_t *ue_s = &ue_stat[num_ues];
    ue_s->rnti = it->rnti;

    NR_UE_sched_ctrl_t sched_ctrl = it->UE_sched_ctrl;
    ue_s->dl_ul_tbs = sched_ctrl.num_total_bytes / 8;

    NR_mac_stats_t *stats = &it->mac_stats;
    ue_s->dl_errors = stats->dl.errors;
    ue_s->ul_errors = stats->ul.errors;

    uint32_t rx_pdcp_sdu_pkts = 0;
    uint32_t tx_pdcp_sdu_pkts = 0;

    for(int rb_id = 1; rb_id < 6; ++rb_id ){

      nr_rlc_statistics_t rb_rlc = {0};
      const int srb_flag = 0;
      const bool rc = nr_rlc_get_statistics(it->rnti, srb_flag, rb_id, &rb_rlc);

      rx_pdcp_sdu_pkts += rb_rlc.rxsdu_pkts;
      tx_pdcp_sdu_pkts += rb_rlc.txsdu_pkts;
    }

    // static var last_total: we might have old data, larger than what
    // reports RLC, leading to a huge number -> cut off to zero
    if (lt->dl > rlc.txpdu_bytes)
      lt->dl = rlc.txpdu_bytes;
    if (lt->ul > rlc.rxpdu_bytes)
      lt->ul = rlc.rxpdu_bytes;
    if(lt->tot_ul_prbs > stats->ul.total_rbs)
      lt->tot_ul_prbs = stats->ul.total_rbs;
    if(lt->tot_dl_prbs > stats->dl.total_rbs)
      lt->tot_dl_prbs = stats->dl.total_rbs;
    if(lt->rx_pdcp_sdu_pkts > rx_pdcp_sdu_pkts)
      lt->rx_pdcp_sdu_pkts = rx_pdcp_sdu_pkts;
    if(lt->tx_pdcp_sdu_pkts > tx_pdcp_sdu_pkts)
      lt->tx_pdcp_sdu_pkts = tx_pdcp_sdu_pkts;

    ue_s->thr.tot_ul_prbs = (stats->ul.total_rbs - lt->tot_ul_prbs);
    ue_s->thr.tot_dl_prbs = (stats->dl.total_rbs - lt->tot_dl_prbs);
    ue_s->thr.rx_pdcp_sdu_pkts = rx_pdcp_sdu_pkts - lt->rx_pdcp_sdu_pkts;
    ue_s->thr.tx_pdcp_sdu_pkts = tx_pdcp_sdu_pkts - lt->tx_pdcp_sdu_pkts;
    ue_s->thr.dl = (rlc.txpdu_bytes - lt->dl) * 8 / diff_msec;
    ue_s->thr.ul = (rlc.rxpdu_bytes - lt->ul) * 8 / diff_msec;

    lt->rx_pdcp_sdu_pkts = rx_pdcp_sdu_pkts;
    lt->tx_pdcp_sdu_pkts = tx_pdcp_sdu_pkts;
    lt->tot_dl_prbs = stats->dl.total_rbs;
    lt->tot_ul_prbs = stats->ul.total_rbs;
    lt->dl = rlc.txpdu_bytes;
    lt->ul = rlc.rxpdu_bytes;
    num_ues++;
  }

  prnt("{\n");
    prnt("  \"o1-config\": {\n");
    if(node_type == ngran_gNB){
    prnt("     \"NRFREQREL\": {\n");
    prnt("      \""PMAX"\": %d\n", pMax);
    prnt("     },\n");
    prnt("     \"NRCELLCU\": {\n");
    prnt("      \""CELLLOCALID"\": %d,\n", node_id);
    prnt("      \""CU_MCC"\": \"%03d\",\n", cu.plmn[0].mcc);
    prnt("      \""CU_MNC"\": \"%02d\",\n", cu.plmn[0].mnc);
    prnt("      \""CU_SST"\": %d,\n", 1); // Sreeram: This is hardcoded should be changed to proper values. 
    prnt("      \""CU_SD"\": %d,\n", 0); // Sreeram: This is hardcoded should be changed to proper values. 
    prnt("      \""NR_FREQ_REF"\": \"%s\"\n", nRFrequencyRef);
    prnt("     },\n");
    }
    prnt("    \"BWP\": {\n");
    prnt("      \"dl\": [{\n");
    prnt("        \"" ISINITBWP "\": true,\n");
    prnt("        \"" NUMRBS "\": %ld,\n", NRRIV2BW(initialDL->locationAndBandwidth, MAX_BWP_SIZE));
    prnt("        \"" STARTRB "\": %ld,\n", NRRIV2PRBOFFSET(initialDL->locationAndBandwidth, MAX_BWP_SIZE));
    prnt("        \"" BWPSCS "\": %ld\n", 15 * (1U << scs));
    prnt("      }],\n");
    prnt("      \"ul\": [{\n");
    prnt("        \"" ISINITBWP "\": true,\n");
    prnt("        \"" NUMRBS "\": %ld,\n", NRRIV2BW(initialUL->locationAndBandwidth, MAX_BWP_SIZE));
    prnt("        \"" STARTRB "\": %ld,\n", NRRIV2PRBOFFSET(initialUL->locationAndBandwidth, MAX_BWP_SIZE));
    prnt("        \"" BWPSCS "\": %ld\n", 15 * (1U << scs));
    prnt("      }]\n");
    prnt("    },\n");

    prnt("    \"NRCELLDU\": {\n");
    prnt("      \"" SSBFREQ "\": %ld,\n", *scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB);
    prnt("      \"" ARFCNDL "\": %ld,\n", frequencyInfoDL->absoluteFrequencyPointA);
    prnt("      \"" BWDL "\": %ld,\n", bw_mhz);
    prnt("      \"" ARFCNUL "\": %ld,\n", frequencyInfoUL->absoluteFrequencyPointA ? *frequencyInfoUL->absoluteFrequencyPointA : frequencyInfoDL->absoluteFrequencyPointA);
    prnt("      \"" BWUL "\": %ld,\n", bw_mhz);
    prnt("      \"" PCI "\": %ld,\n", *scc->physCellId);
    prnt("      \"" TAC "\": %ld,\n", *cell_info->tac);
    prnt("      \"" MCC "\": \"%03d\",\n", cell_info->plmn.mcc);
    prnt("      \"" MNC "\": \"%0*d\",\n", cell_info->plmn.mnc_digit_length, cell_info->plmn.mnc);
    prnt("      \"" SD  "\": %d,\n", cell_info->nssai[0].sd);
    prnt("      \"" SST "\": %d,\n", cell_info->nssai[0].sst);
    prnt("      \"DL_UL_TransmissionPeriodicity\": %d,\n", tdd->dl_UL_TransmissionPeriodicity);
    prnt("      \"nrofDownlinkSlots\": %d,\n", tdd->nrofDownlinkSlots);
    prnt("      \"nrofDownlinkSymbols\": %d,\n", tdd->nrofDownlinkSymbols);
    prnt("      \"nrofUplinkSlots\": %d,\n", tdd->nrofUplinkSlots);
    prnt("      \"nrofUplinkSymbols\": %d\n", tdd->nrofUplinkSymbols);
    prnt("    },\n");
    if(mplane_enabled){
      ru_mplane_config_t *oru = &RC.ru[0]->openair0_cfg.split7.ru_sessions.ru_session[0].ru_mplane_config; // hardcoding to send only one RU related data.
      ru_mplane_metrics_t *oru_metrics = &RC.ru[0]->openair0_cfg.split7.ru_sessions.ru_session[0].ru_mplane_metrics; // hardcoding to send only one RU related data.

      prnt("  \"o-ru-stats\": {\n");
      prnt("    \"o-ran-uplane-conf\": [\n");
      prnt("      {\n");
      prnt("        \"tx-array-carrier:absolute-frequency-center\": %d,\n", oru->tx_array_carrier.arfcn_center);
      prnt("        \"tx-array-carrier:center-of-channel-bandwidth\": %ld,\n", oru->tx_array_carrier.center_channel_bw);
      prnt("        \"tx-array-carrier:channel-bandwidth\": %d,\n", oru->tx_array_carrier.channel_bw);
      prnt("        \"tx-array-carrier:active\": \"%s\",\n", oru->tx_array_carrier.ru_carrier);
      prnt("        \"tx-array-carrier:rw-duplex-scheme\": \"%s\",\n", oru->tx_array_carrier.rw_duplex_scheme);
      prnt("        \"tx-array-carrier:rw-type\": \"%s\",\n", oru->tx_array_carrier.rw_type);
      prnt("        \"tx-array-carrier:gain\": %.2f,\n", oru->tx_array_carrier.gain);
      prnt("        \"tx-array-carrier:downlink-radio-frame-offset\": %d,\n", oru->tx_array_carrier.dl_radio_frame_offset);
      prnt("        \"tx-array-carrier:downlink-sfn-offset\": %d\n", oru->tx_array_carrier.dl_sfn_offset);
      prnt("      },\n");
      prnt("      {\n");
      prnt("        \"rx-array-carrier:absolute-frequency-center\": %d,\n", oru->tx_array_carrier.arfcn_center);
      prnt("        \"rx-array-carrier:center-of-channel-bandwidth\": %ld,\n", oru->tx_array_carrier.center_channel_bw);
      prnt("        \"rx-array-carrier:channel-bandwidth\": %d,\n", oru->tx_array_carrier.channel_bw);
      prnt("        \"rx-array-carrier:active\": \"%s\",\n", oru->tx_array_carrier.ru_carrier);
      prnt("        \"rx-array-carrier:downlink-radio-frame-offset\": %d,\n", oru->tx_array_carrier.dl_radio_frame_offset);
      prnt("        \"rx-array-carrier:downlink-sfn-offset\": %d,\n", oru->tx_array_carrier.dl_sfn_offset);
      prnt("        \"rx-array-carrier:gain-correction\": %.2f,\n", oru->tx_array_carrier.gain_correction);
      prnt("        \"rx-array-carrier:n-ta-offset\": %d\n", oru->tx_array_carrier.n_ta_offset);
      prnt("      }\n");
      prnt("    ],\n");
      prnt("    \"delay-management\": {\n");
      prnt("      \"ru-delay-profile:t2a-min-up\": %d,\n", oru->delay.T2a_min_up);
      prnt("      \"ru-delay-profile:t2a-max-up\": %d,\n", oru->delay.T2a_max_up);
      prnt("      \"ru-delay-profile:t2a-min-cp-dl\": %d,\n", oru->delay.T2a_min_cp_dl);
      prnt("      \"ru-delay-profile:t2a-max-cp-dl\": %d,\n", oru->delay.T2a_max_cp_dl);
      prnt("      \"ru-delay-profile:tcp-adv-dl\": %d,\n", oru->delay.Tcp_adv_dl);
      prnt("      \"ru-delay-profile:ta3-min\": %d,\n", oru->delay.Ta3_min);
      prnt("      \"ru-delay-profile:ta3-max\": %d,\n", oru->delay.Ta3_max);
      prnt("      \"ru-delay-profile:t2a-min-cp-ul\": %d,\n", oru->delay.T2a_min_cp_ul);
      prnt("      \"ru-delay-profile:t2a-max-cp-ul\": %d\n", oru->delay.T2a_max_cp_ul);
      prnt("    },\n");

      // fill dummy metrics
      ru_session_t *ru_session = &RC.ru[0]->openair0_cfg.split7.ru_sessions.ru_session[0];
      ru_session->ru_mplane_metrics.total_rx_good_pkt_cnt = ru_global_metrics.total_rx_good_pkt_cnt;
      ru_session->ru_mplane_metrics.total_rx_bit_rate =  ru_global_metrics.total_rx_bit_rate;
      ru_session->ru_mplane_metrics.oran_rx_on_time = ru_global_metrics.oran_rx_on_time;
      ru_session->ru_mplane_metrics.oran_rx_early =  ru_global_metrics.oran_rx_early;
      ru_session->ru_mplane_metrics.oran_rx_late = ru_global_metrics.oran_rx_late;
      ru_session->ru_mplane_metrics.oran_rx_corrupt =  ru_global_metrics.oran_rx_corrupt;
      ru_session->ru_mplane_metrics.oran_rx_total = ru_global_metrics.oran_rx_total;
      ru_session->ru_mplane_metrics.oran_rx_total_c = ru_global_metrics.oran_rx_total_c;
      ru_session->ru_mplane_metrics.oran_rx_on_time_c = ru_global_metrics.oran_rx_on_time_c;
      ru_session->ru_mplane_metrics.oran_rx_early_c = ru_global_metrics.oran_rx_early_c;
      ru_session->ru_mplane_metrics.oran_rx_late_c = ru_global_metrics.oran_rx_late_c;
      ru_session->ru_mplane_metrics.oran_rx_error_drop = ru_global_metrics.oran_rx_error_drop;
      ru_session->ru_mplane_metrics.oran_tx_total = ru_global_metrics.oran_tx_total;
      ru_session->ru_mplane_metrics.oran_tx_total_c = ru_global_metrics.oran_tx_total_c;
      // end dummy fill

      prnt("    \"o-ran-performance-counters\": {\n");
      prnt("      \"performance-counters:total-rx-good-pkt-cnt\": %d,\n", oru_metrics->total_rx_good_pkt_cnt);
      prnt("      \"performance-counters:total-rx-bit-rate\": %d,\n", oru_metrics->total_rx_bit_rate);
      prnt("      \"performance-counters:oran-rx-on-time\": %d,\n", oru_metrics->oran_rx_on_time);
      prnt("      \"performance-counters:oran-rx-early\": %d,\n", oru_metrics->oran_rx_early);
      prnt("      \"performance-counters:oran-rx-late\": %d,\n", oru_metrics->oran_rx_late);
      prnt("      \"performance-counters:oran-rx-corrupt\": %d,\n", oru_metrics->oran_rx_corrupt);
      prnt("      \"performance-counters:oran-rx-total\": %d,\n", oru_metrics->oran_rx_total);
      prnt("      \"performance-counters:oran-rx-total-c\": %d,\n", oru_metrics->oran_rx_total_c);
      prnt("      \"performance-counters:oran-rx-on-time-c\": %d,\n", oru_metrics->oran_rx_on_time_c);
      prnt("      \"performance-counters:oran-rx-early-c\": %d,\n", oru_metrics->oran_rx_early_c);
      prnt("      \"performance-counters:oran-rx-late-c\": %d,\n", oru_metrics->oran_rx_late_c);
      prnt("      \"performance-counters:oran-rx-error-drop\": %d,\n", oru_metrics->oran_rx_error_drop);
      prnt("      \"performance-counters:oran-tx-total\": %d,\n", oru_metrics->oran_tx_total);
      prnt("      \"performance-counters:oran-tx-total-c\": %d\n", oru_metrics->oran_tx_total_c);
      prnt("    }\n");
      prnt("  },\n");

    }
    prnt("    \"device\": {\n");
    prnt("      \"gnb_DU_Id\": %d,\n", sr->gNB_DU_id);
    prnt("      \"gnbId\": %d,\n", sr->gNB_DU_id); 
    prnt("      \"gnbName\": \"%s\",\n", sr->gNB_DU_name); 
    if(node_type == ngran_gNB) prnt("      \"gnbCUName\": \"%s\",\n", rrc->node_name);
    prnt("      \"vendor\": \"OpenAirInterface\"\n");
    prnt("    }\n");
    prnt("  },\n");
    prnt("  \"O1-Operational\": {\n");
    if(node_type == ngran_gNB){
      prnt("    \"NUM_DUS\": %d,\n",rrc->num_dus);
      prnt("    \"NUM_CUUPS\": %d,\n",rrc->num_cuups);
      prnt("      \"vendor\": \"OpenAirInterface\",\n");
    }
    prnt("    \"frame-type\": \"%s\",\n", frame_type == TDD ? "tdd" : "fdd");
    prnt("    \"band-number\": %ld,\n", band);
    prnt("    \"ra_procedure\": %d,\n", ra_procedure);
    prnt("    \"ra_successful\": %d,\n", ra_successful);
    prnt("    \"ra_success_rate\": %f,\n", ra_success_rate);
    prnt("    \"num-ues\": %d,\n", num_ues);
    prnt("    \"ues\": ["); PRINTLIST_i(num_ues, "%d", ue_stat[i].rnti); prnt("],\n");
    prnt("    \"load\": %d,\n", load);
    prnt("    \"ues-thp\": [");
    PRINTLIST_i(num_ues, "\n      {\"rnti\": %d, \"dl\": %ld, \"ul\": %ld, \"dl_ul_tbs\": %d, \"err_dl_tbs\": %d, \"err_ul_tbs\": %d, \"tot_dl_prbs\": %d, \"tot_ul_prbs\": %d, \"ul_pdcp_sdu\": %d, \"dl_pdcp_sdu\": %d}", ue_stat[i].rnti, ue_stat[i].thr.dl, ue_stat[i].thr.ul, ue_stat[i].dl_ul_tbs, ue_stat[i].dl_errors, ue_stat[i].ul_errors, ue_stat[i].thr.tot_dl_prbs, ue_stat[i].thr.tot_ul_prbs, ue_stat[i].thr.tx_pdcp_sdu_pkts, ue_stat[i].thr.rx_pdcp_sdu_pkts);
    prnt("\n    ]\n");
    prnt("  }\n");
    prnt("}\n");
    prnt("OK\n");
    NR_SCHED_UNLOCK(&mac->sched_lock);
  }
 }
  return 0;
}

static int read_long(const char *buf, const char *end, const char *id, long *val)
{
  const char *curr = buf;
  while (isspace(*curr) && curr < end) // skip leading spaces
    curr++;
  int len = strlen(id);
  if (curr + len >= end)
    return -1;
  if (strncmp(curr, id, len) != 0) // check buf has id
    return -1;
  curr += len;
  while (isspace(*curr) && curr < end) // skip middle spaces
    curr++;
  if (curr >= end)
    return -1;
  int nread = sscanf(curr, "%ld", val);
  if (nread != 1)
    return -1;
  while (isdigit(*curr) && curr < end) // skip all digits read above
    curr++;
  if (curr > end)
    return -1;
  return curr - buf;
}

bool running = true; // in the beginning, the softmodem is started automatically
static int set_config(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!buf)
    ERROR_MSG_RET("need param: o1 config param1 val1 [param2 val2 ...]\n");
  if (running)
    ERROR_MSG_RET("cannot set parameters while L1 is running\n");
  const char *end = buf + strlen(buf);

  /* we need to update the following fields to change frequency and/or
   * bandwidth:
   * --gNBs.[0].servingCellConfigCommon.[0].absoluteFrequencySSB 620736            -> SSBFREQ
   * --gNBs.[0].servingCellConfigCommon.[0].dl_absoluteFrequencyPointA 620020      -> ARFCNDL
   * --gNBs.[0].servingCellConfigCommon.[0].dl_carrierBandwidth 51                 -> BWDL
   * --gNBs.[0].servingCellConfigCommon.[0].initialDLBWPlocationAndBandwidth 13750 -> NUMRBS + STARTRB
   * --gNBs.[0].servingCellConfigCommon.[0].ul_carrierBandwidth 51                 -> BWUL?
   * --gNBs.[0].servingCellConfigCommon.[0].initialULBWPlocationAndBandwidth 13750 -> ?
   */

  int processed = 0;
  int pos = 0;

  long ssbfreq;
  processed = read_long(buf + pos, end, SSBFREQ, &ssbfreq);
  if (processed < 0)
    ERROR_MSG_RET("could not read " SSBFREQ " at index %d\n", pos + processed);
  pos += processed;
  prnt("setting " SSBFREQ ":   %ld [len %d]\n", ssbfreq, pos);

  long arfcn;
  processed = read_long(buf + pos, end, ARFCNDL, &arfcn);
  if (processed < 0)
    ERROR_MSG_RET("could not read " ARFCNDL " at index %d\n", pos + processed);
  pos += processed;
  prnt("setting " ARFCNDL ":        %ld [len %d]\n", arfcn, pos);

  long bwdl;
  processed = read_long(buf + pos, end, BWDL, &bwdl);
  if (processed < 0)
    ERROR_MSG_RET("could not read " BWDL " at index %d\n", pos + processed);
  pos += processed;
  prnt("setting " BWDL ":  %ld [len %d]\n", bwdl, pos);

  long numrbs;
  processed = read_long(buf + pos, end, NUMRBS, &numrbs);
  if (processed < 0)
    ERROR_MSG_RET("could not read " NUMRBS " at index %d\n", pos + processed);
  pos += processed;
  prnt("setting " NUMRBS ":         %ld [len %d]\n", numrbs, pos);

  long startrb;
  processed = read_long(buf + pos, end, STARTRB, &startrb);
  if (processed < 0)
    ERROR_MSG_RET("could not read " STARTRB " at index %d\n", pos + processed);
  pos += processed;
  prnt("setting " STARTRB ":             %ld [len %d]\n", startrb, pos);

  int locationAndBandwidth = PRBalloc_to_locationandbandwidth0(numrbs, startrb, MAX_BWP_SIZE);
  prnt("inferred locationAndBandwidth:       %d\n", locationAndBandwidth);
  prnt("OK\n");
  return 0;
}

// Should change this function to apply the changes to the UE as Well
// To be done !! Safwan Sreeram. 
static int set_cu_config(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!buf)
    ERROR_MSG_RET("need param: o1 config param1 val1 [param2 val2 ...]\n");
  if (running)
    ERROR_MSG_RET("cannot set parameters while L1 is running\n");
  const char *end = buf + strlen(buf);
  int processed = 0;
  int pos = 0;
  long pMax;

  processed = read_long(buf + pos, end, PMAX, &pMax);
  if (processed < 0)
    ERROR_MSG_RET("could not read " PMAX " at index %d\n", pos + processed);
  pos += processed;

  const gNB_MAC_INST *mac = RC.nrmac[0];
  const NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;
  *scc->uplinkConfigCommon->frequencyInfoUL->p_Max = pMax;

  prnt("OK\n");
  return 0;
}

static int config_ru(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!buf)
    ERROR_MSG_RET("need param: o1 config_ru <parameter>\n");

//  ru_global_metrics.ru_reset = 1;
//  printf("ru_global_metrics.ru_reset = %d\n", ru_global_metrics.ru_reset);
  printf("Resetting RU .... RU will go down\n");
  return 0;
}

static int set_bwconfig(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (running)
    ERROR_MSG_RET("cannot set parameters while L1 is running\n");
  if (!buf)
    ERROR_MSG_RET("need param: o1 bwconfig <BW>\n");

  char *end = NULL;
  if (NULL != (end = strchr(buf, '\n')))
    *end = 0;
  if (NULL != (end = strchr(buf, '\r')))
    *end = 0;

  gNB_MAC_INST *mac = RC.nrmac[0];
  NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;
  NR_FrequencyInfoDL_t *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
  NR_BWP_t *initialDL = &scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters;
  NR_FrequencyInfoUL_t *frequencyInfoUL = scc->uplinkConfigCommon->frequencyInfoUL;
  NR_BWP_t *initialUL = &scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;
  if (strcmp(buf, "40") == 0) {
    *scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB = 641280;
    frequencyInfoDL->absoluteFrequencyPointA = 640008;
    AssertFatal(frequencyInfoUL->absoluteFrequencyPointA == NULL, "only handle TDD\n");
    frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth = 106;
    initialDL->locationAndBandwidth = 28875;
    frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth = 106;
    initialUL->locationAndBandwidth = 28875;
    get_softmodem_params()->threequarter_fs = 1;
  } else if (strcmp(buf, "20") == 0) {
    *scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB = 641280;
    frequencyInfoDL->absoluteFrequencyPointA = 640596;
    AssertFatal(frequencyInfoUL->absoluteFrequencyPointA == NULL, "only handle TDD\n");
    frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth = 51;
    initialDL->locationAndBandwidth = 13750;
    frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth = 51;
    initialUL->locationAndBandwidth = 13750;
    get_softmodem_params()->threequarter_fs = 0;
  } else if (strcmp(buf, "100") == 0) {
    *scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB = 646668;
    frequencyInfoDL->absoluteFrequencyPointA = 643392;
    AssertFatal(frequencyInfoUL->absoluteFrequencyPointA == NULL, "only handle TDD\n");
    frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth = 273;
    initialDL->locationAndBandwidth = 1099;
    frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth = 273;
    initialUL->locationAndBandwidth = 1099;
    get_softmodem_params()->threequarter_fs = 0;
  } else if (strcmp(buf, "60") == 0) {
    *scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB = 621984;
    frequencyInfoDL->absoluteFrequencyPointA = 620040;
    AssertFatal(frequencyInfoUL->absoluteFrequencyPointA == NULL, "only handle TDD\n");
    frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth = 162;
    initialDL->locationAndBandwidth = 31624;
    frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth = 162;
    initialUL->locationAndBandwidth = 31624;
    get_softmodem_params()->threequarter_fs = 0;
  } else {
    ERROR_MSG_RET("unhandled option %s\n", buf);
  }

  free(RC.nrmac[0]->sched_ctrlSIB1);
  RC.nrmac[0]->sched_ctrlSIB1 = NULL;

  free_MIB_NR(mac->common_channels[0].mib);
  mac->common_channels[0].mib = get_new_MIB_NR(scc);

  const f1ap_served_cell_info_t *info = &mac->f1_config.setup_req->cell[0].info;
  nr_mac_configure_sib1(mac, &info->plmn, info->nr_cellid, *info->tac);

  prnt("OK\n");
  return 0;
}

extern int stop_L1(module_id_t gnb_id);

static int set_pMaxconfig(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!buf)
    ERROR_MSG_RET("need param: o1 pMax \n");

  const gNB_MAC_INST *mac = RC.nrmac[0];
  const NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;


  char *end = NULL;
  if (NULL != (end = strchr(buf, '\n')))
    *end = 0;

  if (NULL != (end = strchr(buf, '\r'))){
    printf("%s\n", buf);
    *end = 0;
  }
  if(atoi(buf) <= 30 && atoi(buf) >= -33)
    *scc->uplinkConfigCommon->frequencyInfoUL->p_Max = atoi(buf);

  else
     printf("unknown pMax value%s\n", buf);

  prnt("OK\n");

  return 0;

}

static int set_tdd_slot_config(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!buf)
    ERROR_MSG_RET("need param: o1 tdd_config \n");

  const gNB_MAC_INST *mac = RC.nrmac[0];
  const NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;

  char *end = NULL;
  if (NULL != (end = strchr(buf, '\n')))
    *end = 0;

  if (NULL != (end = strchr(buf, '\r'))){
    printf("%s\n", buf);
    *end = 0;
  }

  int dl_ul_trans_period = 0;
  int nr_of_downlink_slots = 0;
  int nr_of_uplink_slots = 0;
  
  char key[100];
  int value;
  
  char *token = strtok(buf, " ");
  while (token != NULL) {
      if (sscanf(token, "%d", &value) != 1) {
          strcpy(key, token);
          
          token = strtok(NULL, " ");
          
          if (token != NULL) {
              sscanf(token, "%d", &value);

              if (strcmp(key, "dl_ul_trans_period") == 0) {
                  dl_ul_trans_period = value;
              } else if (strcmp(key, "dl_slots") == 0) {
                  nr_of_downlink_slots = value;
              } else if (strcmp(key, "ul_slots") == 0) {
                  nr_of_uplink_slots = value;
              }
              
              printf("%s: %d\n", key, value);
          }
      }
      
      token = strtok(NULL, " ");
  }
  if(nr_of_downlink_slots < 5 && nr_of_uplink_slots < 5) 
    scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity = 5; 
  else
    scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity = 6;

  scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots = nr_of_downlink_slots;
  scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots = nr_of_uplink_slots;
  prnt("OK\n");

  return 0;

}

static int set_tdd_symbol_config(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!buf)
    ERROR_MSG_RET("need param: o1 tdd_config \n");

  const gNB_MAC_INST *mac = RC.nrmac[0];
  const NR_ServingCellConfigCommon_t *scc = mac->common_channels[0].ServingCellConfigCommon;

  char *end = NULL;
  if (NULL != (end = strchr(buf, '\n')))
    *end = 0;

  if (NULL != (end = strchr(buf, '\r'))){
    printf("%s\n", buf);
    *end = 0;
  }

  int nr_of_downlink_symbols = 0;
  int nr_of_uplink_symbols = 0;
  
  char key[100];
  int value;
  
  char *token = strtok(buf, " ");
  while (token != NULL) {
      if (sscanf(token, "%d", &value) != 1) {
          strcpy(key, token);
          
          token = strtok(NULL, " ");
          
          if (token != NULL) {
              sscanf(token, "%d", &value);

              if (strcmp(key, "dl_symbols") == 0) {
                  nr_of_downlink_symbols = value;
              } 
              else if (strcmp(key, "ul_symbols") == 0) {
                  nr_of_uplink_symbols = value;
              }
              
              printf("%s: %d\n", key, value);
          }
      }
      
      token = strtok(NULL, " ");
  }
 
  scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols = nr_of_downlink_symbols;
  scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols = nr_of_uplink_symbols;
  prnt("OK\n");

  return 0;
}

static int set_gNBId(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!buf)
    ERROR_MSG_RET("need param: o1 gNBId \n");

  char *end = NULL;
  if (NULL != (end = strchr(buf, '\n')))
    *end = 0;

  if (NULL != (end = strchr(buf, '\r'))){
    printf("%s\n", buf);
    *end = 0;
  }

  node_id = atoi(buf);
  printf("node_id is %d\n",node_id);
  prnt("OK\n");

  return 0;

}

static int set_nRFrequencyRef(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!buf)
    ERROR_MSG_RET("need param: o1 nRFrequencyRef \n");

  char *end = NULL;
  if (NULL != (end = strchr(buf, '\n')))
    *end = 0;

  if (NULL != (end = strchr(buf, '\r'))){
    printf("%s\n", buf);
    *end = 0;
  }

  // nRFrequencyRef = buf;
  strcpy(nRFrequencyRef, buf);
  printf(" nRFrequencyRef is %s\n", nRFrequencyRef);
  prnt("OK\n");

  return 0;

}

static int stop_modem(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!running)
    ERROR_MSG_RET("cannot stop, nr-softmodem not running\n");

  /* make UEs out of sync and wait 50ms to ensure no PUCCH is scheduled. After
   * a restart, the frame/slot numbers will be different, which "confuses" the
   * scheduler, which has many PUCCH structures filled with expected frame/slot
   * combinations that won't happen. */
  const gNB_MAC_INST *mac = RC.nrmac[0];
  UE_iterator((NR_UE_info_t **)mac->UE_info.connected_ue_list, it) {
    nr_mac_trigger_ul_failure(&it->UE_sched_ctrl, 1);
  }
  usleep(50000);

  stop_L1(0);
  running = false;
  prnt("OK\n");
  return 0;
}

extern int start_L1L2(module_id_t gnb_id);
static int start_modem(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (running)
    ERROR_MSG_RET("cannot start, nr-softmodem already running\n");
  start_L1L2(0);
  running = true;
  prnt("OK\n");
  return 0;
}

extern void du_clear_all_ue_states();
static int remove_mac_ues(char *buf, int debug, telnet_printfunc_t prnt)
{
  du_clear_all_ue_states();
  prnt("OK\n");
  return 0;
}

static telnetshell_cmddef_t o1cmds[] = {
  {"stats", "", get_stats},
  {"config", "[]", set_config},
  {"config_cu", "[]", set_cu_config},
  {"config_ru", "[]", config_ru}, // to be implemented
  {"bwconfig", "", set_bwconfig},
  {"pMax", "", set_pMaxconfig},
  {"gnbid", "", set_gNBId},
  {"tdd_slot_config", "", set_tdd_slot_config},
  {"tdd_symbol_config", "", set_tdd_symbol_config},
  {"nRFrequencyRef", "", set_nRFrequencyRef},
  {"stop_modem", "", stop_modem},
  {"start_modem", "", start_modem},
  {"remove_mac_ues", "", remove_mac_ues},
  {"", "", NULL},
};

static telnetshell_vardef_t o1vars[] = {
  {"", 0, 0, NULL}
};

void add_o1_cmds(void) {
  add_telnetcmd("o1", o1vars, o1cmds);
}
