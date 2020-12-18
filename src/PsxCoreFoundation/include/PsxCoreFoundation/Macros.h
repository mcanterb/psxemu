#pragma once
#define GET_ARG_COUNT(...)                                                     \
  INTERNAL_GET_ARG_COUNT_PRIVATE(                                              \
      0, ##__VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58,    \
      57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40,  \
      39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22,  \
      21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2,  \
      1, 0)
#define INTERNAL_GET_ARG_COUNT_PRIVATE(                                        \
    _0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_,   \
    _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_,    \
    _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, \
    _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, \
    _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, \
    _69, _70, count, ...)                                                      \
  count

#define EXPAND(...) __VA_ARGS__
#define GLUE2(a, b) a##b
#define GLUE3(a, b, c) a##b##c

#define PCF_PANIC(msg, ...)                                                    \
  do {                                                                         \
    char result1[256];                                                         \
    char result2[1024];                                                        \
    int numBytes = sprintf_s(result1, 256, msg, __VA_ARGS__);                  \
    if (numBytes < 0) {                                                        \
      PCFPanic("PANIC: Failed to format error message");                       \
    }                                                                          \
    numBytes =                                                                 \
        sprintf_s(result2, 1024, "PANIC: %s\n Occurred %s:%s @ line %d",       \
                  result1, __FILE__, __func__, __LINE__);                      \
    if (numBytes < 0) {                                                        \
      PCFPanic("PANIC: Failed to format error message");                       \
    }                                                                          \
    PCFPanic(result2);                                                         \
  } while (0)

#define PCF_VTABLE9(name, o1k, o1v, o2k, o2v, o3k, o3v, o4k, o4v)              \
  PCFBaseVTable *name##VTable() {                                              \
    static PCFBaseVTable k##name##VTable = {};                                 \
    if (k##name##VTable.destructor == NULL) {                                  \
      k##name##VTable = kPCFObjectVTable;                                      \
      k##name##VTable.o1k = o1v;                                               \
      k##name##VTable.o2k = o2v;                                               \
      k##name##VTable.o3k = o3v;                                               \
      k##name##VTable.o4k = o4v;                                               \
    }                                                                          \
    return &k##name##VTable;                                                   \
  }

#define PCF_VTABLE7(name, o1k, o1v, o2k, o2v, o3k, o3v)                        \
  PCFBaseVTable *name##VTable() {                                              \
    static PCFBaseVTable k##name##VTable = {};                                 \
    if (k##name##VTable.destructor == NULL) {                                  \
      k##name##VTable = kPCFObjectVTable;                                      \
      k##name##VTable.o1k = o1v;                                               \
      k##name##VTable.o2k = o2v;                                               \
      k##name##VTable.o3k = o3v;                                               \
    }                                                                          \
    return &k##name##VTable;                                                   \
  }

#define PCF_VTABLE5(name, o1k, o1v, o2k, o2v)                                  \
  PCFBaseVTable *name##VTable() {                                              \
    static PCFBaseVTable k##name##VTable = {};                                 \
    if (k##name##VTable.destructor == NULL) {                                  \
      k##name##VTable = kPCFObjectVTable;                                      \
      k##name##VTable.o1k = o1v;                                               \
      k##name##VTable.o2k = o2v;                                               \
    }                                                                          \
    return &k##name##VTable;                                                   \
  }

#define PCF_VTABLE3(name, o1k, o1v)                                            \
  PCFBaseVTable *GLUE2(name, VTable)() {                                       \
    static PCFBaseVTable GLUE3(k, name, VTable) = {};                          \
    if (GLUE3(k, name, VTable).destructor == NULL) {                           \
      GLUE3(k, name, VTable) = kPCFObjectVTable;                               \
      GLUE3(k, name, VTable).o1k = o1v;                                        \
    }                                                                          \
    return &GLUE3(k, name, VTable);                                            \
  }

#define PCF_VTABLE1(name)                                                      \
  PCFBaseVTable *GLUE2(name, VTable)() {                                       \
    static PCFBaseVTable GLUE3(k, name, VTable) = {};                          \
    if (GLUE3(k, name, VTable).destructor == NULL) {                           \
      GLUE3(k, name, VTable) = kPCFObjectVTable;                               \
    }                                                                          \
    return &GLUE3(k, name, VTable);                                            \
  }

#define PCF_VTABLE(name, overrides)                                            \
  PCFBaseVTable *name##VTable() {                                              \
    static PCFBaseVTable table = {};                                           \
    if (table.destructor == NULL) {                                            \
      table = kPCFObjectVTable;                                                \
      overrides                                                                \
    }                                                                          \
    return &table;                                                             \
  }

#define RESULT_TYPE(name, type)                                                \
  typedef struct __##name##Result {                                            \
    bool successful;                                                           \
    union {                                                                    \
      PCFStringRef error;                                                      \
      type result;                                                             \
    } resultOrError;                                                           \
  } name##Result;                                                              \
                                                                               \
  inline type name##ResultOrPanic(name##Result result) {                       \
    if (result.successful) {                                                   \
      return result.resultOrError.result;                                      \
    }                                                                          \
    PCF_PANIC("Failed to get result: %s",                                      \
              PCFStringToCString(result.resultOrError.error));                 \
    PCFRelease(result.resultOrError.error);                                    \
    return result.resultOrError.result;                                        \
  }                                                                            \
                                                                               \
  inline name##Result name##ResultSuccess(type value) {                        \
    name##Result result = {.successful = true};                                \
    result.resultOrError.result = value;                                       \
    return result;                                                             \
  }                                                                            \
                                                                               \
  inline name##Result name##ResultError(PCFStringRef error) {                  \
    name##Result result = {.successful = false};                               \
    result.resultOrError.error = error;                                        \
    return result;                                                             \
  }

#ifdef NDEBUG
#define assert(statement) ((void)0)

#else
#define assert(statement)                                                      \
  if (!(statement)) {                                                          \
    PCF_PANIC("Assertion Failed: " #statement);                                \
  }
#endif
