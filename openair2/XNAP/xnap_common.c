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

/*! \file xnap_common.c
 * \brief xnap encoder,decoder dunctions for gNB
 * \author Sreeshma Shiv <sreeshmau@iisc.ac.in>
 * \date Dec 2023
 * \version 1.0
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "assertions.h"
#include "conversions.h"
#include "intertask_interface.h"
#include "xnap_common.h"

int xnap_gNB_encode_pdu(XNAP_XnAP_PDU_t *pdu, uint8_t **buffer, uint32_t *len)
{
  ssize_t encoded;

  DevAssert(pdu != NULL);
  DevAssert(buffer != NULL);
  DevAssert(len != NULL);

  xer_fprint(stdout, &asn_DEF_XNAP_XnAP_PDU, (void *)pdu);

  encoded = aper_encode_to_new_buffer(&asn_DEF_XNAP_XnAP_PDU, 0, pdu, (void **)buffer);

  if (encoded < 0) {
    return -1;
  }

  *len = encoded;

  return encoded;
}

int xnap_gNB_decode_pdu(XNAP_XnAP_PDU_t *pdu, const uint8_t *const buffer, uint32_t length)
{
  asn_dec_rval_t dec_ret;

  DevAssert(buffer != NULL);

  dec_ret = aper_decode(NULL, &asn_DEF_XNAP_XnAP_PDU, (void **)&pdu, buffer, length, 0, 0);
  xer_fprint(stdout, &asn_DEF_XNAP_XnAP_PDU, pdu);
  if (dec_ret.code != RC_OK) {
    LOG_E(XNAP, "Failed to decode PDU\n");
    return -1;
  }
  return 0;
}


int xnap_gNB_set_cause(XNAP_Cause_t *cause_p,const xnap_cause_t *in)
{
  DevAssert(cause_p != NULL);
  switch (in->type) {
    case XNAP_CAUSE_RADIO_NETWORK:
      cause_p->present = XNAP_Cause_PR_radioNetwork;
      cause_p->choice.radioNetwork = in->value;
      break;
    case XNAP_CAUSE_TRANSPORT:
      cause_p->present = XNAP_Cause_PR_transport;
      cause_p->choice.transport = in->value;
      break;
    case XNAP_CAUSE_PROTOCOL:
      cause_p->present = XNAP_Cause_PR_protocol;
      cause_p->choice.protocol = in->value;
      break;
    case XNAP_CAUSE_MISC:
      cause_p->present = XNAP_Cause_PR_misc;
      cause_p->choice.misc = in->value;
      break;
    case XNAP_CAUSE_NOTHING:
    default:
      cause_p->present = XNAP_Cause_PR_NOTHING;
      break;
  }
  return 0;
}

xnap_cause_t decode_xnap_cause(const XNAP_Cause_t *in){
  xnap_cause_t out = {0};
  switch (in->present) {
     case XNAP_Cause_PR_radioNetwork:
         out.type = XNAP_CAUSE_RADIO_NETWORK;
         out.value = in->choice.radioNetwork;
         break;

     case XNAP_Cause_PR_transport:
         out.type = XNAP_CAUSE_TRANSPORT;
         out.value = in->choice.transport;
         break;

     case XNAP_Cause_PR_protocol:
         out.type = XNAP_CAUSE_PROTOCOL;
         out.value = in->choice.protocol;
         break;

     case XNAP_Cause_PR_misc:
         out.type = XNAP_CAUSE_MISC;
         out.value = in->choice.misc;
         break;
  
     default:
         out.type = XNAP_CAUSE_RADIO_NETWORK;
         LOG_E(XNAP, "Unknown failure cause %d\n", in->present);
         break;
   }
   return out;
}
