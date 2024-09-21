the pointers being used in player to replace the mallocs need to be memset to 0

  self = (plm_demux_t *)PLM_MALLOC(sizeof(plm_demux_t));
  memset(self, 0, sizeof(plm_demux_t));