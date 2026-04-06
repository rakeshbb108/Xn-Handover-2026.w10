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

/*
                                gnb_app.c
                             -------------------
  AUTHOR  : Laurent Winckel, Sebastien ROUX, Lionel GAUTHIER, Navid Nikaein, WEI-TAI CHEN
  COMPANY : EURECOM, NTUST
  EMAIL   : Lionel.Gauthier@eurecom.fr and Navid Nikaein, kroempa@gmail.com
*/

#include <string.h>
#include <stdio.h>
#include <nr_pdcp/nr_pdcp.h>
#include <softmodem-common.h>
#include <nr-softmodem.h>

#include "gnb_app.h"
#include "assertions.h"
#include "common/ran_context.h"

#include "common/utils/LOG/log.h"

#include "x2ap_eNB.h"
#include "intertask_interface.h"
#include "ngap_gNB.h"
#include "sctp_eNB_task.h"
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "PHY/INIT/phy_init.h" 
#include "f1ap_cu_task.h"
#include "f1ap_du_task.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include <openair2/LAYER2/nr_pdcp/nr_pdcp.h>
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include "openair2/E1AP/e1ap.h"
#include "gnb_config.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/GNB_APP/gnb_config_ng.h"
#include "openair2/XNAP/xnap_gNB_task.h"
#include "openair2/XNAP/xnap_gNB_management_procedures.h"
#include "openair3/NGAP/ngap_gNB_defs.h"
#include "openair3/NGAP/ngap_gNB_management_procedures.h"

extern RAN_CONTEXT_t RC;
/*------------------------------------------------------------------------------*/


uint32_t gNB_app_register(uint32_t gnb_id_start, uint32_t gnb_id_end)//, const Enb_properties_array_t *enb_properties)
{
  uint32_t         gnb_id;
  MessageDef      *msg_p;
  uint32_t         register_gnb_pending = 0;

  for (gnb_id = gnb_id_start; (gnb_id < gnb_id_end) ; gnb_id++) {
    {
      if (IS_SA_MODE(get_softmodem_params())) {

        // note:  there is an implicit relationship between the data structure and the message name
        msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, NGAP_REGISTER_GNB_REQ); //Message Temporarily reuse

        RCconfig_NR_NG(msg_p, gnb_id);

        itti_send_msg_to_task (TASK_NGAP, GNB_MODULE_ID_TO_INSTANCE(gnb_id), msg_p);
      }
    }

    register_gnb_pending++;
    }

  return register_gnb_pending;
}


/*------------------------------------------------------------------------------*/
uint32_t gNB_app_register_x2(uint32_t gnb_id_start, uint32_t gnb_id_end) {
  uint32_t         gnb_id;
  MessageDef      *msg_p;
  uint32_t         register_gnb_x2_pending = 0;

  for (gnb_id = gnb_id_start; (gnb_id < gnb_id_end) ; gnb_id++) {
    {
      msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, X2AP_REGISTER_ENB_REQ);
      RCconfig_NR_X2(msg_p, gnb_id);
      itti_send_msg_to_task (TASK_X2AP, ENB_MODULE_ID_TO_INSTANCE(gnb_id), msg_p);
      register_gnb_x2_pending++;
    }
  }

  return register_gnb_x2_pending;
}

