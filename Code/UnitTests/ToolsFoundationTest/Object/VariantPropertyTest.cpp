#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

static nsHybridArray<nsDocumentObjectPropertyEvent, 2> s_Changes;
void TestPropertyEventHandler(const nsDocumentObjectPropertyEvent& e)
{
  s_Changes.PushBack(e);
}

void TestArray(nsVariantSubAccessor& ref_accessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsDelegate<nsVariant()>& getNativeValue)
{
  auto VerifyChange = [&]()
  {
    // Any operation should collapse to the nsVariant being set as a whole.
    NS_TEST_INT(s_Changes.GetCount(), 1);
    NS_TEST_BOOL(s_Changes[0].m_EventType == nsDocumentObjectPropertyEvent::Type::PropertySet);
    NS_TEST_BOOL(s_Changes[0].m_pObject == pObject);
    NS_TEST_BOOL(s_Changes[0].m_sProperty == pProp->GetPropertyName());
    s_Changes.Clear();
  };

  s_Changes.Clear();
  ref_accessor.StartTransaction("Insert Element");
  nsInt32 iCount = 0;
  nsVariant value = nsColor(1, 2, 3);
  NS_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  NS_TEST_INT(iCount, 0);
  NS_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value, 0));
  NS_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  NS_TEST_INT(iCount, 1);
  NS_TEST_BOOL(getNativeValue()[0] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();
  s_Changes.Clear();

  nsVariant outValue;
  NS_TEST_STATUS(ref_accessor.GetValue(pObject, pProp, outValue, 0));
  NS_TEST_BOOL(value == outValue);

  ref_accessor.StartTransaction("Set Element");
  value = nsVariantDictionary();
  NS_TEST_STATUS(ref_accessor.SetValue(pObject, pProp, value, 0));
  NS_TEST_BOOL(getNativeValue()[0] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Insert Element");
  nsVariant value2 = "Test";
  NS_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value2, 1));
  NS_TEST_BOOL(getNativeValue()[0] == value);
  NS_TEST_BOOL(getNativeValue()[1] == value2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Move Element");
  NS_TEST_STATUS(ref_accessor.MoveValue(pObject, pProp, 1, 0));
  NS_TEST_BOOL(getNativeValue()[0] == value2);
  NS_TEST_BOOL(getNativeValue()[1] == value);
  NS_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  NS_TEST_INT(iCount, 2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Remove Element");
  NS_TEST_STATUS(ref_accessor.RemoveValue(pObject, pProp, 0));
  NS_TEST_BOOL(getNativeValue()[0] == value);
  NS_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  NS_TEST_INT(iCount, 1);
  ref_accessor.FinishTransaction();
  VerifyChange();
}

void TestDictionary(nsVariantSubAccessor& ref_accessor, const nsDocumentObject* pObject, const nsAbstractProperty* pProp, const nsDelegate<nsVariant()>& getNativeValue)
{
  auto VerifyChange = [&]()
  {
    // Any operation should collapse to the nsVariant being set as a whole.
    NS_TEST_INT(s_Changes.GetCount(), 1);
    NS_TEST_BOOL(s_Changes[0].m_EventType == nsDocumentObjectPropertyEvent::Type::PropertySet);
    NS_TEST_BOOL(s_Changes[0].m_pObject == pObject);
    NS_TEST_BOOL(s_Changes[0].m_sProperty == pProp->GetPropertyName());
    s_Changes.Clear();
  };

  s_Changes.Clear();
  ref_accessor.StartTransaction("Insert Element");
  nsInt32 iCount = 0;
  nsVariant value = nsColor(1, 2, 3);
  NS_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  NS_TEST_INT(iCount, 0);
  NS_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value, "A"));
  NS_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  NS_TEST_INT(iCount, 1);
  NS_TEST_BOOL(getNativeValue()["A"] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();
  s_Changes.Clear();

  nsVariant outValue;
  NS_TEST_STATUS(ref_accessor.GetValue(pObject, pProp, outValue, "A"));
  NS_TEST_BOOL(value == outValue);

  ref_accessor.StartTransaction("Set Element");
  value = 42u;
  NS_TEST_STATUS(ref_accessor.SetValue(pObject, pProp, value, "A"));
  NS_TEST_BOOL(getNativeValue()["A"] == value);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Insert Element");
  nsVariant value2 = nsVariantArray();
  NS_TEST_STATUS(ref_accessor.InsertValue(pObject, pProp, value2, "B"));
  NS_TEST_BOOL(getNativeValue()["A"] == value);
  NS_TEST_BOOL(getNativeValue()["B"] == value2);
  ref_accessor.FinishTransaction();
  VerifyChange();

  ref_accessor.StartTransaction("Remove Element");
  NS_TEST_STATUS(ref_accessor.RemoveValue(pObject, pProp, "A"));
  NS_TEST_BOOL(getNativeValue()["B"] == value2);
  NS_TEST_STATUS(ref_accessor.GetCount(pObject, pProp, iCount));
  NS_TEST_INT(iCount, 1);
  ref_accessor.FinishTransaction();
  VerifyChange();
}

