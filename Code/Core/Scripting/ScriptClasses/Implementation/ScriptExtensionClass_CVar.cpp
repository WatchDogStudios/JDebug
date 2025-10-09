#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_CVar.h>

#include <Foundation/Configuration/CVar.h>

// clang-format off
NS_BEGIN_STATIC_REFLECTED_TYPE(nsScriptExtensionClass_CVar, nsNoBase, 1, nsRTTINoAllocator)
{
  NS_BEGIN_FUNCTIONS
  {
    NS_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name")->AddFlags(nsPropertyFlags::PureFunction),
    NS_SCRIPT_FUNCTION_PROPERTY(GetBoolValue, In, "Name")->AddFlags(nsPropertyFlags::PureFunction),
    NS_SCRIPT_FUNCTION_PROPERTY(GetIntValue, In, "Name")->AddFlags(nsPropertyFlags::PureFunction),
    NS_SCRIPT_FUNCTION_PROPERTY(GetFloatValue, In, "Name")->AddFlags(nsPropertyFlags::PureFunction),
    NS_SCRIPT_FUNCTION_PROPERTY(GetStringValue, In, "Name")->AddFlags(nsPropertyFlags::PureFunction),
    NS_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Name", In, "Value"),
    NS_SCRIPT_FUNCTION_PROPERTY(SetBoolValue, In, "Name", In, "Value"),
    NS_SCRIPT_FUNCTION_PROPERTY(SetIntValue, In, "Name", In, "Value"),
    NS_SCRIPT_FUNCTION_PROPERTY(SetFloatValue, In, "Name", In, "Value"),
    NS_SCRIPT_FUNCTION_PROPERTY(SetStringValue, In, "Name", In, "Value"),
  }
  NS_END_FUNCTIONS;
  NS_BEGIN_ATTRIBUTES
  {
    new nsScriptExtensionAttribute("CVar"),
  }
  NS_END_ATTRIBUTES;
}
NS_END_STATIC_REFLECTED_TYPE;
// clang-format on

static nsHashTable<nsTempHashedString, nsCVar*> s_CachedCVars;

static nsCVar* FindCVarByNameCached(nsStringView sName)
{
  nsTempHashedString sNameHashed(sName);

  nsCVar* pCVar = nullptr;
  if (!s_CachedCVars.TryGetValue(sNameHashed, pCVar))
  {
    pCVar = nsCVar::FindCVarByName(sName);

    s_CachedCVars.Insert(sNameHashed, pCVar);
  }

  nsCVar::s_AllCVarEvents.AddEventHandler(
    [&](const nsCVarEvent& e)
    {
      if (e.m_EventType == nsCVarEvent::Type::ListOfVarsChanged)
      {
        s_CachedCVars.Clear();
      }
    });

  return pCVar;
}

// static
nsVariant nsScriptExtensionClass_CVar::GetValue(nsStringView sName)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr)
  {
    return {};
  }

  switch (pCVar->GetType())
  {
    case nsCVarType::Bool:
      return static_cast<nsCVarBool*>(pCVar)->GetValue();
    case nsCVarType::Int:
      return static_cast<nsCVarInt*>(pCVar)->GetValue();
    case nsCVarType::Float:
      return static_cast<nsCVarFloat*>(pCVar)->GetValue();
    case nsCVarType::String:
      return static_cast<nsCVarString*>(pCVar)->GetValue();

    default:
      NS_ASSERT_NOT_IMPLEMENTED;
  }

  return {};
}

// static
bool nsScriptExtensionClass_CVar::GetBoolValue(nsStringView sName)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != nsCVarType::Bool)
  {
    nsLog::Error("CVar '{}' does not exist or is not of type bool.", sName);
    return false;
  }

  return static_cast<nsCVarBool*>(pCVar)->GetValue();
}

// static
int nsScriptExtensionClass_CVar::GetIntValue(nsStringView sName)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != nsCVarType::Int)
  {
    nsLog::Error("CVar '{}' does not exist or is not of type int.", sName);
    return 0;
  }

  return static_cast<nsCVarInt*>(pCVar)->GetValue();
}

// static
float nsScriptExtensionClass_CVar::GetFloatValue(nsStringView sName)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != nsCVarType::Float)
  {
    nsLog::Error("CVar '{}' does not exist or is not of type float.", sName);
    return 0;
  }

  return static_cast<nsCVarFloat*>(pCVar)->GetValue();
}

// static
nsString nsScriptExtensionClass_CVar::GetStringValue(nsStringView sName)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != nsCVarType::String)
  {
    nsLog::Error("CVar '{}' does not exist or is not of type string.", sName);
    return "";
  }

  return static_cast<nsCVarString*>(pCVar)->GetValue();
}

// static
void nsScriptExtensionClass_CVar::SetValue(nsStringView sName, const nsVariant& value)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr)
  {
    nsLog::Error("CVar '{}' does not exist.", sName);
    return;
  }

  switch (pCVar->GetType())
  {
    case nsCVarType::Bool:
    {
      nsCVarBool* pVar = static_cast<nsCVarBool*>(pCVar);
      *pVar = value.ConvertTo<bool>();
      break;
    }

    case nsCVarType::Int:
    {
      nsCVarInt* pVar = static_cast<nsCVarInt*>(pCVar);
      *pVar = value.ConvertTo<int>();
      break;
    }

    case nsCVarType::Float:
    {
      nsCVarFloat* pVar = static_cast<nsCVarFloat*>(pCVar);
      *pVar = value.ConvertTo<float>();
      break;
    }

    case nsCVarType::String:
    {
      nsCVarString* pVar = static_cast<nsCVarString*>(pCVar);
      *pVar = value.ConvertTo<nsString>();
      break;
    }

    default:
      NS_ASSERT_NOT_IMPLEMENTED;
  }
}

// static
void nsScriptExtensionClass_CVar::SetBoolValue(nsStringView sName, bool bValue)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != nsCVarType::Bool)
  {
    nsLog::Error("CVar '{}' does not exist or is not of type bool.", sName);
    return;
  }

  nsCVarBool* pVar = static_cast<nsCVarBool*>(pCVar);
  *pVar = bValue;
}

// static
void nsScriptExtensionClass_CVar::SetIntValue(nsStringView sName, int iValue)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != nsCVarType::Int)
  {
    nsLog::Error("CVar '{}' does not exist or is not of type int.", sName);
    return;
  }

  nsCVarInt* pVar = static_cast<nsCVarInt*>(pCVar);
  *pVar = iValue;
}

// static
void nsScriptExtensionClass_CVar::SetFloatValue(nsStringView sName, float fValue)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != nsCVarType::Float)
  {
    nsLog::Error("CVar '{}' does not exist or is not of type float.", sName);
    return;
  }

  nsCVarFloat* pVar = static_cast<nsCVarFloat*>(pCVar);
  *pVar = fValue;
}

// static
void nsScriptExtensionClass_CVar::SetStringValue(nsStringView sName, const nsString& sValue)
{
  nsCVar* pCVar = FindCVarByNameCached(sName);
  if (pCVar == nullptr || pCVar->GetType() != nsCVarType::String)
  {
    nsLog::Error("CVar '{}' does not exist or is not of type string.", sName);
    return;
  }

  nsCVarString* pVar = static_cast<nsCVarString*>(pCVar);
  *pVar = sValue;
}


NS_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_CVar);
