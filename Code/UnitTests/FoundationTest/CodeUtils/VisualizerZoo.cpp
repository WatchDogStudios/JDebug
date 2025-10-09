#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/MathExpression.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Containers/StaticRingBuffer.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/VarianceTypes.h>

#include <string>

#ifdef __clang__
#  pragma clang optimize off
#endif

class TestRefCounted : public nsRefCounted
{
public:
  nsUInt32 m_uiDummyMember = 0x42u;
};

// declare bitflags using macro magic
NS_DECLARE_FLAGS(nsUInt32, TestFlags, Bit1, Bit2, Bit3, Bit4);
NS_DEFINE_AS_POD_TYPE(TestFlags::Enum);

struct TestFlagsManual
{
  using StorageType = nsUInt32;

  enum Enum
  {
    Bit1 = NS_BIT(0),
    Bit2 = NS_BIT(1),
    Bit3 = NS_BIT(2),
    Bit4 = NS_BIT(3),
    MultiBits = Bit1 | Bit3,
    Default = 0
  };

  struct Bits
  {
    StorageType Bit1 : 1;
    StorageType Bit2 : 1;
    StorageType Bit3 : 1;
    StorageType Bit4 : 1;
  };
};

NS_DECLARE_FLAGS_OPERATORS(TestFlagsManual);

class ReflectedTest : public nsReflectedClass
{
  NS_ADD_DYNAMIC_REFLECTION(ReflectedTest, nsReflectedClass);

public:
  float u;
  float v;
};

// clang-format off
NS_BEGIN_DYNAMIC_REFLECTED_TYPE(ReflectedTest, 1, nsRTTINoAllocator)
{
  NS_BEGIN_PROPERTIES
  {
    NS_MEMBER_PROPERTY("u", u),
    NS_MEMBER_PROPERTY("v", v),
  }
  NS_END_PROPERTIES;
}
NS_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


