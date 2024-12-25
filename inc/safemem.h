#ifndef SAFEMEM_H
#define SAFEMEM_H

#define safe_free(obj) \
  do {                 \
    if (obj) {         \
      free(obj);       \
      obj = nullptr;   \
    }                  \
  } while (0);

#endif /* SAFEMEM_H */
