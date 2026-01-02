#include <stdio.h>

#define OK_OR_RET(fn)                                                          \
  {                                                                            \
    const chif_net_result res = fn;                                            \
    ALF_CHECK_TRUE(state, !res);                                               \
    if (res) {                                                                 \
      printf("[chif_net] error [%s]", chif_net_result_to_string(res));         \
      return;                                                                  \
    }                                                                          \
  }
