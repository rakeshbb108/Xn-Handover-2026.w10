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

#include "connect-mplane.h"
#include "init-mplane.h"
#include "common/utils/assertions.h"

#include <libyang/libyang.h>
#include <nc_client.h>

#define CLI_CH_TIMEOUT 10         // time to wait for call-home

static int my_auth_hostkey_check(const char *hostname, ssh_session session, void *priv)
{
  (void)hostname;
  (void)session;
  (void)priv;

  return 0;
}

/* be aware that this function might need to be expanded to unix and TLS use cases */
int listen_mplane(ru_session_t **ru_session, char **du_key_pair, char *ru_username, size_t num_ru)
{
  int port = NC_PORT_CH_SSH;
  char *host = "0.0.0.0";       // better IPv4
  int timeout = CLI_CH_TIMEOUT;

  /* create the session */
  nc_client_ssh_ch_set_username(ru_username);
  nc_client_ssh_ch_add_bind_listen(host, port);

  nc_client_ssh_ch_set_auth_pref(NC_SSH_AUTH_PASSWORD, -1);
  nc_client_ssh_ch_set_auth_pref(NC_SSH_AUTH_PUBLICKEY, 1);  // ssh-key identification
  nc_client_ssh_ch_set_auth_pref(NC_SSH_AUTH_INTERACTIVE, -1);

  static u_int16_t set = 0;
  if (!set) {
    int keypair_ret = nc_client_ssh_ch_add_keypair(du_key_pair[0], du_key_pair[1]);
    assert(keypair_ret == 0 && "Unable to authenticate RU\n");
    set = 1;
  }
  nc_client_ssh_ch_set_auth_hostkey_check_clb(my_auth_hostkey_check, "DATA");  // host-key identification

  printf("Waiting %ds for an SSH Call Home connection on port %u...\n", timeout, port);

  int ret = nc_accept_callhome(timeout * 1000, NULL, &ru_session[num_ru]->session); // check the right session; maybe session[0] is already ongoing, and need to create session[1]
  if(!set){
    assert(ret == 1 && "SSH Call Home failed.");
  }else{
    if(ret != 1){
      printf("[ERROR] SSH Call Home failed.\n");
      return 0;
    }
  }

  nc_client_ssh_ch_del_bind(host, port);

  const char *ru_ip_add = nc_session_get_host(ru_session[num_ru]->session);
  ru_session[num_ru]->ru_ip_add = malloc(strlen(ru_ip_add) + 1);
  memcpy(ru_session[num_ru]->ru_ip_add, ru_ip_add, strlen(ru_ip_add) + 1);
  printf("Successfuly connected to RU with IP address %s\n", ru_session[num_ru]->ru_ip_add);

  return 1;
}

bool connect_mplane(ru_session_t *ru_session)
{
  int port = NC_PORT_SSH;

  nc_client_ssh_set_username(ru_session->username);

  nc_client_ssh_set_auth_pref(NC_SSH_AUTH_PUBLICKEY, 1);  // ssh-key identification
  nc_client_ssh_set_auth_pref(NC_SSH_AUTH_PASSWORD, -1);
  nc_client_ssh_set_auth_pref(NC_SSH_AUTH_INTERACTIVE, -1);

  nc_client_ssh_set_auth_hostkey_check_clb(my_auth_hostkey_check, "DATA");  // host-key identification

  MP_LOG_I("RPC request to RU \"%s\" = <connect> with username \"%s\" and port ID \"%d\".\n", ru_session->ru_ip_add, ru_session->username, port);
  ru_session->session = nc_connect_ssh(ru_session->ru_ip_add, port, NULL);
  AssertError(ru_session->session != NULL, return false, "[MPLANE] RU IP address %s unreachable. Maybe M-plane is disabled on the RU?\n", ru_session->ru_ip_add);

  MP_LOG_I("Successfuly connected to RU \"%s\" with username \"%s\" and port ID \"%d\".\n", ru_session->ru_ip_add, ru_session->username, port);

  return true;
}

void disconnect_mplane(void *rus_disconnect)
{
  ru_session_list_t *ru_session_list = (ru_session_list_t *)rus_disconnect;
  free_ru_session_list(ru_session_list);

  nc_client_destroy();
}
