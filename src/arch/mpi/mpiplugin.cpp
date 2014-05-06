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

#include "plugin.hpp"
#include "mpidd.hpp"
#include "system.hpp"

namespace nanos {
namespace ext {


class MPIPlugin : public ArchPlugin
{
   bool _initialized;
   bool _extraeInitialized;

   public:
      MPIPlugin() : ArchPlugin( "MPI PE Plugin",1 ), _initialized(false), _extraeInitialized(false) {}

      virtual void config ( Config& cfg )
      {
         cfg.setOptionsSection( "Offload Arch", "Offload specific options" );
         MPIProcessor::prepareConfig( cfg );
      }
      
      virtual bool configurable() { 
         char *offload_trace_on = getenv(const_cast<char*> ("NX_OFFLOAD_INSTRUMENTATION"));
          return offload_trace_on == NULL || _extraeInitialized; 
      }
      
      virtual void init() {
         /* if MPITRAE_ON not defined, activate it */
         int provided;
         //MPI Init triggers extrae init
         //If OmpSs has compiled MPI tasks, we assume we are in an offload environment
         //if (sys.getOmpssUsesOffload()!=0){ //doesnt seem to be working...
         char *offload_trace_on = getenv(const_cast<char*> ("NX_OFFLOAD_INSTRUMENTATION"));
         if (offload_trace_on != NULL && !_extraeInitialized){ 
             _extraeInitialized=true;              
             if (getenv("I_MPI_WAIT_MODE")==NULL) putenv( const_cast<char*> ("I_MPI_WAIT_MODE=1"));
             MPI_Init_thread(0, 0, MPI_THREAD_MULTIPLE, &provided);
             return;
         }
        
         if (!_initialized) {
           _initialized=true;
           nanos::ext::MPIRemoteNode::mpiOffloadSlaveMain();
         }
      }
      
      virtual unsigned getNumHelperPEs() const
      {
           return 0;
      }

      virtual unsigned getNumPEs() const
      {
           return 0;
      }

      virtual unsigned getNumThreads() const
      {
           return 0;
      }
      
      
      virtual void createBindingList()
      {
//        /* As we now how many devices we have and how many helper threads we
//         * need, reserve a PE for them */
//        for ( unsigned i = 0; i < OpenCLConfig::getOpenCLDevicesCount(); ++i )
//        {
//           // TODO: if HWLOC is available, use it.
//           int node = sys.getNumSockets() - 1;
//           unsigned pe = sys.reservePE( node );
//           // Now add this node to the binding list
//           addBinding( pe );
//        }
      }

   virtual PE* createPE( unsigned id, unsigned uid )
   {
      //Not used
      return NULL;
   }
};
}
}

DECLARE_PLUGIN("arch-mpi",nanos::ext::MPIPlugin);
