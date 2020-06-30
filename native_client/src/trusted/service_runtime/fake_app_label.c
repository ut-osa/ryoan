#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "native_client/src/trusted/service_runtime/fake_app_label.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium.h"
#include "native_client/src/trusted/service_runtime/security.h"
#include "native_client/src/trusted/service_runtime/sel_ldr.h"

static struct NaClLabel* app_label = NULL;

struct NaClLabel* GetFakeAppLabel(struct NaClApp *nap) {
  if (app_label == NULL) {
    uint32_t label_size;
    if (nap->fake_app_label == NULL) {
      NaClLog(LOG_FATAL, "Fake app label is not set\n");
    }
    label_size = strlen(nap->fake_app_label)/2;
    app_label = NaClLabelMake(label_size, LABEL_OTHER);
    sodium_hex2bin(NaClLabelGetLabel(app_label), label_size,
                   nap->fake_app_label, strlen(nap->fake_app_label),
                   NULL, NULL, NULL);
  }
  return app_label;
}

void FakeAppLabelFinalize(void) {
  if (!app_label)
    free(app_label);
  app_label = NULL;
}
