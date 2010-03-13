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

#include "schedule.hpp"
#include "processingelement.hpp"
#include "basethread.hpp"
#include "system.hpp"

using namespace nanos;

void Scheduler::submit ( WD &wd )
{
   // TODO: increase ready count

   sys._taskNum++;

   debug ( "submitting task " << wd.getId() );
   WD *next = myThread->getTeam()->getSchedulePolicy().atSubmit( myThread, wd );

   if ( next ) {
      switchTo ( next );
   }
}

template<class behaviour>
static inline void idleLoop ()
{
   for ( ; ; ) {
      BaseThread *thread = getMyThreadSafe();

      if ( !thread->isRunning() ) break;

      if ( thread->getTeam() != NULL ) {

         WD *current = thread->getCurrentWD();
         WD * next = behaviour::getWD(thread,current);

         if (next) {
           behaviour::switchWD(thread,current, next);
         }
      }
   }
}

void Scheduler::waitOnCondition (GenericSyncCond *condition)
{
   int spins=100; // FIXME: this has to be configurable (see #147)

   myThread->getCurrentWD()->setSyncCond( condition );

   while ( !condition->check() ) {
      BaseThread *thread = getMyThreadSafe();
      WD * current = thread->getCurrentWD();
      current->setIdle();

      spins--;
      if ( spins == 0 ) {
         condition->lock();
         if ( !( condition->check() ) ) {
            condition->addWaiter( current );

            WD *next = thread->getTeam()->getSchedulePolicy().atBlock( thread, current );

            if ( next ) {
               sys._numReady--;
            }

            if ( next ) {
               switchTo ( next );
            }
            else {
               condition->unlock();
               thread->yield();
            }
         } else {
            condition->unlock();
         }
         spins = 100;
      }
   }
   myThread->getCurrentWD()->setReady();
   myThread->getCurrentWD()->setSyncCond( NULL );
}

void Scheduler::wakeUp ( WD *wd )
{
  if ( wd->isBlocked() ) {
    wd->setReady();
    Scheduler::queue( *wd );
  }
}

void Scheduler::exitHelper (WD *oldWD, WD *newWD, void *arg)
{
    myThread->exitHelperDependent(oldWD, newWD, arg);
    sys.getInstrumentor()->wdExit( oldWD, newWD );
    delete oldWD;
    myThread->setCurrentWD( *newWD );
}

struct ExitBehaviour
{
   static WD * getWD ( BaseThread *thread, WD *current )
   {
      return thread->getTeam()->getSchedulePolicy().atExit( thread, current );
   }

   static void switchWD ( BaseThread *thread, WD *current, WD *next )
   {
      Scheduler::exitTo(next);
   }
};

void Scheduler::exitTo ( WD *next )
 {
    WD *current = myThread->getCurrentWD();

    sys._numReady--;
    sys._numTasksRunning++;

     if (!next->started()) next->start(true,current);
     myThread->exitTo ( next, Scheduler::exitHelper );
}

void Scheduler::exit ( void )
{
   // TODO: Support WD running on lended stack
   // Cases:
   // The WD was running on its own stack, switch to a new one
   // The WD was running on a thread stack, exit to the loop

   // At this point the WD work is done, so we mark it as such and look for other work to do
   // Deallocation doesn't happen here because:
   // a) We are still running in the WD stack
   // b) Resources can potentially be reused by the next WD
   WD *oldwd = myThread->getCurrentWD();
   oldwd->done();
   sys._taskNum--;

   idleLoop<ExitBehaviour>();

   fatal("A thread should never return from Scheduler::exit");
}

struct WorkerBehaviour
{
   static WD * getWD ( BaseThread *thread, WD *current )
   {
      return thread->getTeam()->getSchedulePolicy().atIdle ( thread );
   }

   static void switchWD ( BaseThread *thread, WD *current, WD *next )
   {
// FIX stats
//                   sys._numReady--;
//             sys._idleThreads--;
//             sys._numTasksRunning++;

      if (next->started())
        Scheduler::switchTo(next);
      else
        Scheduler::inlineWork ( next );

//             sys._numTasksRunning--;
//             sys._idleThreads++;

   }
};

