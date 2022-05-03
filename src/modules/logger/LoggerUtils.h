#pragma once
#include "LoggerMath.h"

/**** helper functions for textual translations of state values ****/

// NOTE: size is always passed as safety precaution to not overallocate the target
// sizeof(target) would not work because it's a pointer (always size 4)
// NOTE: consider implementing better error catching for overlong key/value pairs

// formatting patterns
#define PATTERN_IKVSUNT_JSON      "{\"i\":%d,\"k\":\"%s\",\"v\":%s,\"s\":%s,\"u\":\"%s\",\"n\":%d,\"to\":%lu}"
#define PATTERN_IKVSUN_JSON       "{\"i\":%d,\"k\":\"%s\",\"v\":%s,\"s\":%s,\"u\":\"%s\",\"n\":%d}"
#define PATTERN_IKVSUN_SIMPLE     "#%d %s: %s+/-%s%s (%d)"

#define PATTERN_KVSUNT_JSON       "{\"k\":\"%s\",\"v\":%s,\"s\":%s,\"u\":\"%s\",\"n\":%d,\"to\":%lu}"
#define PATTERN_KVSUN_JSON        "{\"k\":\"%s\",\"v\":%s,\"s\":%s,\"u\":\"%s\",\"n\":%d}"
#define PATTERN_KVSUN_SIMPLE      "%s: %s+/-%s%s (%d)"

#define PATTERN_IKVUNT_JSON       "{\"i\":%d,\"k\":\"%s\",\"v\":%s,\"u\":\"%s\",\"n\":%d,\"to\":%lu}"
#define PATTERN_IKVUN_JSON        "{\"i\":%d,\"k\":\"%s\",\"v\":%s,\"u\":\"%s\",\"n\":%d}"
#define PATTERN_IKVUN_SIMPLE      "#%d %s: %s%s (%d)"

#define PATTERN_KVUNT_JSON        "{\"k\":\"%s\",\"v\":%s,\"u\":\"%s\",\"n\":%d,\"to\":%lu}"
#define PATTERN_KVUN_JSON         "{\"k\":\"%s\",\"v\":%s,\"u\":\"%s\",\"n\":%d}"
#define PATTERN_KVUN_JSON_QUOTED  "{\"k\":\"%s\",\"v\":\"%s\",\"u\":\"%s\",\"n\":%d}"
#define PATTERN_KVUN_SIMPLE       "%s: %s%s (%d)"

#define PATTERN_IKVU_JSON         "{\"i\":%d,\"k\":\"%s\",\"v\":%s,\"u\":\"%s\"}"
#define PATTERN_IKVU_JSON_QUOTED  "{\"i\":%d,\"k\":\"%s\",\"v\":\"%s\",\"u\":\"%s\"}"
#define PATTERN_IKVU_SIMPLE       "#%d %s: %s%s"

#define PATTERN_KVU_JSON          "{\"k\":\"%s\",\"v\":%s,\"u\":\"%s\"}"
#define PATTERN_KVU_JSON_QUOTED   "{\"k\":\"%s\",\"v\":\"%s\",\"u\":\"%s\"}"
#define PATTERN_KVU_SIMPLE        "%s: %s%s"

#define PATTERN_IKV_JSON          "{\"i\":%d,\"k\":\"%s\",\"v\":%s}"
#define PATTERN_IKV_JSON_QUOTED   "{\"i\":%d,\"k\":\"%s\",\"v\":\"%s\"}"
#define PATTERN_IKV_SIMPLE        "#%d %s: %s"

#define PATTERN_KV_JSON           "{\"k\":\"%s\",\"v\":%s}"
#define PATTERN_KV_JSON_QUOTED    "{\"k\":\"%s\",\"v\":\"%s\"}"
#define PATTERN_KV_SIMPLE         "%s: %s"

#define PATTERN_VUN_SIMPLE        "%s%s (%d)"
#define PATTERN_VU_SIMPLE         "%s%s"
#define PATTERN_V_SIMPLE          "%s"

/**** GENERAL UTILITY FUNCTIONS ****/