NS_CREATE_SIMPLE_TEST(DocumentObject, VariantPropertyTest)
{
  NS_SCOPE_EXIT(s_Changes.Clear(); s_Changes.Compact(););
  nsTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  nsObjectAccessorBase* pAccessor = doc.GetObjectAccessor();
  const nsDocumentObject* pObject = nullptr;
  const nsVariantTestStruct* pNative = nullptr;
  doc.GetObjectManager()->m_PropertyEvents.AddEventHandler(nsMakeDelegate(&TestPropertyEventHandler));
  NS_SCOPE_EXIT(doc.GetObjectManager()->m_PropertyEvents.RemoveEventHandler(nsMakeDelegate(&TestPropertyEventHandler)));

  NS_TEST_BLOCK(nsTestBlock::Enabled, "CreateObject")
  {
    nsUuid objGuid;
    pAccessor->StartTransaction("Add Object");
    NS_TEST_STATUS(pAccessor->AddObject(nullptr, (const nsAbstractProperty*)nullptr, -1, nsGetStaticRTTI<nsVariantTestStruct>(), objGuid));
    pAccessor->FinishTransaction();
    pObject = pAccessor->GetObject(objGuid);
    pNative = static_cast<nsVariantTestStruct*>(doc.m_ObjectMirror.GetNativeObjectPointer(pObject));
  }

  const nsAbstractProperty* pProp = pObject->GetType()->FindPropertyByName("Variant");
  const nsAbstractProperty* pPropArray = pObject->GetType()->FindPropertyByName("VariantArray");
  const nsAbstractProperty* pPropDict = pObject->GetType()->FindPropertyByName("VariantDictionary");

  NS_TEST_BLOCK(nsTestBlock::Enabled, "TestVariant")
  {
    pAccessor->StartTransaction("Set as Array");
    NS_TEST_STATUS(pAccessor->SetValue(pObject, pProp, nsVariantArray()));
    pAccessor->FinishTransaction();
    s_Changes.Clear();

    nsVariantSubAccessor accessor(pAccessor, pProp);
    nsMap<const nsDocumentObject*, nsVariant> subItemMap;
    subItemMap.Insert(pObject, nsVariant());
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pProp, [&]()
      { return pNative->m_Variant; });
    // What remains is an nsVariantDictionary at index 0 that we can recurse into.
    {
      nsVariantSubAccessor accessor2(&accessor, pProp);
      nsMap<const nsDocumentObject*, nsVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pProp, [&]()
        { return pNative->m_Variant[0]; });
    }
    pAccessor->StartTransaction("Set as Dict");
    NS_TEST_STATUS(pAccessor->SetValue(pObject, pProp, nsVariantDictionary()));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    TestDictionary(accessor, pObject, pProp, [&]()
      { return pNative->m_Variant; });
    // What remains is an nsVariantArray at index "B" that we can recurse into.
    {
      nsVariantSubAccessor accessor2(&accessor, pProp);
      nsMap<const nsDocumentObject*, nsVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pProp, [&]()
        { return pNative->m_Variant["B"]; });
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "TestVariantArray")
  {
    pAccessor->StartTransaction("Insert Array");
    NS_TEST_STATUS(pAccessor->InsertValue(pObject, pPropArray, nsVariantArray(), 0));
    pAccessor->FinishTransaction();

    nsVariantSubAccessor accessor(pAccessor, pPropArray);
    nsMap<const nsDocumentObject*, nsVariant> subItemMap;
    subItemMap.Insert(pObject, 0);
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pPropArray, [&]()
      { return pNative->m_VariantArray[0]; });
    // What remains is an nsVariantDictionary at index 0 that we can recurse into.
    {
      nsVariantSubAccessor accessor2(&accessor, pPropArray);
      nsMap<const nsDocumentObject*, nsVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pPropArray, [&]()
        { return pNative->m_VariantArray[0][0]; });
    }
    pAccessor->StartTransaction("Insert Dictionary");
    NS_TEST_STATUS(pAccessor->InsertValue(pObject, pPropArray, nsVariantDictionary(), 1));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    subItemMap.Insert(pObject, 1);
    accessor.SetSubItems(subItemMap);
    TestDictionary(accessor, pObject, pPropArray, [&]()
      { return pNative->m_VariantArray[1]; });
    // What remains is an nsVariantArray at index "B" that we can recurse into.
    {
      nsVariantSubAccessor accessor2(&accessor, pPropArray);
      nsMap<const nsDocumentObject*, nsVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pPropArray, [&]()
        { return pNative->m_VariantArray[1]["B"]; });
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "TestVariantDictionary")
  {
    pAccessor->StartTransaction("Insert Array");
    NS_TEST_STATUS(pAccessor->InsertValue(pObject, pPropDict, nsVariantArray(), "AAA"));
    pAccessor->FinishTransaction();

    nsVariantSubAccessor accessor(pAccessor, pPropDict);
    nsMap<const nsDocumentObject*, nsVariant> subItemMap;
    subItemMap.Insert(pObject, "AAA");
    accessor.SetSubItems(subItemMap);
    TestArray(accessor, pObject, pPropDict, [&]()
      { return *pNative->m_VariantDictionary.GetValue("AAA"); });
    // What remains is an nsVariantDictionary at index 0 that we can recurse into.
    {
      nsVariantSubAccessor accessor2(&accessor, pPropDict);
      nsMap<const nsDocumentObject*, nsVariant> subItemMap2;
      subItemMap2.Insert(pObject, 0);
      accessor2.SetSubItems(subItemMap2);
      TestDictionary(accessor2, pObject, pPropDict, [&]()
        { return (*pNative->m_VariantDictionary.GetValue("AAA"))[0]; });
    }
    pAccessor->StartTransaction("Insert Dictionary");
    NS_TEST_STATUS(pAccessor->InsertValue(pObject, pPropDict, nsVariantDictionary(), "BBB"));
    pAccessor->FinishTransaction();
    s_Changes.Clear();
    subItemMap.Insert(pObject, "BBB");
    accessor.SetSubItems(subItemMap);
    TestDictionary(accessor, pObject, pPropDict, [&]()
      { return *pNative->m_VariantDictionary.GetValue("BBB"); });
    // What remains is an nsVariantArray at index "B" that we can recurse into.
    {
      nsVariantSubAccessor accessor2(&accessor, pPropDict);
      nsMap<const nsDocumentObject*, nsVariant> subItemMap2;
      subItemMap2.Insert(pObject, "B");
      accessor2.SetSubItems(subItemMap2);
      TestArray(accessor2, pObject, pPropDict, [&]()
        { return (*pNative->m_VariantDictionary.GetValue("BBB"))["B"]; });
    }
  }
}
