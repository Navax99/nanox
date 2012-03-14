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

/*
<testinfo>
test_generator=gens/api-generator
</testinfo>
*/

#include <stdio.h>
#include <nanos.h>
#include <alloca.h>


nanos_wd_dyn_props_t dyn_props = {0};

void sleep_100 ( void )
{
   nanos_event_key_t event_key;
   nanos_event_value_t event_value;

   nanos_instrument_get_key ("user-funct-name", &event_key);
   nanos_instrument_register_value ( &event_value, "user-funct-name", "sleep_100", "Function: sleep_100", false );
   nanos_instrument_enter_burst( event_key, event_value );

   usleep ( 100 );
   nanos_yield();
   usleep ( 100 );
   nanos_yield();
   usleep ( 100 );

   nanos_instrument_leave_burst( event_key );
}
// compiler: outlined function arguments
typedef struct {
   int value;
} main__task_1_data_t;

typedef struct {
   int value;
} main__task_2_data_t;

void main__task_2 ( void *args )
{
   nanos_event_key_t event_key;
   nanos_event_value_t event_value;

   nanos_instrument_get_key ("user-funct-name", &event_key);
   nanos_instrument_register_value ( &event_value, "user-funct-name", "task-2", "Function: main__task_2", false );
   nanos_instrument_enter_burst( event_key, event_value );

   main__task_2_data_t *hargs = (main__task_2_data_t * ) args;

   nanos_yield();
   usleep ( hargs->value );
   nanos_yield();

   nanos_instrument_leave_burst( event_key );
}
// compiler: smp device for main__task_2 function
nanos_smp_args_t main__task_2_device_args = { main__task_2 };

/* ************** CONSTANT PARAMETERS IN WD CREATION ******************** */
nanos_const_wd_definition_t const_data2 = 
{
   {
      .mandatory_creation = true,
      .tied = false,
      .priority = 0
   },
   __alignof__(main__task_2_data_t),
   0,
   1,
   {
      {
         nanos_smp_factory,
         &main__task_2_device_args
      }
   }
};

// compiler: outlined function
void main__task_1 ( void *args )
{
   nanos_event_key_t event_key;
   nanos_event_value_t event_value;

   nanos_instrument_get_key ("user-funct-name", &event_key);
   nanos_instrument_register_value ( &event_value, "user-funct-name", "task-1", "Function: main__task_1", false );
   nanos_instrument_enter_burst( event_key, event_value );

   nanos_instrument_disable_state_events( NANOS_RUNTIME );

   int i;
   main__task_1_data_t *hargs = (main__task_1_data_t * ) args;

   usleep ( hargs->value );

   for (i = 0; i < 10; i++ ) {
      nanos_wd_t wd = NULL;
      main__task_2_data_t *task_data = NULL;

      NANOS_SAFE( nanos_create_wd_compact ( &wd, &const_data2, &dyn_props, sizeof( main__task_2_data_t ),
                                    (void **) &task_data, nanos_current_wd(), NULL ));

      task_data->value = 1000;

      NANOS_SAFE( nanos_submit( wd,0,0,0 ) );
   }

   usleep ( hargs->value );
   sleep_100 ();
   usleep ( hargs->value );
   nanos_yield();
   usleep ( hargs->value );

   NANOS_SAFE( nanos_wg_wait_completion( nanos_current_wd(), false ) );

   nanos_instrument_enable_state_events();

   nanos_instrument_leave_burst( event_key );
}
// compiler: smp device for main__task_1 function
nanos_smp_args_t main__task_1_device_args = { main__task_1 };

/* ************** CONSTANT PARAMETERS IN WD CREATION ******************** */
nanos_const_wd_definition_t const_data1 = 
{
   {
      .mandatory_creation = true,
      .tied = false,
      .priority = 0
   },
   __alignof__(main__task_1_data_t),
   0,
   1,
   {
      {
         nanos_smp_factory,
         &main__task_1_device_args
      }
   }
};

int main ( int argc, char **argv )
{

   nanos_wd_t wd = NULL;
   main__task_1_data_t *task_data = NULL;

   NANOS_SAFE( nanos_create_wd_compact ( &wd, &const_data1, &dyn_props, sizeof( main__task_1_data_t ),
                                    (void **) &task_data, nanos_current_wd(), NULL ));

   task_data->value = 100;

   NANOS_SAFE( nanos_submit( wd,0,0,0 ) );

   NANOS_SAFE( nanos_wg_wait_completion( nanos_current_wd(), false ) );

   return 0; 
}

