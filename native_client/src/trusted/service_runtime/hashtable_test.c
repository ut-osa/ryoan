/*
 * Copyright (c) 2016 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "native_client/src/include/nacl_macros.h"
#include "native_client/src/include/portability.h"

#include "native_client/src/shared/platform/platform_init.h"

#include "native_client/src/trusted/service_runtime/hashtable.h"
#include "native_client/src/trusted/service_runtime/libsodium/src/libsodium/include/sodium.h"

char list[32][2][32];

static int
remove_func(void *key, uint32_t key_size, void *data, uint32_t data_size, void *arg) {
	data_size = data_size;
	data = data;
	arg = arg;

	if (key_size > 7) {
		if (!strncasecmp(key, "content", 7)) {
			return 1;
		}
	}
	return 0;
}

int StringKeyValue(void) {
  cfuhash_table_t *hash;
	char *val = NULL;
  uint32_t i;

  printf("Testing string key and string value");
  hash = cfuhash_new_with_initial_size(30);
  cfuhash_set_flag(hash, CFUHASH_FROZEN_UNTIL_GROWS);

  cfuhash_put(hash, "var1", "value1");
	cfuhash_put(hash, "var2", "value2");
	cfuhash_put(hash, "var3", "value3");
	cfuhash_put(hash, "var4", "value4");

	cfuhash_pretty_print(hash, stdout);

	printf("\n\n");
	val = (char *)cfuhash_delete(hash, "var3");
	printf("delete: got back '%s'\n\n", val);

	cfuhash_pretty_print(hash, stdout);

	printf("\n\n");
	val = cfuhash_get(hash, "var2");
	printf("got var2='%s'\n", val);
	printf("var4 %s\n", cfuhash_exists(hash, "var4") ? "exists" : "does NOT exist!!!");

	printf("%d entries, %d buckets used out of %d\n", (int)cfuhash_num_entries(hash), (int)cfuhash_num_buckets_used(hash), (int)cfuhash_num_buckets(hash));

	cfuhash_pretty_print(hash, stdout);
	
	cfuhash_clear(hash);
	for (i = 0; i < 32; i++) {
	  uint32_t used = cfuhash_num_buckets_used(hash);
		uint32_t num_buckets = cfuhash_num_buckets(hash);
		uint32_t num_entries = cfuhash_num_entries(hash);
		cfuhash_put(hash, list[i][0], list[i][1]);
		printf("%u entries, %u buckets used out of %u (%.2f)\n", (int)num_entries, (int)used, (int)num_buckets, (float)num_entries/(float)num_buckets);

	}

	cfuhash_pretty_print(hash, stdout);

	{
		char **keys = NULL;
		uint32_t *key_sizes = NULL;
		uint32_t key_count = 0;
		keys = (char **)cfuhash_keys_data(hash, &key_count, &key_sizes, 0);

		printf("\n\nkeys (%u):\n", (int)key_count);
		for (i = 0; i < key_count; i++) {
			printf("\t%s\n", keys[i]);
			free(keys[i]);
		}
		free(keys);
		free(key_sizes);
	}

	cfuhash_clear(hash);
	printf("%d entries, %d buckets used out of %d\n", (int)cfuhash_num_entries(hash), (int)cfuhash_num_buckets_used(hash), (int)cfuhash_num_buckets(hash));

  cfuhash_destroy(hash);
  return 0;
}

int StringKeyValueCaseInsensitive(void) {
  cfuhash_table_t *hash;
  cfuhash_table_t *hash2;
	printf("\n\n====> case-insensitive hash:\n\n");
  hash = cfuhash_new_with_initial_size(30);
  hash2 = cfuhash_new_with_initial_size(4);
  cfuhash_set_flag(hash, CFUHASH_IGNORE_CASE);
	cfuhash_put(hash, "Content-Type", "value1");
	cfuhash_put(hash, "Content-Length", "value2");
	
	cfuhash_put(hash2, "var3", "value3");
	cfuhash_put(hash2, "var4", "value4");
	

	cfuhash_pretty_print(hash, stdout);
	cfuhash_copy(hash2, hash);
	cfuhash_pretty_print(hash, stdout);

	{
		char keys[4][32] = { "content-type", "content-length", "Var3", "VaR4" };
		size_t i = 0;

		for (i = 0; i < 4; i++) {
			printf("%s => %s\n", keys[i], (char *)cfuhash_get(hash, keys[i]));
		}

		cfuhash_foreach_remove(hash, remove_func, NULL, NULL);
		printf("\n\nafter removing content*:\n");
		for (i = 0; i < 4; i++) {
			printf("%s => %s\n", keys[i], (char *)cfuhash_get(hash, keys[i]));
		}

	}

	cfuhash_destroy(hash);
	cfuhash_destroy(hash2);

  return 0;
}

int StringKeyNumValue(void) {
  cfuhash_table_t *hash;
  int *v1 = NULL, *v2 = NULL, *fd = NULL, *fd2 = NULL;
  hash = cfuhash_new_with_initial_size(30);

  printf("\n\n===> String key and integer value:\n\n");
  fd = (int *)malloc(sizeof(int));
  *fd = 4;
  cfuhash_put(hash, "zhitingz/Documents/service_runtime/file.txt", fd);
  fd2 = (int *)malloc(sizeof(int));
  *fd2 = 5;
  cfuhash_put(hash, "zhitingz/Documents/service_runtime/file2.txt", fd2);

  v1 = (int *)cfuhash_get(hash, "zhitingz/Documents/service_runtime/file.txt");
  v2 = (int *)cfuhash_get(hash, "zhitingz/Documents/service_runtime/file2.txt");

  printf("Got value: %d, %d\n", *v1, *v2);
  cfuhash_destroy_with_free_fn(hash, free);
  return 0;
}

int OnlyKey(void) {
  cfuhash_table_t *hash;
  uint8_t *k1 = (uint8_t *) malloc(10);
  uint8_t *k2 = (uint8_t *) malloc(10);

  hash = cfuhash_new_with_initial_size(10);
  randombytes_buf(k1, 10);
  randombytes_buf(k2, 10);
  cfuhash_put_data(hash, k1, 10, NULL, 0, NULL);
  cfuhash_put_data(hash, k2, 10, NULL, 0, NULL);
  if (!cfuhash_exists_data(hash, k1, 10)) {
    printf("Cannot find inserted key\n");
    return -1;
  }
  if (!cfuhash_exists_data(hash, k2, 10)) {
    printf("Cannot find inserted key\n");
    return -1;
  }
  printf("Pass key checking\n");
  cfuhash_destroy_with_free_fn(hash, free);
  return 0;
}

int main(void) {
  int nerrors = 0;
  int i;

	for (i = 0; i < 32; i++) {
		sprintf(list[i][0], "test_var%d", (int)i);
		sprintf(list[i][1], "value%d", (int)i);
	}

  NaClPlatformInit();
  StringKeyValue();
  StringKeyValueCaseInsensitive();
  StringKeyNumValue();
  if (OnlyKey() < 0) {
    printf("Fail to get key\n");
    nerrors += 1;
  }
  NaClPlatformFini();
  return nerrors != 0;
}
