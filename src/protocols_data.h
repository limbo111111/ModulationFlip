#pragma once

#include <stdint.h>
#include <stddef.h>

#define RF_FLAG_AM (1u << 0)
#define RF_FLAG_FM (1u << 1)

#define FREQ_FLAG_315   (1u << 0)
#define FREQ_FLAG_433   (1u << 1)
#define FREQ_FLAG_868   (1u << 2)
#define FREQ_FLAG_OTHER (1u << 3)

typedef enum {
    ProtoCategoryCar = 0,
    ProtoCategoryGate = 1,
} ProtoCategory;

typedef struct {
    const char* name;
    const char* encoding;
    const char* rf;
    const char* rf_short;
    const char* code;
    const char* freq;
    const char* freq_short;
    const char* tx;
    ProtoCategory category;
    uint8_t rf_mask;
    uint8_t freq_mask;
} ProtocolEntry;

typedef struct {
    const char* name;
    uint16_t offset;
    uint16_t count;
} BrandEntry;

extern const ProtocolEntry kProtocols[];
extern const size_t kProtocolCount;
extern const size_t kCarProtocolCount;
extern const size_t kGateProtocolCount;

extern const BrandEntry kBrands[];
extern const size_t kBrandCount;
extern const uint8_t kBrandProtocolIndices[];