NS_CREATE_SIMPLE_TEST(CodeUtils, VisualizerZoo)
{
  struct StuffStruct
  {
    int a;
    float b;
    nsString c;
  };
  NS_TEST_BOOL(true);
  // Strings
  {
    nsString stringEmpty;
    nsString string = u8"こんにちは 世界";
    nsString* stringPtr = &string;
    nsString stringArray[4] = {"AAA", "BBB", "CCC", "DDD"};
    nsStringBuilder stringBuilder = "Test";
    nsStringView stringViewEmpty;
    nsStringView stringView = string.GetSubString(0, 5);
    nsStringIterator stringIteratorEmpty;
    nsStringIterator stringIterator = stringView.GetIteratorFront();
    stringIterator++;
    nsStringReverseIterator stringReverseIteratorEmpty;
    nsStringReverseIterator stringReverseIterator = stringView.GetIteratorBack();
    stringReverseIterator++;

    nsHashedString hashedStringEmpty;
    nsHashedString hashedString = nsMakeHashedString("Test");
    NS_TEST_BOOL(true);
  }

  // Containers
  {
    nsDynamicArray<nsString> dynamicArray;
    dynamicArray.PushBack("Item1");
    dynamicArray.PushBack("Item2");

    nsHybridArray<StuffStruct, 4> hybridArray;
    hybridArray.PushBack({1, 2.0f, "Item3"});
    hybridArray.PushBack({2, 3.0f, "Item4"});

    nsHybridArray<StuffStruct, 1> hybridArray2;
    hybridArray2.PushBack({1, 2.0f, "Item3"});
    hybridArray2.PushBack({2, 3.0f, "Item4"});
    hybridArray2.PushBack({3, 4.0f, "Item5"});

    nsSmallArray<nsString, 66> smallArray;
    smallArray.PushBack("SmallItem1");
    smallArray.PushBack("SmallItem2");

    nsSmallArray<nsString, 2> smallArray2;
    smallArray2.PushBack("SmallItem1");
    smallArray2.PushBack("SmallItem2");
    smallArray2.PushBack("SmallItem3");
    smallArray2.PushBack("SmallItem4");

    nsStaticArray<float, 2> staticArray;
    staticArray.SetCount(2);
    staticArray[0] = 1.0f;
    staticArray[1] = 2.0f;

    nsHashTable<int, StuffStruct> hashTable;
    hashTable.Insert(1, StuffStruct{3, 4.0f, "HashItem1"});
    hashTable.Insert(99, StuffStruct{3, 4.0f, "HashItem1"});

    nsHashSet<nsString> hashSet;
    hashSet.Insert("HashSetItem1");
    hashSet.Insert("HashSetItem2");

    nsList<nsString> list;
    list.PushBack("ListItem1");
    list.PushBack("ListItem2");

    nsDeque<nsString> deque;
    deque.PushBack("DequeItem1");
    deque.PushBack("DequeItem2");
    deque.PushFront("DequeItem0");

    nsMap<int, nsString> map;
    map.Insert(1, "MapItem1");
    map.Insert(2, "MapItem2");

    nsSet<nsStringView> set;
    set.Insert("SetItem1");
    set.Insert("SetItem2");

    nsStaticRingBuffer<StuffStruct, 4> staticRingBuffer;
    staticRingBuffer.PushBack({5, 6.0f, "StaticRingItem1"});
    staticRingBuffer.PushBack({7, 8.0f, "StaticRingItem2"});
    staticRingBuffer.PushBack({9, 10.0f, "StaticRingItem3"});
    staticRingBuffer.PushBack({11, 12.0f, "StaticRingItem4"});
    staticRingBuffer.PopFront();
    staticRingBuffer.PushBack({13, 14.0f, "StaticRingItem5"});

    nsArrayPtr<nsString> arrayPtr = nsMakeArrayPtr(dynamicArray.GetData(), dynamicArray.GetCount());
    nsArrayPtr<StuffStruct> hybridArrayPtr = nsMakeArrayPtr(hybridArray.GetData(), hybridArray.GetCount());
    NS_TEST_BOOL(true);
  }

  // nsVariant
  {
    nsVariant variantInvalid; // Default constructor creates Invalid type
    nsVariant variantBool = nsVariant(true);
    nsVariant variantInt8 = nsVariant(static_cast<nsInt8>(42));
    nsVariant variantUInt8 = nsVariant(static_cast<nsUInt8>(42));
    nsVariant variantInt16 = nsVariant(static_cast<nsInt16>(42));
    nsVariant variantUInt16 = nsVariant(static_cast<nsUInt16>(42));
    nsVariant variantInt32 = nsVariant(static_cast<nsInt32>(42));
    nsVariant variantUInt32 = nsVariant(static_cast<nsUInt32>(42));
    nsVariant variantInt64 = nsVariant(static_cast<nsInt64>(42));
    nsVariant variantUInt64 = nsVariant(static_cast<nsUInt64>(42));
    nsVariant variantFloat = nsVariant(42.0f);
    nsVariant variantDouble = nsVariant(42.0);
    nsVariant variantColor = nsVariant(nsColor(1.0f, 0.5f, 0.25f, 1.0f));
    nsVariant variantVector2 = nsVariant(nsVec2(1.0f, 2.0f));
    nsVariant variantVector3 = nsVariant(nsVec3(1.0f, 2.0f, 3.0f));
    nsVariant variantVector4 = nsVariant(nsVec4(1.0f, 2.0f, 3.0f, 4.0f));
    nsVariant variantVector2I = nsVariant(nsVec2I32(1, 2));
    nsVariant variantVector3I = nsVariant(nsVec3I32(1, 2, 3));
    nsVariant variantVector4I = nsVariant(nsVec4I32(1, 2, 3, 4));
    nsVariant variantVector2U = nsVariant(nsVec2U32(1, 2));
    nsVariant variantVector3U = nsVariant(nsVec3U32(1, 2, 3));
    nsVariant variantVector4U = nsVariant(nsVec4U32(1, 2, 3, 4));
    nsVariant variantQuaternion = nsVariant(nsQuat::MakeIdentity());
    nsVariant variantMatrix3 = nsVariant(nsMat3::MakeIdentity());
    nsVariant variantMatrix4 = nsVariant(nsMat4::MakeIdentity());
    nsVariant variantTransform = nsVariant(nsTransform::MakeIdentity());
    nsVariant variantString = nsVariant("SampleString");
    nsVariant variantStringView = nsVariant(nsStringView("SampleStringView"));

    nsDataBuffer dataBuffer;
    dataBuffer.PushBack(1);
    dataBuffer.PushBack(2);
    dataBuffer.PushBack(3);
    nsVariant variantDataBuffer = nsVariant(dataBuffer);

    nsVariant variantTime = nsVariant(nsTime::Seconds(42.0));
    nsVariant variantUuid = nsVariant(nsUuid::MakeStableUuidFromInt(42));
    nsVariant variantAngle = nsVariant(nsAngle::MakeFromDegree(45.0f));
    nsVariant variantColorGamma = nsVariant(nsColorGammaUB(128, 192, 255, 255));
    nsVariant variantHashedString = nsVariant(nsMakeHashedString("HashedSample"));
    nsVariant variantTempHashedString = nsVariant(nsTempHashedString("TempHashedSample"));

    // Extended types
    nsVariantArray varArray;
    varArray.PushBack(nsVariant("Item1"));
    varArray.PushBack(nsVariant(42));
    nsVariant variantVariantArray = nsVariant(varArray);

    nsVariantDictionary varDict;
    varDict.Insert("Key1", nsVariant("Value1"));
    varDict.Insert("Key2", nsVariant(42));
    nsVariant variantVariantDictionary = nsVariant(varDict);

    ReflectedTest test;
    nsVariant variantTypedPointer(&test);
    nsTypedPointer ptr = {nullptr, nsGetStaticRTTI<ReflectedTest>()};
    nsVariant variantTypedPointerNull = ptr;

    nsVarianceTypeAngle value2(nsAngle::MakeFromRadian(1.57079637f), 0.2f);
    nsVariant variantTypedObject(value2);
    NS_TEST_BOOL(true);
  }

  // Enum
  {
    nsEnum<nsVariantType> enumTest = nsVariantType::Angle;
    nsEnum<nsVariantType> enumArray[3] = {nsVariantType::Int32, nsVariantType::String, nsVariantType::Color};
    nsHybridArray<nsEnum<nsVariantType>, 3> hybridEnumArray;
    hybridEnumArray.PushBack(nsVariantType::Int32);
    hybridEnumArray.PushBack(nsVariantType::String);
    NS_TEST_BOOL(true);
  }

  // Bitflags
  {
    TestFlags::Enum rawBitflags = TestFlags::Bit1;
    TestFlags::Enum rawBitflags2 = (TestFlags::Enum)(TestFlags::Bit1 | TestFlags::Bit2).GetValue();

    nsBitflags<TestFlagsManual> bitflagsEmpty;
    nsBitflags<TestFlagsManual> bitflags;
    bitflags.Add(TestFlagsManual::Bit1);
    bitflags.Add(TestFlagsManual::Bit3);

    nsBitflags<TestFlags> bitflagsArray[2];
    bitflagsArray[0].Add(TestFlags::Bit3);
    bitflagsArray[1].Add(TestFlags::Bit4);

    nsHybridArray<nsBitflags<TestFlags>, 2> hybridBitflagsArray;
    hybridBitflagsArray.PushBack(TestFlags::Bit1 | TestFlags::Bit2);
    hybridBitflagsArray.PushBack(TestFlags::Bit3 | TestFlags::Bit4);
    NS_TEST_BOOL(true);
  }

  // Math
  {
    nsVec2 vec2(1.0f, 2.0f);
    nsVec3 vec3(1.0f, 2.0f, 3.0f);
    nsVec4 vec4(1.0f, 2.0f, 3.0f, 4.0f);
    nsVec2I32 vec2I(1, 2);
    nsVec3I32 vec3I(1, 2, 3);
    nsVec4I32 vec4I(1, 2, 3, 4);
    nsVec2U32 vec2U(1, 2);
    nsVec3U32 vec3U(1, 2, 3);
    nsVec4U32 vec4U(1, 2, 3, 4);
    nsQuat quat = nsQuat::MakeFromAxisAndAngle(nsVec3(0, 1, 0), nsAngle::MakeFromDegree(90.0f));
    nsMat3 mat3 = nsMat3::MakeRotationZ(nsAngle::MakeFromDegree(45.0f));
    nsMat4 mat4 = nsMat4::MakeRotationY(nsAngle::MakeFromDegree(30.0f));
    nsTransform transform(nsVec3(1.0f, 2.0f, 3.0f), quat, nsVec3(1.0f, 1.0f, 1.0f));
    nsAngle angle = nsAngle::MakeFromDegree(60.0f);
    nsPlane plane = nsPlane::MakeFromNormalAndPoint(nsVec3(0, 1, 0), nsVec3(0, 0, 0));

    nsColor color(1.0f, 0.5f, 0.25f, 1.0f);
    nsColorGammaUB colorGamma(128, 192, 255, 255);
    nsColorLinearUB colorLinear(128, 192, 255, 255);
    nsTime time = nsTime::Seconds(42.0);

    nsUuid uuid = nsConversionUtils::ConvertStringToUuid("{ 01234567-89AB-CDEF-0123-456789ABCDEF }");
    nsStringBuilder uuidString;
    nsConversionUtils::ToString(uuid, uuidString);
    NS_TEST_BOOL(true);
  }

  // UniquePtr
  {
    nsUniquePtr<ReflectedTest> uniquePtr = NS_DEFAULT_NEW(ReflectedTest);
    uniquePtr->u = 1.0f;
    uniquePtr->v = 2.0f;

    nsSharedPtr<TestRefCounted> sharedPtr = NS_DEFAULT_NEW(TestRefCounted);
    sharedPtr->m_uiDummyMember = 0x42u;

    TestRefCounted testRef;
    nsScopedRefPointer<TestRefCounted> scopedRefPointer(&testRef);
    NS_TEST_BOOL(true);
  }

  // Mutex
  {
    nsMutex mutex;
    NS_LOCK(mutex);
    NS_TEST_BOOL(true);
  }
}
