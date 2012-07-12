/*************************************************************************************/
/*      Copyright 2012 Barcelona Supercomputing Center                               */
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


#ifndef _NANOS_BIT_COUNTER_DECL
#define _NANOS_BIT_COUNTER_DECL

namespace nanos
{
   template<typename T, int BITS = sizeof(T)*8>
   class BitCounter
   {
      public:
         static bool __attribute__((always_inline)) hasMoreThanOneOne(T value);
   };


   template<typename T>
   class BitCounter<T, 2>
   {
      public:
         static bool __attribute__((always_inline)) hasMoreThanOneOne(T value);
   
   };

} // namespace nanos

#endif // _NANOS_BIT_COUNTER_DECL
