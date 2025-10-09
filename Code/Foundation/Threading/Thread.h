#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/Threading/Implementation/OSThread.h>

// Warning: 'this' used in member initialization list (is fine here since it is just stored and not
// accessed in the constructor (so no operations on a not completely initialized object happen)

NS_WARNING_PUSH()
NS_WARNING_DISABLE_MSVC(4355)

#ifndef NS_THREAD_CLASS_ENTRY_POINT
#  error "Definition for nsThreadClassEntryPoint is missing on this platform!"
#endif

NS_THREAD_CLASS_ENTRY_POINT;

/// \brief Event data for thread lifecycle notifications
struct nsThreadEvent
{
  enum class Type
  {
    ThreadCreated,     ///< Called on the thread that creates the nsThread instance (not the nsThread itself).
    ThreadDestroyed,   ///< Called on the thread that destroys the nsThread instance (not the nsThread itself).
    StartingExecution, ///< Called on the nsThread before the Run() method is executed.
    FinishedExecution, ///< Called on the nsThread after the Run() method was executed.
    ClearThreadLocals, ///< Potentially called on the nsThread (currently only for task system threads) at a time when plugins should clean up thread-local storage.
  };

  Type m_Type;
  nsThread* m_pThread = nullptr;
};

/// \brief This class is the base class for platform independent long running threads
///
/// Used by deriving from this class and overriding the Run() method.
class NS_FOUNDATION_DLL nsThread : public nsOSThread
{
public:
  /// \brief Returns the current nsThread if the current platform thread is an nsThread. Returns nullptr otherwise.
  static const nsThread* GetCurrentThread();

  /// \brief Thread execution state
  enum nsThreadStatus
  {
    Created = 0, ///< Thread created but not yet started
    Running,     ///< Thread is currently executing
    Finished     ///< Thread execution has completed
  };

  /// \brief Creates a new thread with specified name and stack size
  ///
  /// The thread is created in Created state and must be started separately.
  /// Default stack size of 128KB is suitable for most purposes.
  nsThread(nsStringView sName = "nsThread", nsUInt32 uiStackSize = 128 * 1024);

  /// \brief Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~nsThread();

  /// \brief Returns the thread status
  inline nsThreadStatus GetThreadStatus() const { return m_ThreadStatus; }

  /// \brief Helper function to determine if the thread is running
  inline bool IsRunning() const { return m_ThreadStatus == Running; }

  /// \brief Returns the thread name
  inline const char* GetThreadName() const { return m_sName.GetData(); }

  /// \brief Global events for thread lifecycle monitoring
  ///
  /// These events inform about threads starting and finishing. Events are raised on the executing thread,
  /// allowing thread-specific initialization and cleanup code to be executed during callbacks.
  /// Useful for setting up thread-local storage or registering threads with profiling systems.
  static nsEvent<const nsThreadEvent&, nsMutex> s_ThreadEvents;

private:
  /// \brief Pure virtual function that contains the thread's main execution logic
  ///
  /// Override this method to implement the work that the thread should perform.
  /// The return value is passed as the thread exit code and can be retrieved after the thread finishes.
  virtual nsUInt32 Run() = 0;


  volatile nsThreadStatus m_ThreadStatus = Created;

  nsString m_sName;

  friend nsUInt32 RunThread(nsThread* pThread);
};

NS_WARNING_POP()