/*================================================================================*/
uint32_t gNB_app_register_xn(instance_t instance){
  uint32_t                        gnb_nb = RC.nb_nr_inst;
  uint32_t                        gnb_inst_start = 0;
  uint32_t                        gnb_inst_end = gnb_inst_start + gnb_nb;
  int                             register_gnb_xn_pending = 0;
  struct                          served_guami_s *guami;
  struct                          plmn_identity_s  *plmn;
  struct                          served_region_id_s *region;
       
  ngap_gNB_amf_data_t             *amf_node;
  ngap_gNB_instance_t *instance_p = ngap_gNB_get_instance(instance);
  for(uint32_t gnb_inst = gnb_inst_start; (gnb_inst < gnb_inst_end) ; gnb_inst++){
     MessageDef   *msg_p = itti_alloc_new_message (TASK_GNB_APP, 0, XNAP_REGISTER_GNB_REQ);
     xnap_register_gnb_req_t *msg   = &XNAP_REGISTER_GNB_REQ(msg_p);
      
     // reading xnap net configuration 
     msg->net_config  =   Read_IPconfig_Xn();      
     // gNB ID
     msg->setup_info.gNB_id = instance_p->gNB_id;
     LOG_D(GNB_APP, "gNB_ID in Registering Xn Handover Procedure: %u \n", msg->setup_info.gNB_id);
     msg->setup_info.plmn.mcc = instance_p->plmn[0].plmn.mcc; // correct?
     msg->setup_info.plmn.mnc = instance_p->plmn[0].plmn.mnc;
     msg->setup_info.plmn.mnc_digit_length = instance_p->plmn[0].plmn.mnc_digit_length;

     // TAI support list
     msg->setup_info.num_tai = 1; // as avilable now 
     for(int i = 0; i< msg->setup_info.num_tai; i++){
         msg->setup_info.tai_support[i].tac = instance_p->tac;
         msg->setup_info.tai_support[i].num_plmn = instance_p->num_plmn;
         for (int j = 0; j < instance_p->num_plmn; j++){
             msg->setup_info.tai_support[i].plmn_support[j].plmn.mcc = instance_p->plmn[j].plmn.mcc;
             msg->setup_info.tai_support[i].plmn_support[j].plmn.mnc = instance_p->plmn[j].plmn.mnc;;
             msg->setup_info.tai_support[i].plmn_support[j].plmn.mnc_digit_length =  instance_p->plmn[j].plmn.mnc_digit_length;
             msg->setup_info.tai_support[i].plmn_support[j].num_nssai = instance_p->plmn[j].num_nssai;
             for (int k = 0; k < msg->setup_info.tai_support[i].plmn_support[j].num_nssai; k++){
                 msg->setup_info.tai_support[i].plmn_support[j].s_nssai[k].sst = instance_p->plmn[j].s_nssai[k].sst;
                 if(instance_p->plmn[j].s_nssai[k].sd)
                   msg->setup_info.tai_support[i].plmn_support[j].s_nssai[k].sd  = instance_p->plmn[j].s_nssai[k].sd;
             }
         }
     }
     
    // Global AMF Region IDs
    int r = 0; 
    /* Iterate over AMF tree */
    RB_FOREACH(amf_node, ngap_amf_map, &instance_p->ngap_amf_head) {
        /* Iterate over served GUAMIs */
        STAILQ_FOREACH(guami, &amf_node->served_guami, next) {
            /* Iterate over region IDs */
            STAILQ_FOREACH(plmn, &guami->served_plmns, next) {
               msg->setup_info.amf_region_info[r].plmn.mcc = plmn->mcc;
               msg->setup_info.amf_region_info[r].plmn.mnc = plmn->mnc;
               msg->setup_info.amf_region_info[r].plmn.mnc_digit_length = plmn->mnc_digit_length;
            }
 
            /* Iterate over region IDs */
            STAILQ_FOREACH(region, &guami->served_region_ids, next) {
               msg->setup_info.amf_region_info[r].amf_region_id = region->amf_region_id;
            }
          r++;
       }
   }
   msg->setup_info.num_amf_regions = r;
   register_gnb_xn_pending++;
   
   createXninst(gnb_inst, &msg->setup_info, &msg->net_config);
   LOG_I(GNB_APP ,"Sending XNAP_register gNB request from gNB app\n");
   itti_send_msg_to_task (TASK_XNAP, GNB_MODULE_ID_TO_INSTANCE(gnb_inst), msg_p);
  }
  return register_gnb_xn_pending; 
}

/*================================================================================*/

/*------------------------------------------------------------------------------*/