static void getInfoIdxKeyValueSigmaUnitsNumberTimeOffset(char* target, int size, int idx, char* key, char* value, char* sigma, char* units, int n, unsigned long time_offset, const char* pattern = PATTERN_IKVSUNT_JSON) {
  snprintf(target, size, pattern, idx, key, value, sigma, units, n, time_offset);
}

static void getInfoKeyValueSigmaUnitsNumberTimeOffset(char* target, int size, char* key, char* value, char* sigma, char* units, int n, unsigned long time_offset, const char* pattern = PATTERN_IKVSUNT_JSON) {
  snprintf(target, size, pattern, key, value, sigma, units, n, time_offset);
}

static void getInfoIdxKeyValueUnitsNumberTimeOffset(char* target, int size, int idx, char* key, char* value, char* units, int n, unsigned long time_offset, const char* pattern = PATTERN_IKVUNT_JSON) {
  snprintf(target, size, pattern, idx, key, value, units, n, time_offset);
}

static void getInfoKeyValueUnitsNumberTimeOffset(char* target, int size, char* key, char* value, char* units, int n, unsigned long time_offset, const char* pattern = PATTERN_KVUNT_JSON) {
  snprintf(target, size, pattern, key, value, units, n, time_offset);
}

static void getInfoKeyValueUnitsNumber(char* target, int size, char* key, char* value, char* units, int n, const char* pattern = PATTERN_KVUN_SIMPLE) {
  snprintf(target, size, pattern, key, value, units, n);
}

static void getInfoValueUnitsNumber(char* target, int size, char* value, char* units, int n, const char* pattern = PATTERN_VUN_SIMPLE) {
  snprintf(target, size, pattern, value, units, n);
}

static void getInfoIdxKeyValueUnits(char* target, int size, int idx, char* key, char* value, char* units, const char* pattern = PATTERN_IKVU_SIMPLE) {
  snprintf(target, size, pattern, idx, key, value, units);
}

static void getInfoKeyValueUnits(char* target, int size, char* key, char* value, char* units, const char* pattern = PATTERN_KVU_SIMPLE) {
  snprintf(target, size, pattern, key, value, units);
}

static void getInfoIdxKeyValue(char* target, int size, int idx, char* key, char* value, const char* pattern = PATTERN_IKV_SIMPLE) {
  snprintf(target, size, pattern, idx, key, value);
}

static void getInfoKeyValue(char* target, int size, char* key, char* value, const char* pattern = PATTERN_KV_SIMPLE) {
  snprintf(target, size, pattern, key, value);
}

static void getInfoValueUnits(char* target, int size, char* value, char* units, const char* pattern = PATTERN_VU_SIMPLE) {
  snprintf(target, size, pattern, value, units);
}

static void getInfoValue(char* target, int size, char* value, const char* pattern = PATTERN_V_SIMPLE) {
  snprintf(target, size, pattern, value);
}

/**** DATA INFO FUNCTIONS ****/
// Note: whenever idx is negative, it is excluded from the printing

static void getDataDoubleWithSigmaText(int idx, char* key, double value, double sigma, char* units, int n, unsigned long time_offset, char* target, int size, const char* pattern, int decimals) {
  char value_text[20];
  print_to_decimals(value_text, sizeof(value_text), value, decimals);
  char sigma_text[20];
  print_to_decimals(sigma_text, sizeof(sigma_text), sigma, decimals);
  (idx >= 0) ?
    getInfoIdxKeyValueSigmaUnitsNumberTimeOffset(target, size, idx, key, value_text, sigma_text, units, n, time_offset, pattern) :
    getInfoKeyValueSigmaUnitsNumberTimeOffset(target, size, key, value_text, sigma_text, units, n, time_offset, pattern);

}

static void getDataDoubleWithSigmaText(int idx, char* key, double value, double sigma, char* units, int n, char* target, int size, const char* pattern, int decimals) {
  getDataDoubleWithSigmaText(idx, key, value, sigma, units, n, -1, target, size, pattern, decimals);
}

static void getDataDoubleWithSigmaText(char* key, double value, double sigma, char* units, int n, char* target, int size, char* pattern, int decimals) {
  getDataDoubleWithSigmaText(-1, key, value, sigma, units, n, -1, target, size, pattern, decimals);
}