void Scheduler::idle ()
{
   sys.getInstrumentor()->enterIdle();

   // This function is run always by the same BaseThread so we don't need to use getMyThreadSafe
   BaseThread *thread = myThread;

   thread->getCurrentWD()->setIdle();

   sys._idleThreads++;

   idleLoop<WorkerBehaviour>();

   thread->getCurrentWD()->setReady();

   sys._idleThreads--;

   verbose ( "Working thread finishing" );
   sys.getInstrumentor()->leaveIdle();
}

void Scheduler::queue ( WD &wd )
{
      myThread->getTeam()->getSchedulePolicy().queue( myThread, wd );
      sys._numReady++;
}

void SchedulingGroup::init ( int groupSize )
{
   _group.reserve ( groupSize );
}

void SchedulingGroup::addMember ( BaseThread &thread )
{
   SchedulingData *data = createMemberData ( thread );

   data->setSchId ( getSize() );
   thread.setScheduling ( this, data );
   _group.push_back( data );
}

void SchedulingGroup::removeMember ( BaseThread &thread )
{
//TODO
}

void SchedulingGroup::queueIdle ( BaseThread *thread, WD &wd )
{
   _idleQueue.push_back ( &wd );
}

void Scheduler::switchHelper (WD *oldWD, WD *newWD, void *arg)
{
   GenericSyncCond *syncCond = oldWD->getSyncCond();
   if ( syncCond != NULL ) {
      oldWD->setBlocked();
      syncCond->unlock();
   } else {
      Scheduler::queue( *oldWD );
   }
   myThread->switchHelperDependent(oldWD, newWD, arg);
   
   sys.getInstrumentor()->wdSwitch( oldWD, newWD );
   myThread->setCurrentWD( *newWD );
}

void Scheduler::inlineWork ( WD *wd )
{
   // run it in the current frame
   WD *oldwd = myThread->getCurrentWD();

   GenericSyncCond *syncCond = oldwd->getSyncCond();
   if ( syncCond != NULL ) {
      syncCond->unlock();
   }

   debug( "switching(inlined) from task " << oldwd << ":" << oldwd->getId() <<
          " to " << wd << ":" << wd->getId() );

   sys.getInstrumentor()->wdSwitch( oldwd, wd );

   // This ensures that when we return from the inlining is still the same thread
   // and we don't violate rules about tied WD
   wd->tieTo(*oldwd->isTiedTo());
   wd->start(false);
   myThread->setCurrentWD( *wd );
   myThread->inlineWorkDependent(*wd);
   wd->done();

   debug( "exiting task(inlined) " << wd << ":" << wd->getId() <<
          " to " << oldwd << ":" << oldwd->getId() );

   sys.getInstrumentor()->wdSwitch( wd, oldwd );

   BaseThread *thread = getMyThreadSafe();
   thread->setCurrentWD( *oldwd );

   // While we tie the inlined tasks this is not needed
   // as we will always return to the current thread
   #if 0
   if ( oldwd->isTiedTo() != NULL )
      switchToThread(oldwd->isTiedTo());
   #endif

   ensure(oldwd->isTiedTo() != NULL || oldwd->isTiedTo() == thread,"Violating tied rules");
}

void Scheduler::switchTo ( WD *to )
{
   if ( myThread->runningOn()->supportsUserLevelThreads() ) {
      if (!to->started())
        to->start(true);

      myThread->switchTo( to, switchHelper );
   } else {
      inlineWork(to);
      delete to;
   }
}

void Scheduler::yield ()
{
   WD *next = myThread->getTeam()->getSchedulePolicy().atYield( myThread, myThread->getCurrentWD() );

   if ( next ) {
        sys._numTasksRunning++;
        switchTo(next);
   }
}

void Scheduler::switchToThread ( BaseThread *thread )
{
   while ( getMyThreadSafe() != thread )
        yield();
}

