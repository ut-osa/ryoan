
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium/version.h"

const char *
sodium_version_string(void)
{
    return SODIUM_VERSION_STRING;
}

int
sodium_library_version_major(void)
{
    return SODIUM_LIBRARY_VERSION_MAJOR;
}

int
sodium_library_version_minor(void)
{
    return SODIUM_LIBRARY_VERSION_MINOR;
}
