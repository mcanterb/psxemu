#include <stdbool.h>
#include <stdint.h>

enum PsxCoreFoundationType { PCFUnknownTypeId, PCFDataTypeId, PCFStringTypeId };
enum PCFLogLevels { PCFLogDebug, PCFLogWarn, PCFLogError };

struct __PCFString;
struct __PCFData;
typedef uint32_t PCFLogLevel;
typedef struct __PCFString *PCFStringRef;
typedef struct __PCFString *PCFMutableStringRef;
typedef struct __PCFData *PCFDataRef;
