#include <Foundation/Logging/Log.h>

template <typename T>
T nsObjectAccessorBase::Get(const nsDocumentObject* pObject, const nsAbstractProperty* pProp, nsVariant index /*= nsVariant()*/)
{
  nsVariant value;
  nsStatus res = GetValue(pObject, pProp, value, index);
  if (res.Failed())
    nsLog::Error("GetValue failed: {0}", res.GetMessageString());
  return value.ConvertTo<T>();
}

template <typename T>
T nsObjectAccessorBase::GetByName(const nsDocumentObject* pObject, nsStringView sProp, nsVariant index /*= nsVariant()*/)
{
  nsVariant value;
  nsStatus res = GetValueByName(pObject, sProp, value, index);
  if (res.Failed())
    nsLog::Error("GetValue failed: {0}", res.GetMessageString());
  return value.ConvertTo<T>();
}

inline nsInt32 nsObjectAccessorBase::GetCount(const nsDocumentObject* pObject, const nsAbstractProperty* pProp)
{
  nsInt32 iCount = 0;
  nsStatus res = GetCount(pObject, pProp, iCount);
  if (res.Failed())
    nsLog::Error("GetCount failed: {0}", res.GetMessageString());
  return iCount;
}

inline nsInt32 nsObjectAccessorBase::GetCountByName(const nsDocumentObject* pObject, nsStringView sProp)
{
  nsInt32 iCount = 0;
  nsStatus res = GetCountByName(pObject, sProp, iCount);
  if (res.Failed())
    nsLog::Error("GetCount failed: {0}", res.GetMessageString());
  return iCount;
}
