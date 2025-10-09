#pragma once

#include <Foundation/Basics.h>

/// \brief Low-level platform-independent atomic operations for thread-safe programming
///
/// Provides atomic (indivisible) operations that are faster than mutexes for simple operations
/// but slower than regular operations. Use only when thread safety is required.
///
/// Important considerations:
/// - Individual operations are atomic, but sequences of operations are not
/// - Only use in code that requires thread safety - atomic ops have performance overhead
/// - For higher-level usage, prefer nsAtomicInteger which wraps these utilities
/// - All operations use lock-free hardware instructions where available
///
/// These functions form the foundation for lock-free data structures and algorithms.
struct NS_FOUNDATION_DLL nsAtomicUtils
{
  /// \brief Atomically reads a 32-bit integer value
  ///
  /// Ensures the read operation is atomic and not subject to partial reads on all platforms.
  static nsInt32 Read(const nsInt32& iSrc); // [tested]

  /// \brief Atomically reads a 64-bit integer value
  ///
  /// Ensures the read operation is atomic and not subject to partial reads on all platforms.
  static nsInt64 Read(const nsInt64& iSrc); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static nsInt32 Increment(nsInt32& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the new value.
  static nsInt64 Increment(nsInt64& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static nsInt32 Decrement(nsInt32& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the new value.
  static nsInt64 Decrement(nsInt64& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static nsInt32 PostIncrement(nsInt32& ref_iDest); // [tested]

  /// \brief Increments dest as an atomic operation and returns the old value.
  static nsInt64 PostIncrement(nsInt64& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static nsInt32 PostDecrement(nsInt32& ref_iDest); // [tested]

  /// \brief Decrements dest as an atomic operation and returns the old value.
  static nsInt64 PostDecrement(nsInt64& ref_iDest); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(nsInt32& ref_iDest, nsInt32 value); // [tested]

  /// \brief Adds value to dest as an atomic operation.
  static void Add(nsInt64& ref_iDest, nsInt64 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(nsInt32& ref_iDest, nsInt32 value); // [tested]

  /// \brief Performs an atomic bitwise AND on dest using value.
  static void And(nsInt64& ref_iDest, nsInt64 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(nsInt32& ref_iDest, nsInt32 value); // [tested]

  /// \brief Performs an atomic bitwise OR on dest using value.
  static void Or(nsInt64& ref_iDest, nsInt64 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(nsInt32& ref_iDest, nsInt32 value); // [tested]

  /// \brief Performs an atomic bitwise XOR on dest using value.
  static void Xor(nsInt64& ref_iDest, nsInt64 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(nsInt32& ref_iDest, nsInt32 value); // [tested]

  /// \brief Performs an atomic min operation on dest using value.
  static void Min(nsInt64& ref_iDest, nsInt64 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(nsInt32& ref_iDest, nsInt32 value); // [tested]

  /// \brief Performs an atomic max operation on dest using value.
  static void Max(nsInt64& ref_iDest, nsInt64 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static nsInt32 Set(nsInt32& ref_iDest, nsInt32 value); // [tested]

  /// \brief Sets dest to value as an atomic operation and returns the original value of dest.
  static nsInt64 Set(nsInt64& ref_iDest, nsInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(nsInt32& ref_iDest, nsInt32 iExpected, nsInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(nsInt64& ref_iDest, nsInt64 iExpected, nsInt64 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value* and returns true. Otherwise *dest* will not be modified and the
  /// function returns false.
  static bool TestAndSet(void** pDest, void* pExpected, void* value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static nsInt32 CompareAndSwap(nsInt32& ref_iDest, nsInt32 iExpected, nsInt32 value); // [tested]

  /// \brief If *dest* is equal to *expected*, this function sets *dest* to *value*. Otherwise *dest* will not be modified. Always returns the value
  /// of *dest* before the modification.
  static nsInt64 CompareAndSwap(nsInt64& ref_iDest, nsInt64 iExpected, nsInt64 value); // [tested]
};

// include platforma specific implementation
#include <AtomicUtils_Platform.h>
