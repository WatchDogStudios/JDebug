#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Thread.h>

/// \brief Thread base class enabling cross-thread function call dispatching
///
/// Extends nsThread to provide a message passing mechanism where other threads can schedule
/// function calls to execute within this thread's context. Useful for thread-safe operations
/// that must run on specific threads (e.g., UI updates, OpenGL calls).
///
/// Derived classes must call DispatchQueue() regularly in their Run() method to process
/// queued function calls. The double-buffering design ensures minimal lock contention.
class NS_FOUNDATION_DLL nsThreadWithDispatcher : public nsThread
{
public:
  using DispatchFunction = nsDelegate<void(), 128>;

  /// \brief Initializes the runnable class
  nsThreadWithDispatcher(const char* szName = "nsThreadWithDispatcher", nsUInt32 uiStackSize = 128 * 1024);

  /// \brief Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~nsThreadWithDispatcher();

  /// \brief Use this to enqueue a function call to the given delegate at some later point running in the given thread context.
  void Dispatch(DispatchFunction&& delegate);

protected:
  /// \brief Needs to be called by derived thread implementations to dispatch the function calls.
  void DispatchQueue();

private:
  /// \brief The run function can be used to implement a long running task in a thread in a platform independent way
  virtual nsUInt32 Run() = 0;

  nsDynamicArray<DispatchFunction> m_ActiveQueue;
  nsDynamicArray<DispatchFunction> m_CurrentlyBeingDispatchedQueue;

  nsMutex m_QueueMutex;
};
