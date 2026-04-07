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

#ifndef SUBSCRIBE_MPLANE_H
#define SUBSCRIBE_MPLANE_H

#include "ru-mplane-api.h"

#include <stdbool.h>
typedef struct ru_global_metrics{
  uint32_t total_rx_good_pkt_cnt;
  uint32_t total_rx_bit_rate;
  uint32_t oran_rx_bit_rate;
  uint32_t oran_rx_on_time;
  uint32_t oran_rx_early;
  uint32_t oran_rx_late;
  uint32_t oran_rx_corrupt;
  uint32_t oran_rx_total;
  uint32_t oran_rx_total_c;
  uint32_t oran_rx_on_time_c;
  uint32_t oran_rx_early_c;
  uint32_t oran_rx_late_c;
  uint32_t oran_rx_error_drop;
  uint32_t oran_tx_total;
  uint32_t oran_tx_total_c;
  uint32_t ru_reset;
} ru_global_metrics_t;

bool subscribe_mplane(ru_session_t *ru_session, const char *stream, const char *filter, void *answer);

bool update_timer_mplane(ru_session_t *ru_session, char **answer);

void print_performance_metrics(const char *xml);

#endif /* SUBSCRIBE_MPLANE_H */
