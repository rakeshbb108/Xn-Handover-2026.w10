#include "openair2/RRC/NR/rrc_gNB_du.h"
#include "openair2/RRC/NR/rrc_gNB_mobility.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair2/RRC/NR/nr_rrc_defs.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "rc_ctrl_service_style_3.h"

#if defined(NGRAN_GNB_DU) || defined(NGRAN_GNB_CUCP)
void nr_HO_F1_trigger_xApp(gNB_RRC_INST *rrc, uint32_t rrc_ue_id)
{
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, rrc_ue_id);
  if (ue_context_p == NULL) {
    LOG_E(NR_RRC, "cannot find UE context for UE ID %d\n", rrc_ue_id);
    return;
  }
  gNB_RRC_UE_t *ue = &ue_context_p->ue_context;
  nr_rrc_du_container_t *source_du = get_du_for_ue(rrc, ue->rrc_ue_id);
  if (source_du == NULL) {
    f1_ue_data_t ue_data = cu_get_f1_ue_data(rrc_ue_id);
    LOG_E(NR_RRC, "cannot get source gNB-DU with assoc_id %d for UE %u\n", ue_data.du_assoc_id, ue->rrc_ue_id);
    return;
  }

  nr_rrc_du_container_t *target_du = find_target_du(rrc, source_du->assoc_id);
  if (target_du == NULL) {
    LOG_E(NR_RRC, "No target gNB-DU found. Handover for UE %u aborted.\n", ue->rrc_ue_id);
    return;
  }

  nr_rrc_trigger_f1_ho(rrc, ue, source_du, target_du);
}

bool handover_ue(ue_id_t rrc_ue_id)
{
  rrc_gNB_ue_context_t *ue = NULL;
  if (!RC.nrrrc)
    printf("no RRC present, cannot list counts\n");
  ue = rrc_gNB_get_ue_context(RC.nrrrc[0], rrc_ue_id);
  if(!ue) {
    printf("could not find UE with ue_id %ld in RRC\n", rrc_ue_id);
    return false;
  }

  gNB_RRC_UE_t *UE = &ue->ue_context;
  nr_HO_F1_trigger_xApp(RC.nrrrc[0], UE->rrc_ue_id);

  return true;
}
#endif
