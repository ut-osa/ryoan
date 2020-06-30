#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "native_client/src/shared/platform/nacl_log.h"
#include "native_client/src/trusted/service_runtime/sgx_profile.h"

/* measured on SGX harware */
#define EENTER_DELAY 3.9
#define ERESUME_DELAY 3.14
#define EMODPE_DELAY (EENTER_DELAY / 10)

struct sgx_profile active_sgx_profile = {
	.eenter_usecs  = EENTER_DELAY,
	.eresume_usecs = ERESUME_DELAY,
	.emodpe_usecs  = EMODPE_DELAY,
};

void NaClLogSgxProfile(int logLevel)
{
	(void)(logLevel);
	fprintf(stderr, "active_sgx_profile (all usecs): "
				    "EENTER(%.2f), ERESUME(%.2f), EMODPE(%.2f)\n",
					active_sgx_profile.eenter_usecs,
					active_sgx_profile.eresume_usecs,
					active_sgx_profile.emodpe_usecs);
}

static int try_extract(char *raw, const char *needle, double *to_update)
{
	char *substr, *bak;
	double new_val;

	substr = strcasestr(raw, needle);
	if (substr) {
		substr += strlen(needle);
		new_val = strtod(substr, &bak);
		if (bak) {
			*to_update = new_val;
			return 0;
		}
	}
	return -1;
}

static int process_line(char *line, struct sgx_profile *profile)
{
	int ret;

	ret = try_extract(line, "EENTER", &profile->eenter_usecs);
	if (!ret)
		goto done;
	ret = try_extract(line, "ERESUME", &profile->eresume_usecs);
	if (!ret)
		goto done;
	ret = try_extract(line, "EMODPE", &profile->emodpe_usecs);

done:
	return ret;
}

int NaClUpdateSgxProfileFromFilename(char *filename)
{
	FILE *fp;
    char * line;
    size_t len;
	int ret = 0;
	struct sgx_profile profile = active_sgx_profile;

	fp = fopen(filename, "r");
	if (!fp) {
		perror(filename);
		return -1;
	}

    while (!ret && getline(&line, &len, fp) != -1)
		ret = process_line(line, &profile);

	fclose(fp);
	if (!ret)
		active_sgx_profile = profile;
	else
		NaClLog(LOG_ERROR, "SGX profile parse failed on %s\n", line);
	return ret;
}

void NaClUpdateSgxProfileFromEnv()
{
	char *filename = getenv(SGX_PROFILE_ENV_NAME);

	if (filename) {
		if (NaClUpdateSgxProfileFromFilename(filename)) {
			fprintf(stderr, "Failed to parse env SGX_PROFILE ("
					        SGX_PROFILE_ENV_NAME"=%s)\n", filename);
		}
	}
}
