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

#ifndef __XNAP_GNB_MANAGEMENT_PROCEDURES__H__
#define __XNAP_GNB_MANAGEMENT_PROCEDURES__H__
#include "openair2/XNAP/xnap_gNB_defs.h"

void createXninst(instance_t instanceP, xnap_setup_req_t *req, xnap_net_config_t *nc);
void updateXninst(instance_t instanceP, xnap_setup_req_t *req, xnap_net_config_t *nc, sctp_assoc_t assoc_id);
sctp_assoc_t xnap_gNB_get_assoc_id(xnap_gNB_instance_t *instance, long nci);
void xnap_dump_trees(const instance_t instance);
xnap_gNB_instance_t *xnap_gNB_get_instance(instance_t instanceP);
xnap_gNB_data_t *xnap_get_gNB(instance_t instance, sctp_assoc_t assoc_id);
void xnap_insert_gnb(instance_t instance, xnap_gNB_data_t *xnap_gnb_data_p);
void xnap_handle_xn_setup_message(instance_t instance, sctp_assoc_t assoc_id, int sctp_shutdown);
#endif /* __XNAP_GNB_MANAGEMENT_PROCEDURES__H__ */
