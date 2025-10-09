#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Strings/HashedString.h>

/// \brief Defines categories and metadata for spatial data used by spatial systems.
///
/// Provides a category system for organizing spatial objects (like render objects, collision objects)
/// that can be used by spatial systems for efficient queries and updates. Categories are registered
/// globally and can have flags to indicate update frequency hints.
struct nsSpatialData
{
  struct Flags
  {
    using StorageType = nsUInt8;

    enum Enum
    {
      None = 0,
      FrequentChanges = NS_BIT(0), ///< Indicates that objects in this category change their bounds frequently. Spatial System implementations can use that as hint for internal optimizations.

      Default = None
    };

    struct Bits
    {
      StorageType FrequentUpdates : 1;
    };
  };

  /// \brief Represents a spatial data category for organizing objects in spatial systems.
  struct Category
  {
    NS_ALWAYS_INLINE Category()
      : m_uiValue(nsSmallInvalidIndex)
    {
    }

    NS_ALWAYS_INLINE explicit Category(nsUInt16 uiValue)
      : m_uiValue(uiValue)
    {
    }

    NS_ALWAYS_INLINE bool operator==(const Category& other) const { return m_uiValue == other.m_uiValue; }
    NS_ALWAYS_INLINE bool operator!=(const Category& other) const { return m_uiValue != other.m_uiValue; }

    nsUInt16 m_uiValue;

    /// \brief Returns the bitmask representation of this category for use in queries.
    NS_ALWAYS_INLINE nsUInt32 GetBitmask() const { return m_uiValue != nsSmallInvalidIndex ? static_cast<nsUInt32>(NS_BIT(m_uiValue)) : 0; }
  };

  /// \brief Registers a spatial data category under the given name.
  ///
  /// If the same category was already registered before, it returns that instead.
  /// Asserts that there are no more than 32 unique categories.
  NS_CORE_DLL static Category RegisterCategory(nsStringView sCategoryName, const nsBitflags<Flags>& flags);

  /// \brief Returns either an existing category with the given name or nsInvalidSpatialDataCategory.
  NS_CORE_DLL static Category FindCategory(nsStringView sCategoryName);

  /// \brief Returns the name of the given category.
  NS_CORE_DLL static const nsHashedString& GetCategoryName(Category category);

  /// \brief Returns the flags for the given category.
  NS_CORE_DLL static const nsBitflags<Flags>& GetCategoryFlags(Category category);

private:
  struct CategoryData
  {
    nsHashedString m_sName;
    nsBitflags<Flags> m_Flags;
  };

  static nsHybridArray<nsSpatialData::CategoryData, 32>& GetCategoryData();
};

/// \brief Predefined spatial data categories commonly used throughout the engine.
struct NS_CORE_DLL nsDefaultSpatialDataCategories
{
  static nsSpatialData::Category RenderStatic;     ///< Static render objects that don't change position frequently
  static nsSpatialData::Category RenderDynamic;    ///< Dynamic render objects that may change position frequently
  static nsSpatialData::Category OcclusionStatic;  ///< Static objects used for occlusion culling
  static nsSpatialData::Category OcclusionDynamic; ///< Dynamic objects used for occlusion culling
};

/// \brief When an object is 'seen' by a view and thus tagged as 'visible', this enum describes what kind of observer triggered this.
///
/// This is used to determine how important certain updates, such as animations, are to execute.
/// E.g. when a 'shadow view' or 'reflection view' is the only thing that observes an object, animations / particle effects and so on,
/// can be updated less frequently.
struct nsVisibilityState
{
  using StorageType = nsUInt8;

  enum Enum : StorageType
  {
    Invisible = 0, ///< The object isn't visible to any view.
    Indirect = 1,  ///< The object is seen by a view that only indirectly makes the object visible (shadow / reflection / render target).
    Direct = 2,    ///< The object is seen directly by a main view and therefore it needs to be updated at maximum frequency.

    Default = Invisible
  };
};

#define nsInvalidSpatialDataCategory nsSpatialData::Category()
