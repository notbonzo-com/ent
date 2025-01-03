#ifndef SAFEMEM_H
#define SAFEMEM_H

#include <stdlib.h>

#define SAFE_FREE(obj) \
  do {                 \
    if (obj) {         \
      free(obj);       \
      obj = nullptr;   \
    }                  \
  } while (0);

#endif /* SAFEMEM_H */
