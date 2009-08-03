#ifndef _NANOS_H_
#define _NANOS_H_

#include <unistd.h>

#ifdef MCC
// define API version
#pragma nanos interface family(master) version(5000)
#endif

// data types

// C++ types hidden as void *
typedef void * nanos_wd_t;
typedef void * nanos_team_t;
typedef void * nanos_thread_t;

// other types
typedef struct {
   void **address;
   struct {
     bool  input: 1;
     bool  output: 1;
     bool  can_rename:1;
   } flags;
   size_t  size;
} nanos_dependence_t;

typedef struct {
   bool mandatory_creation:1;
   bool tied:1;
   bool reserved0:1;
   bool reserved1:1;
   bool reserved2:1;
   bool reserved3:1;
   bool reserved4:1;
   bool reserved5:1;
   nanos_thread_t * tie_to;
   unsigned int priority;
} nanos_wd_props_t;

typedef struct {
   int nthreads;
   void *arch;
} nanos_restriction_t;

typedef enum { NANOS_OK=0, } nanos_err_t;

typedef struct {
  void * (*factory) (void *arg);
  int  factory_args;
} nanos_device_t;

// Functions related to WD
nanos_err_t nanos_create_wd ( nanos_device_t *devices, nanos_wd_t **wd, size_t data_size,
                              void ** data, nanos_wd_props_t *props );

nanos_err_t nanos_submit ( nanos_wd_t *wd, nanos_dependence_t *deps, nanos_team_t *team );

nanos_err_t nanos_create_wd_and_run ( nanos_device_t *devices, void * data, nanos_dependence_t *deps,
                                      nanos_wd_props_t *props );


#endif
