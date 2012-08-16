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

#include <iostream>
#include <cstring>
#include "simpleallocator.hpp"
#include "atomic.hpp"
#include "system.hpp"

using namespace nanos;

SimpleAllocator::SimpleAllocator( uint64_t baseAddress, size_t len ) : _baseAddress( baseAddress )
{
   _freeChunks[ baseAddress ] = len;
}

void SimpleAllocator::init( uint64_t baseAddress, size_t len )
{
   _baseAddress = baseAddress;
   _freeChunks[ baseAddress ] = len;
}

void * SimpleAllocator::allocate( size_t size )
{
   SegmentMap::iterator mapIter = _freeChunks.begin();
   void * retAddr = (void *) 0;

   size_t alignedLen;
   unsigned int count = 0;
   while ( (size >> count) != 1 ) count++;
   alignedLen = (1UL<<(count));
   //fprintf(stderr, "alignedLen %lX, size %lX\n", alignedLen, size);

   //while( mapIter != _freeChunks.end() && mapIter->second < size )
   while( mapIter != _freeChunks.end() && mapIter->second < ( ( mapIter->first & ~(alignedLen-1) ) + ( ( ( mapIter->first & (alignedLen-1) ) == 0 ) ? 0 : alignedLen ) + size ) - mapIter->first )
   {
      //std::cerr << "this chunk addr " << (void *) mapIter->first << " computed size is " <<  (void *) (mapIter->first | ((size-1))) << " " << ( ((mapIter->first | ((size-1))) + 1 + size ) - mapIter->first ) << std::endl;
      mapIter++;
   }
   if ( mapIter != _freeChunks.end() ) {

      //uint64_t targetAddr = mapIter->first;
      uint64_t chunkAddr = mapIter->first;
      size_t chunkSize = mapIter->second;
      //std::size_t realSize = ( ((mapIter->first | (size-1)) + 1 + size ) - mapIter->first ); //aligned
      uint64_t targetAddr = ( mapIter->first & ~(alignedLen-1) ) + ( ( ( mapIter->first & (alignedLen-1) ) == 0 ) ? 0 : alignedLen ) ;
      //fprintf(stderr, "addr (chunk) is %lX, aligned (target) is %lX\n", mapIter->first, targetAddr );

      //_freeChunks.erase( mapIter );

      //add the chunk with the new size (previous size - requested size)
      //if (chunkSize > size)
         //_freeChunks[ targetAddr + size ] = chunkSize - size ;
      if (chunkSize > size) {
         if (targetAddr == chunkAddr ) {
            _freeChunks.erase( chunkAddr ); 
         } else { 
            _freeChunks[ mapIter->first ] = ( targetAddr - chunkAddr );
         }
         if ((chunkAddr + chunkSize) - (targetAddr + size) > 0)
            _freeChunks[ targetAddr + size ] = (chunkAddr + chunkSize) - (targetAddr + size) ;
      } else if ( chunkSize == size ) {
         _freeChunks.erase( chunkAddr );
      } else { fprintf(stderr, "Error, this does not make sense!\n"); }
      _allocatedChunks[ targetAddr ] = size;

      retAddr = ( void * ) targetAddr ;
   }
   else {
      // Could not get a chunk of 'size' bytes
      std::cerr << sys.getNetwork()->getNodeNum() << ": WARNING: Allocator is full" << std::endl;
      printMap();
      return NULL;
   }

   return retAddr;
}

size_t SimpleAllocator::free( void *address )
{
   SegmentMap::iterator mapIter = _allocatedChunks.find( ( uint64_t ) address );
   size_t size = mapIter->second;
   std::pair< SegmentMap::iterator, bool > ret;

   _allocatedChunks.erase( mapIter );

   if ( !_freeChunks.empty() ) {
      mapIter = _freeChunks.lower_bound( ( uint64_t ) address );

      //case where address is the highest key, check if it can be merged with the previous chunk
      if ( mapIter == _freeChunks.end() ) {
         mapIter--;
         if ( mapIter->first + mapIter->second == ( uint64_t ) address ) {
            mapIter->second += size;
         } else {
            _freeChunks[ ( uint64_t ) address ] = size;
         }
      }
      //address is not the highest key, check if it can be merged with the previous and next chunks
      else if ( _freeChunks.key_comp()( ( uint64_t ) address, mapIter->first ) ) {
         size_t totalSize = size;
         bool firstMerge = false;

         //check next chunk
         if ( mapIter->first == ( ( uint64_t ) address ) + size ) {
            totalSize += mapIter->second;
            _freeChunks.erase( mapIter );
            ret = _freeChunks.insert( SegmentMap::value_type( ( uint64_t ) address, totalSize ) );
            mapIter = ret.first;
            firstMerge = true;
         }

         //check previous chunk
         if ( _freeChunks.begin() != mapIter )
         {
            mapIter--;
            if ( mapIter->first + mapIter->second == ( uint64_t ) address ) {
               //if totalSize > size then the next chunk was merged
               if ( totalSize > size ) {
                  mapIter->second += totalSize;
                  mapIter++;
                  _freeChunks.erase( mapIter );
               }
               else {
                  mapIter->second += size;
               }
            } else if ( !firstMerge ) {
               _freeChunks[ ( uint64_t ) address ] = size;
            }
         }
         else if ( !firstMerge ) {
            _freeChunks[ ( uint64_t ) address ] = size;
         }
      }
      //duplicate key, error
      else {
         std::cerr << "Duplicate entry in segment map, addr " << address << "." << std::endl;
      }
   }
   else {
      _freeChunks[ ( uint64_t ) address ] = size;
   }

   return size;
}

void SimpleAllocator::printMap()
{
   std::size_t totalAlloc = 0, totalFree = 0;
   std::cerr << (void *) this <<" ALLOCATED CHUNKS" << std::endl;
   for (SegmentMap::iterator it = _allocatedChunks.begin(); it != _allocatedChunks.end(); it++ ) {
      std::cerr << "|... ";
      std::cerr << (void *) it->first << " @ " << (std::size_t)it->second;
      std::cerr << " ...";
      totalAlloc += it->second;
   }
   std::cerr << "| total allocated bytes " << (std::size_t) totalAlloc << std::endl;

   std::cerr << (void *) this <<" FREE CHUNKS" << std::endl;
   for (SegmentMap::iterator it = _freeChunks.begin(); it != _freeChunks.end(); it++ ) {
      std::cerr << "|... ";
      std::cerr << (void *) it->first << " @ " << (std::size_t) it->second;
      std::cerr << " ...";
      totalFree += it->second;
   }
   std::cerr << "| total free bytes "<< (std::size_t) totalFree << std::endl;
}

void SimpleAllocator::lock() {
   _lock.acquire();
}

void SimpleAllocator::unlock() {
   _lock.release();
}

BufferManager::BufferManager( void * address, size_t size )
{
   init(address,size);
}

void BufferManager::init ( void * address, size_t size )
{
   _baseAddress = address;
   _index = 0;
   _size = size;
}

void * BufferManager::allocate ( size_t size )
{
   void * address = ( void * ) ( ( uint64_t ) _baseAddress + _index );
   _index = _index + size;
   return  _size >= _index ? address : NULL;
}

void BufferManager::reset ()
{
   _index = 0;
}