void *gNB_app_task(void *args_p)
{

  MessageDef                      *msg_p           = NULL;
  const char                      *msg_name        = NULL;
  instance_t                      instance;
  int                             result;
  /* for no gcc warnings */
  (void)instance;

  int cell_to_activate = 0;
  itti_mark_task_ready (TASK_GNB_APP);
  ngran_node_t node_type = get_node_type();

  if (RC.nb_nr_inst > 0) {
    if (node_type == ngran_gNB_CUCP ||
        node_type == ngran_gNB_CU ||
        node_type == ngran_eNB_CU ||
        node_type == ngran_ng_eNB_CU) {

      if (itti_create_task(TASK_CU_F1, F1AP_CU_task, NULL) < 0) {
        LOG_E(F1AP, "Create task for F1AP CU failed\n");
        AssertFatal(1==0,"exiting");
      }
    }

    if (node_type == ngran_gNB_CUCP) {
      if (itti_create_task(TASK_CUCP_E1, E1AP_CUCP_task, NULL) < 0)
        AssertFatal(false, "Create task for E1AP CP failed\n");
      E1_t e1type = CPtype;
      MessageDef *msg = RCconfig_NR_CU_E1(&e1type);
      AssertFatal(msg != NULL, "Send ITTI to task for E1AP CP failed\n");
      // this sends the E1AP_REGISTER_REQ to CU-CP so it sets up the socket
      // it does NOT use the E1AP part
      itti_send_msg_to_task(TASK_CUCP_E1, 0, msg);
    }
    
    if (node_type == ngran_gNB_CUCP || node_type == ngran_gNB_CU || node_type == ngran_gNB) {
      if (is_xnap_enabled()) {
        if (itti_create_task(TASK_XNAP, xnap_task, NULL) < 0) {
          LOG_E(XNAP, "Create task for XNAP failed\n");
        }
      } else {
        LOG_I(XNAP, "XNAP is disabled.\n");
      }
    }

    if (node_type == ngran_gNB_CUUP) {
      AssertFatal(false, "To run CU-UP use executable nr-cuup\n");
    }

    if (NODE_IS_DU(node_type)) {
      if (itti_create_task(TASK_DU_F1, F1AP_DU_task, NULL) < 0) {
        LOG_E(F1AP, "Create task for F1AP DU failed\n");
        AssertFatal(1==0,"exiting");
      }
    }
    if (NODE_IS_DU(node_type) || NODE_IS_MONOLITHIC(node_type)) {
      // need to check SA?
      nr_mac_send_f1_setup_req();
    }
  }
  do {
    // Wait for a message
    itti_receive_msg (TASK_GNB_APP, &msg_p);

    msg_name = ITTI_MSG_NAME (msg_p);
    instance = ITTI_MSG_DESTINATION_INSTANCE (msg_p);

    switch (ITTI_MSG_ID(msg_p)) {
    case TERMINATE_MESSAGE:
      LOG_W(GNB_APP, " *** Exiting GNB_APP thread\n");
      itti_exit_task ();
      break;

    case MESSAGE_TEST:
      LOG_I(GNB_APP, "Received %s\n", ITTI_MSG_NAME(msg_p));
      break;



    case NGAP_REGISTER_GNB_CNF:
      LOG_I(GNB_APP, "[gNB %ld] Received %s: associated AMF %d\n", instance, msg_name,
            NGAP_REGISTER_GNB_CNF(msg_p).nb_amf);
      if (is_xnap_enabled()) {
         gNB_app_register_xn(instance);
      }
      break;

    case F1AP_SETUP_RESP:
      AssertFatal(false, "Should not received this, logic bug\n");
      break;

    case F1AP_GNB_CU_CONFIGURATION_UPDATE:
      AssertFatal(NODE_IS_DU(node_type), "Should not have received F1AP_GNB_CU_CONFIGURATION_UPDATE in CU/gNB\n");
      LOG_I(GNB_APP,
            "Received %s: associated with %d cells to activate\n",
            ITTI_MSG_NAME(msg_p),
            F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).num_cells_to_activate);
      cell_to_activate += F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).num_cells_to_activate;
      gNB_app_handle_f1ap_gnb_cu_configuration_update(&F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p));

      /* Check if at least gNB is registered with one AMF */
      AssertFatal(cell_to_activate == 1,"No cells to activate or cells > 1 %d\n",cell_to_activate);

      break;

    case NGAP_DEREGISTERED_GNB_IND:
      LOG_W(GNB_APP, "[gNB %ld] Received %s: associated AMF %d\n", instance, msg_name,
            NGAP_DEREGISTERED_GNB_IND(msg_p).nb_amf);

      /* TODO handle recovering of registration */
      break;

    case TIMER_HAS_EXPIRED:
      LOG_I(GNB_APP, " Received %s: timer_id %ld\n", msg_name, TIMER_HAS_EXPIRED(msg_p).timer_id);
      break;

    case GNB_SAT_POSITION_UPDATE:
      LOG_I(GNB_APP, " Received GNB_SAT_POSITION_UPDATE message\n");
      nr_update_sib19(&GNB_SAT_POSITION_UPDATE(msg_p));
      break;

    default:
      LOG_E(GNB_APP, "Received unexpected message %s\n", msg_name);
      break;
    }

    result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
    AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
  } while (1);


  return NULL;
}
