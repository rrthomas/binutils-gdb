/* Do not modify this file.  */
/* It is created automatically by the Makefile.  */
#include "libiberty.h"
#include "sim-module.h"
extern MODULE_INIT_FN sim_install_dv_sockser;
extern MODULE_INIT_FN sim_install_engine;
extern MODULE_INIT_FN sim_install_hw;
extern MODULE_INIT_FN sim_install_profile;
extern MODULE_INIT_FN sim_install_trace;
MODULE_INSTALL_FN * const sim_modules_detected[] = {
  sim_install_dv_sockser,
  sim_install_engine,
  sim_install_hw,
  sim_install_profile,
  sim_install_trace,
};
const int sim_modules_detected_len = ARRAY_SIZE (sim_modules_detected);
