// clang-format off
#define _GNU_SOURCE
// #define _GNU_SOURCE needed to use POSIX-compliant strdup
#include "kvs_lru.h"

#include <stdlib.h>
#include <string.h>

struct kvs_lru {
  kvs_base_t* kvs_base;
  char** keys;
  char** values; // added a values field
  int capacity;
  int size;
};

kvs_lru_t* kvs_lru_new(kvs_base_t* kvs, int capacity) {
  kvs_lru_t* kvs_lru = malloc(sizeof(kvs_lru_t));
  kvs_lru->kvs_base = kvs;
  kvs_lru->capacity = capacity;
  kvs_lru->size = 0;
  kvs_lru->keys = calloc(capacity, sizeof(char*));
  kvs_lru->values = calloc(capacity, sizeof(char*)); // allocate memory for values
  return kvs_lru;
}

void kvs_lru_free(kvs_lru_t** ptr) {
  for (int i = 0; i < (*ptr)->size; ++i) {
    free((*ptr)->keys[i]);
    free((*ptr)->values[i]); // free each value
  }
  free((*ptr)->keys);
  free((*ptr)->values); // free the values array
  free(*ptr);
  *ptr = NULL;
}

int kvs_lru_set(kvs_lru_t* kvs_lru, const char* key, const char* value) {
  for (int i = 0; i < kvs_lru->size; ++i) {
    if (strcmp(kvs_lru->keys[i], key) == 0) {
      free(kvs_lru->values[i]); // free the old value
      kvs_lru->values[i] = strdup(value); // set new value
      // move the key and value to the end of the array to denote it as the most recently used
      char* temp_key = kvs_lru->keys[i];
      char* temp_value = kvs_lru->values[i];
      memmove(kvs_lru->keys + i, kvs_lru->keys + i + 1, (kvs_lru->size - i - 1) * sizeof(char*));
      memmove(kvs_lru->values + i, kvs_lru->values + i + 1, (kvs_lru->size - i - 1) * sizeof(char*));
      kvs_lru->keys[kvs_lru->size - 1] = temp_key;
      kvs_lru->values[kvs_lru->size - 1] = temp_value;
      return 0; // ket was found in cache, so no need to set it in kvs_base
    }
  }

  // if key is not found in the cache
  if (kvs_lru->size == kvs_lru->capacity) {
    free(kvs_lru->keys[0]);
    free(kvs_lru->values[0]); // free the old value
    memmove(kvs_lru->keys, kvs_lru->keys + 1, (kvs_lru->size - 1) * sizeof(char*));
    memmove(kvs_lru->values, kvs_lru->values + 1, (kvs_lru->size - 1) * sizeof(char*));
    --kvs_lru->size;
  }
  kvs_lru->keys[kvs_lru->size] = strdup(key);
  kvs_lru->values[kvs_lru->size] = strdup(value); // store value
  ++kvs_lru->size;

  return kvs_base_set(kvs_lru->kvs_base, key, value); // key was not found in cache, so set it in kvs_base
}
// TA said that set is only called when you are doing a replacement in the cache!

int kvs_lru_get(kvs_lru_t* kvs_lru, const char* key, char* value) {
  for (int i = 0; i < kvs_lru->size; ++i) {
    if (strcmp(kvs_lru->keys[i], key) == 0) {
      strcpy(value, kvs_lru->values[i]); // return the value from the cache
      // move the key and value to the end of the arrsy to denote it as the most recently used
      char* temp_key = kvs_lru->keys[i];
      char* temp_value = kvs_lru->values[i];
      memmove(kvs_lru->keys + i, kvs_lru->keys + i + 1, (kvs_lru->size - i - 1) * sizeof(char*));
      memmove(kvs_lru->values + i, kvs_lru->values + i + 1, (kvs_lru->size - i - 1) * sizeof(char*));
      kvs_lru->keys[kvs_lru->size - 1] = temp_key;
      kvs_lru->values[kvs_lru->size - 1] = temp_value;
      return 0;
    }
  }

  // if key is not found in the cache
  int rc = kvs_base_get(kvs_lru->kvs_base, key, value); 
  if (rc == 0) { // if key is found in the underlying disk store
    if (kvs_lru->size == kvs_lru->capacity) {
      free(kvs_lru->keys[0]);
      free(kvs_lru->values[0]); // free the old value
      memmove(kvs_lru->keys, kvs_lru->keys + 1, (kvs_lru->size - 1) * sizeof(char*));
      memmove(kvs_lru->values, kvs_lru->values + 1, (kvs_lru->size - 1) * sizeof(char*));
      --kvs_lru->size;
    }
    kvs_lru->keys[kvs_lru->size] = strdup(key);
    kvs_lru->values[kvs_lru->size] = strdup(value); // store the value in the cache
    ++kvs_lru->size;
  }

  return rc;
}

int kvs_lru_flush(kvs_lru_t* kvs_lru) {
  for (int i = 0; i < kvs_lru->size; ++i) {
    free(kvs_lru->keys[i]);
    free(kvs_lru->values[i]); // free each value
  }
  kvs_lru->size = 0;
  return 0;
}