static void getDataDoubleText(int idx, char* key, double value, char* units, int n, unsigned long time_offset, char* target, int size, const char* pattern, int decimals) {
  char value_text[20];
  print_to_decimals(value_text, sizeof(value_text), value, decimals);
  (idx >= 0) ?
    getInfoIdxKeyValueUnitsNumberTimeOffset(target, size, idx, key, value_text, units, n, time_offset, pattern) :
    getInfoKeyValueUnitsNumberTimeOffset(target, size, key, value_text, units, n, time_offset, pattern);
}

static void getDataDoubleText(int idx, char* key, double value, char* units, int n, char* target, int size, const char* pattern, int decimals) {
  getDataDoubleText(idx, key, value, units, n, -1, target, size, pattern, decimals);
}

static void getDataDoubleText(int idx, char* key, double value, char* units, char* target, int size, const char* pattern, int decimals) {
  getDataDoubleText(idx, key, value, units, -1, -1, target, size, pattern, decimals);
}

static void getDataDoubleText(int idx, char* key, double value, char* target, int size, const char* pattern, int decimals) {
  getDataDoubleText(idx, key, value, "", -1, -1, target, size, pattern, decimals);
}

static void getDataDoubleText(char* key, double value, char* units, int n, char* target, int size, const char* pattern, int decimals) {
  getDataDoubleText(-1, key, value, units, n, -1, target, size, pattern, decimals);
}

static void getDataDoubleText(char* key, double value, char* units, char* target, int size, const char* pattern, int decimals) {
  getDataDoubleText(-1, key, value, units, -1, -1, target, size, pattern, decimals);
}

static void getDataDoubleText(char* key, double value, char* target, int size, const char* pattern, int decimals) {
  getDataDoubleText(-1, key, value, "", -1, -1, target, size, pattern, decimals);
}

static void getDataNullText(int idx, char* key, char* target, int size, const char* pattern) {
  char value_text[] = "null";
  (idx >= 0) ?
    getInfoIdxKeyValue(target, size, idx, key, value_text, pattern) :
    getInfoKeyValue(target, size, key, value_text, pattern);
}

static void getDataNullText(char* key, char* target, int size, const char* pattern) {
  getDataNullText(-1, key, target, size, pattern);
}
/**** STATE INFO FUNCTIONS ****/

// helper function to assemble char/string state text
static void getStateStringText(char* key, char* value, char* target, int size, const char* pattern, bool include_key = true) {
  if (include_key)
    getInfoKeyValue(target, size, key, value, pattern);
  else
    getInfoValue(target, size, value, pattern);
}

// helper function to assemble boolean state text
static void getStateBooleanText(char* key, bool value, char* value_true, char* value_false, char* target, int size, const char* pattern, bool include_key = true) {
  char value_text[20];
  value_text[sizeof(value_text) - 1] = 0; // make sure last index is null pointer just to be extra safe
  value ? strncpy(value_text, value_true, sizeof(value_text) - 1) : strncpy(value_text, value_false, sizeof(value_text) - 1);
  if (include_key)
    getInfoKeyValue(target, size, key, value_text, pattern);
  else
    getInfoValue(target, size, value_text, pattern);
}

// helper function to assemble integer state text
static void getStateIntText(char* key, int value, char* units, char* target, int size, const char* pattern, bool include_key = true) {
  char value_text[10];
  snprintf(value_text, sizeof(value_text), "%d", value);
  if (include_key)
    getInfoKeyValueUnits(target, size, key, value_text, units, pattern);
  else
    getInfoValueUnits(target, size, value_text, units, pattern);
}

// helper function to assemble double state text
static void getStateDoubleText(char* key, double value, char* units, char* target, int size, const char* pattern, int decimals, bool include_key = true) {
  char value_text[20];
  print_to_decimals(value_text, sizeof(value_text), value, decimals);
  (include_key) ?
    getInfoKeyValueUnits(target, size, key, value_text, units, pattern) :
    getInfoValueUnits(target, size, value_text, units, pattern);
}
