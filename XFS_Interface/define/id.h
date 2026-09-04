#ifndef NITCBASE_ID_H
#define NITCBASE_ID_H

/* A record is identified by its block number and slot number */
typedef struct recId {
  int block;
  int slot;
} recId;

typedef struct SearchIndexId {
  int sblock;
  int sindex;
} SearchIndexId;

#endif  // NITCBASE_ID_H