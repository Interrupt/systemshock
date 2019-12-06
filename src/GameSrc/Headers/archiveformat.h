#if !defined(ARCHIVEFORMAT_H)
#define ARCHIVEFORMAT_H

#include "res.h"

// A single 32-bit integer. A couple of resources are just a number.
extern const ResourceFormat U32Format;
#define FORMAT_U32 (&U32Format)

// The schedules have fixed resource IDs and do not form part of the main
// archive tables.
extern const ResourceFormat ScheduleFormat;
#define FORMAT_SCHEDULE (&ScheduleFormat)

extern const ResourceFormat ScheduleQueueFormat;
#define FORMAT_SCHEDULE_QUEUE (&ScheduleQueueFormat)

// Master tables for level archives.
#define MAX_LEVEL_INDEX 51 // based from xx02; level resources go up to xx53
extern const ResourceFormat LevelVersion11Format[MAX_LEVEL_INDEX+1];
extern const ResourceFormat LevelVersion12Format[MAX_LEVEL_INDEX+1];

#endif // !defined(ARCHIVEFORMAT_H)
