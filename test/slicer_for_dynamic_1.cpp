/*************************************************************************************/
/*      Copyright 2009 Barcelona Supercomputing Center                               */
/*                                                                                   */
/*      This file is part of the NANOS++ library.                                    */
/*                                                                                   */
/*      NANOS++ is free software: you can redistribute it and/or modify              */
/*      it under the terms of the GNU Lesser General Public License as published by  */
/*      the Free Software Foundation, either version 3 of the License, or            */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      NANOS++ is distributed in the hope that it will be useful,                   */
/*      but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/*      GNU Lesser General Public License for more details.                          */
/*                                                                                   */
/*      You should have received a copy of the GNU Lesser General Public License     */
/*      along with NANOS++.  If not, see <http://www.gnu.org/licenses/>.             */
/*************************************************************************************/

#include "config.hpp"
#include "nanos.h"
#include <iostream>
#include "smpprocessor.hpp"
#include "system.hpp"
#include "slicer.hpp"

using namespace std;

using namespace nanos;
using namespace nanos::ext;

#define USE_NANOS      true
#define NUM_ITERS      100
#define VECTOR_SIZE    1000
#define VECTOR_MARGIN  10

#define STEP_ERROR     13

//#define VERBOSE

int *A;

void print_vector();

#define EXECUTE(get_slicer,slicer_data,lower,upper,k_offset,step,chunk)\
   for ( i = 0; i < NUM_ITERS; i++ ) {\
      fprintf(stderr,"iteration %d has started\n",i);\
      _loop_data.offset = -k_offset; \
      WD * wd = new SlicedWD( sys.get_slicer, *new slicer_data(lower+k_offset,upper+k_offset,step,chunk),\
                        new SMPDD( main__loop_1 ), sizeof( _loop_data ),( void * ) &_loop_data );\
      WG *wg = myThread->getCurrentWD();\
      wg->addWork( *wd );\
      sys.submit( *wd );\
      wg->waitCompletation();\
      fprintf(stderr,"iteration %d has finished\n",i);\
      if (step > 0 ) for ( int j = lower+k_offset; j < upper+k_offset; j+= step ) A[j+_loop_data.offset]--; \
      else if ( step < 0 ) for ( int j = lower+k_offset; j > upper+k_offset; j+= step ) A[j+_loop_data.offset]--; \
   }

#define FINALIZE(type,lower,upper,offset,step,chunk)\
   print_vector();\
   if ( I[0] == STEP_ERROR ) { step_error = true; I[0] = 0;}\
   for ( i = 0; i < VECTOR_SIZE+2*VECTOR_MARGIN; i++ )\
      if ( I[i] != 0 ) {\
         if ( (i < VECTOR_MARGIN) || (i > (VECTOR_SIZE + VECTOR_MARGIN))) out_of_range = true;\
         if ( (I[i] != NUM_ITERS) && (I[i] != -NUM_ITERS)) race_condition = true;\
         I[i] = 0; check = false; p_check = false;\
      }\
   fprintf(stderr, "%s, lower (%s), upper (%s), offset (%s), step(%+03d), chunk(%02d): %s %s %s %s\n",\
      type, lower, upper, offset, step, chunk,\
      p_check?"  successful":"unsuccessful",\
      out_of_range?" - Out of Range":"",\
      race_condition?" - Race Condition":"",\
      step_error?" - Step Error":""\
   );\
   p_check = true; out_of_range = false; race_condition = false; step_error = false;


