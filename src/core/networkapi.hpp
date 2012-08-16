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

#ifndef _NANOX_NETWORK_API
#define _NANOX_NETWORK_API

#include <stdint.h>
#include <vector>
#include "directory_decl.hpp"
#include "regiondirectory_decl.hpp"
#include "simpleallocator_decl.hpp"
#include "workdescriptor_fwd.hpp"

namespace nanos {

   class Network;

   class NetworkAPI
   {
      public:
         virtual void initialize ( Network *net ) = 0;
         virtual void finalize () = 0;
         virtual void poll () = 0;
         virtual void sendExitMsg ( unsigned int dest ) = 0;
         virtual void sendWorkMsg ( unsigned int dest, void ( *work ) ( void * ), unsigned int arg0, unsigned int arg1, unsigned int numPe, std::size_t argSize, char * arg, void ( *xlate ) ( void *, void * ), int arch , void *remoteWdAddr) = 0;
         virtual void sendWorkDoneMsg ( unsigned int dest, void *remoteWdAddr, int peId ) = 0;
         virtual void put ( unsigned int remoteNode, uint64_t remoteAddr, void *localAddr, std::size_t size, unsigned int wdId, WD *wd ) = 0;
         virtual void putStrided1D ( unsigned int remoteNode, uint64_t remoteAddr, void *localAddr, void *localPack, std::size_t size, std::size_t count, std::size_t ld, unsigned int wdId, WD *wd ) = 0;
         virtual void get ( void *localAddr, unsigned int remoteNode, uint64_t remoteAddr, std::size_t size ) = 0;
         virtual void getStrided1D ( void *packedAddr, unsigned int remoteNode, uint64_t remoteTag, uint64_t remoteAddr, std::size_t size, std::size_t count, std::size_t ld, volatile int *req ) = 0;
         virtual void malloc ( unsigned int remoteNode, std::size_t size, void * waitObjAddr) = 0;
         virtual void memFree ( unsigned int remoteNode, void *addr ) = 0;
         virtual void memRealloc ( unsigned int remoteNode, void *oldAddr, std::size_t oldSize, void *newAddr, std::size_t newSize ) = 0;
         virtual void nodeBarrier( void ) = 0;
         //virtual void getNotify ( unsigned int node, uint64_t remoteAddr ) = 0;
         virtual void sendRequestPut( unsigned int dest, uint64_t origAddr, unsigned int dataDest, uint64_t dstAddr, std::size_t len, unsigned int wdId, WD *wd ) = 0;
         virtual void sendRequestPutStrided1D( unsigned int dest, uint64_t origAddr, unsigned int dataDest, uint64_t dstAddr, std::size_t len, std::size_t count, std::size_t ld, unsigned int wdId, WD *wd ) = 0;
         virtual std::size_t getTotalBytes() = 0;
        
         virtual void setMasterDirectory(Directory *d) = 0;
         virtual void setNewMasterDirectory(NewRegionDirectory *d) = 0;
         //virtual void setGpuCache(Cache *_cache) = 0;
         virtual SimpleAllocator *getPackSegment() const = 0;
         virtual std::size_t getMaxGetStridedLen() const = 0;
   };
}

#endif
