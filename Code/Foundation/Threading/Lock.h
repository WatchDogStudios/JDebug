#pragma once

/// \brief Manages a lock (e.g. a mutex) and ensures that it is properly released as the lock object goes out of scope.
/// Works with any object that implements Lock() and Unlock() methods (nsMutex, etc.).
/// Use the NS_LOCK macro for convenient scoped locking.
template <typename T>
class nsLock
{
public:
  /// \brief Acquires the lock immediately upon construction
  NS_ALWAYS_INLINE explicit nsLock(T& ref_lock)
    : m_Lock(ref_lock)
  {
    m_Lock.Lock();
  }

  /// \brief Automatically releases the lock when the object is destroyed
  NS_ALWAYS_INLINE ~nsLock() { m_Lock.Unlock(); }

private:
  nsLock();
  nsLock(const nsLock<T>& rhs);
  void operator=(const nsLock<T>& rhs);

  T& m_Lock;
};

/// \brief Convenient macro for creating a scoped lock with automatic type deduction
///
/// Creates an nsLock instance with a unique name based on the source line number.
/// The lock is held for the duration of the current scope. Equivalent to:
/// nsLock<decltype(lock)> variable_name(lock);
///
/// Example: NS_LOCK(myMutex); // Locks myMutex until end of scope
#define NS_LOCK(lock) nsLock<decltype(lock)> NS_PP_CONCAT(l_, NS_SOURCE_LINE)(lock)
