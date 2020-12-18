#pragma once
#include "PsxCoreFoundation/Base.h"

ASSUME_NONNULL_BEGIN

int PCF_vsnprintf(char *_Nullable str, size_t size, const char *format,
                  va_list arg);

ASSUME_NONNULL_END
