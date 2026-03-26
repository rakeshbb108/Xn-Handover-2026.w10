#ifndef OPENAIRINTERFACE_RC_CTRL_SERVICE_STYLE_3_H
#define OPENAIRINTERFACE_RC_CTRL_SERVICE_STYLE_3_H

#include "common/platform_types.h"

bool handover_ue(ue_id_t rrc_ue_id);

typedef enum{
  Handover_Control_7_6_4_1 = 1,
  Conditional_Handover_Control_7_6_4_1 = 2,
  DAPS_Handover_Control_7_6_4_1 = 3,
} rc_ctrl_service_style_3_act_id;

typedef enum {
  Handover_Control_8_4_4_1 = 1,
  Conditional_Handover_Control_8_4_4_1 = 2,
  DAPS_Handover_Control_8_4_4_1 = 3,
} connected_mode_mobility_control_param_id_e;

#endif // OPENAIRINTERFACE_RC_CTRL_SERVICE_STYLE_3_H