#define TEST(test_type,test_get_slicer,test_slicer_data,test_step,test_chunk)\
   EXECUTE(test_get_slicer, test_slicer_data, 0, VECTOR_SIZE, 0, +test_step, test_chunk);\
   FINALIZE (test_type,"0","+","+0000",+test_step,test_chunk);\
   EXECUTE(test_get_slicer, test_slicer_data, VECTOR_SIZE-1, -1, 0, -test_step, test_chunk);\
   FINALIZE (test_type,"+","0","+0000",-test_step,test_chunk);\
   EXECUTE(test_get_slicer, test_slicer_data, 0, VECTOR_SIZE, -VECTOR_SIZE, +test_step, test_chunk);\
   FINALIZE (test_type,"-","0","-VS  ",+test_step,test_chunk);\
   EXECUTE(test_get_slicer, test_slicer_data, VECTOR_SIZE-1, -1, -VECTOR_SIZE, -test_step, test_chunk);\
   FINALIZE (test_type,"0","-","-VS  ",-test_step,test_chunk);\
   EXECUTE(test_get_slicer, test_slicer_data, 0, VECTOR_SIZE, VECTOR_SIZE/2, +test_step, test_chunk);\
   FINALIZE (test_type,"+","+","+VS/2",+test_step,test_chunk);\
   EXECUTE(test_get_slicer, test_slicer_data, VECTOR_SIZE-1, -1, VECTOR_SIZE/2, -test_step, test_chunk);\
   FINALIZE (test_type,"+","+","+VS/2",-test_step,test_chunk);\
   EXECUTE(test_get_slicer, test_slicer_data, 0, VECTOR_SIZE, -(VECTOR_SIZE/2), +test_step, test_chunk);\
   FINALIZE (test_type,"-","+","-VS/2",+test_step,test_chunk);\
   EXECUTE(test_get_slicer, test_slicer_data, VECTOR_SIZE-1, -1, -(VECTOR_SIZE/2), -test_step, test_chunk);\
   FINALIZE (test_type,"+","-","-VS/2",-test_step,test_chunk);\
   EXECUTE(test_get_slicer, test_slicer_data, 0, VECTOR_SIZE, -(2*VECTOR_SIZE), +test_step, test_chunk);\
   FINALIZE (test_type,"-","-","-VS  ",+test_step,test_chunk);\
   EXECUTE(test_get_slicer, test_slicer_data, VECTOR_SIZE-1, -1, -(2*VECTOR_SIZE), -test_step, test_chunk);\
   FINALIZE (test_type,"-","-","-VS  ",-test_step,test_chunk);


typedef struct {
   nanos_loop_info_t loop_info;
   int offset;
} main__loop_1_data_t;


void main__loop_1 ( void *args )
{
#if 0
   int i;
   main__loop_1_data_t *hargs = (main__loop_1_data_t * ) args;
   if ( hargs->loop_info.step > 0 )
   {
      for ( i = hargs->loop_info.lower; i < hargs->loop_info.upper; i += hargs->loop_info.step) {
         A[i+hargs->offset]++;
      }
   }
   else if ( hargs->loop_info.step < 0 )
   {
      for ( i = hargs->loop_info.lower; i > hargs->loop_info.upper; i += hargs->loop_info.step) {
         A[i+hargs->offset]++;
      }
   }
   else {A[-VECTOR_MARGIN] = STEP_ERROR; }
#endif

}

void print_vector ()
{
#ifdef VERBOSE
   for ( int j = -5; j < 0; j++ ) fprintf(stderr,"%d:",A[j]);
   fprintf(stderr,"[");
   for ( int j = 0; j < VECTOR_SIZE; j++ ) fprintf(stderr,"%d:",A[j]);
   fprintf(stderr,"]");
   for ( int j = VECTOR_SIZE; j < VECTOR_SIZE+5; j++ ) fprintf(stderr,"%d:",A[j]);
   fprintf(stderr,"\n");
#endif
}

int main ( int argc, char **argv )
{
   int i;
   bool check = true; 
   bool p_check = true, out_of_range = false, race_condition = false, step_error= false;
   int I[VECTOR_SIZE+2*VECTOR_MARGIN];
   
   A = &I[VECTOR_MARGIN];

   main__loop_1_data_t _loop_data;

   // initialize vector
   for ( i = 0; i < VECTOR_SIZE+2*VECTOR_MARGIN; i++ ) I[i] = 0;

   TEST("dynamic", getSlicerDynamicFor(), SlicerDataDynamicFor,  1,  1)
   TEST("dynamic", getSlicerDynamicFor(), SlicerDataDynamicFor,  5,  1)
   TEST("dynamic", getSlicerDynamicFor(), SlicerDataDynamicFor, 13,  1)
   TEST("dynamic", getSlicerDynamicFor(), SlicerDataDynamicFor,  1,  5)
   TEST("dynamic", getSlicerDynamicFor(), SlicerDataDynamicFor,  5,  5)
   TEST("dynamic", getSlicerDynamicFor(), SlicerDataDynamicFor, 13,  5)
   TEST("dynamic", getSlicerDynamicFor(), SlicerDataDynamicFor,  1, 13)
   TEST("dynamic", getSlicerDynamicFor(), SlicerDataDynamicFor,  5, 13)
   TEST("dynamic", getSlicerDynamicFor(), SlicerDataDynamicFor, 13, 13)

   // END:
   //fprintf(stderr, "%s : %s\n", argv[0], check ? "  successful" : "unsuccessful");
   if (check) { return 0; } else { return -1; }
}

