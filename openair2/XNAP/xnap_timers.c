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

#include "xnap_timers.h"
#include "assertions.h"
#include "PHY/defs_common.h" /* TODO: try to not include this */
#include "xnap_messages_types.h"
#include "xnap_gNB_defs.h"
#include "xnap_ids.h"
#include "xnap_gNB_management_procedures.h"
#include "xnap_gNB_generate_messages.h"

void xnap_timers_init(xnap_timers_t *t, int t_reloc_prep, int tx2_reloc_overall, int t_dc_prep, int t_dc_overall)
{
  t->tti = 0;
  t->t_reloc_prep = t_reloc_prep;
  t->tx2_reloc_overall = tx2_reloc_overall;
  t->t_dc_prep = t_dc_prep;
  t->t_dc_overall = t_dc_overall;
}
