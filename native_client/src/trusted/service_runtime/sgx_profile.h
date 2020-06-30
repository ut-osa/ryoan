#ifndef _NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME
#define _NATIVE_CLIENT_SRC_TRUSTED_SERVICE_RUNTIME

#define SGX_PROFILE_ENV_NAME "RYOAN_SGX_PROFILE"

struct sgx_profile {
	double eenter_usecs;
	double eresume_usecs;
	double emodpe_usecs;
};

extern struct sgx_profile active_sgx_profile;

int NaClUpdateSgxProfileFromFilename(char *filename);
void NaClUpdateSgxProfileFromEnv(void);
void NaClLogSgxProfile(int LogLevel);

#endif
