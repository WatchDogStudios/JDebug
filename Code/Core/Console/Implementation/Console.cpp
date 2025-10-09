#include <Core/CorePCH.h>

#include <Core/Console/Console.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

NS_ENUMERABLE_CLASS_IMPLEMENTATION(nsConsoleFunctionBase);

nsColor nsConsoleString::GetColor() const
{
  switch (m_Type)
  {
    case nsConsoleString::Type::Default:
      return nsColor::White;

    case nsConsoleString::Type::Error:
      return nsColor(1.0f, 0.2f, 0.2f);

    case nsConsoleString::Type::SeriousWarning:
      return nsColor(1.0f, 0.4f, 0.1f);

    case nsConsoleString::Type::Warning:
      return nsColor(1.0f, 0.6f, 0.1f);

    case nsConsoleString::Type::Note:
      return nsColor(1, 200.0f / 255.0f, 0);

    case nsConsoleString::Type::Success:
      return nsColor(0.1f, 1.0f, 0.1f);

    case nsConsoleString::Type::Executed:
      return nsColor(1.0f, 0.5f, 0.0f);

    case nsConsoleString::Type::VarName:
      return nsColorGammaUB(255, 210, 0);

    case nsConsoleString::Type::FuncName:
      return nsColorGammaUB(100, 255, 100);

    case nsConsoleString::Type::Dev:
      return nsColor(0.6f, 0.6f, 0.6f);

    case nsConsoleString::Type::Debug:
      return nsColor(0.4f, 0.6f, 0.8f);

      NS_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return nsColor::White;
}

nsConsole::nsConsole() = default;

nsConsole::~nsConsole()
{
  if (s_pMainConsole == this)
  {
    s_pMainConsole = nullptr;
  }
}

void nsConsole::SetMainConsole(nsConsole* pConsole)
{
  s_pMainConsole = pConsole;
}

nsConsole* nsConsole::GetMainConsole()
{
  return s_pMainConsole;
}

nsConsole* nsConsole::s_pMainConsole = nullptr;

bool nsConsole::AutoComplete(nsStringBuilder& ref_sText)
{
  NS_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    nsCommandInterpreterState s;
    s.m_sInput = ref_sText;

    m_pCommandInterpreter->AutoComplete(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }

    if (!s.m_sOutput.IsEmpty())
    {
      AddConsoleString("", nsConsoleString::Type::Note);
    }

    if (ref_sText != s.m_sInput)
    {
      ref_sText = s.m_sInput;
      return true;
    }
  }

  return false;
}

void nsConsole::ExecuteCommand(nsStringView sInput)
{
  if (sInput.IsEmpty())
    return;

  NS_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    nsCommandInterpreterState s;
    s.m_sInput = sInput;
    m_pCommandInterpreter->Interpret(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }
  }
  else
  {
    AddConsoleString(sInput);
  }
}

void nsConsole::AddConsoleString(nsStringView sText, nsConsoleString::Type type /*= nsConsoleString::Type::Default*/)
{
  nsConsoleString cs;
  cs.m_sText = sText;
  cs.m_Type = type;

  // Broadcast that we have added a string to the console
  nsConsoleEvent e;
  e.m_Type = nsConsoleEvent::Type::OutputLineAdded;
  e.m_AddedpConsoleString = &cs;

  m_Events.Broadcast(e);
}

void nsConsole::AddToInputHistory(nsStringView sText)
{
  NS_LOCK(m_Mutex);

  m_iCurrentInputHistoryElement = -1;

  if (sText.IsEmpty())
    return;

  for (nsInt32 i = 0; i < (nsInt32)m_InputHistory.GetCount(); i++)
  {
    if (m_InputHistory[i] == sText) // already in the History
    {
      // just move it to the front

      for (nsInt32 j = i - 1; j >= 0; j--)
        m_InputHistory[j + 1] = m_InputHistory[j];

      m_InputHistory[0] = sText;
      return;
    }
  }

  m_InputHistory.SetCount(nsMath::Min<nsUInt32>(m_InputHistory.GetCount() + 1, m_InputHistory.GetCapacity()));

  for (nsUInt32 i = m_InputHistory.GetCount() - 1; i > 0; i--)
    m_InputHistory[i] = m_InputHistory[i - 1];

  m_InputHistory[0] = sText;
}

void nsConsole::RetrieveInputHistory(nsInt32 iHistoryUp, nsStringBuilder& ref_sResult)
{
  NS_LOCK(m_Mutex);

  if (m_InputHistory.IsEmpty())
    return;

  m_iCurrentInputHistoryElement = nsMath::Clamp<nsInt32>(m_iCurrentInputHistoryElement + iHistoryUp, 0, m_InputHistory.GetCount() - 1);

  if (!m_InputHistory[m_iCurrentInputHistoryElement].IsEmpty())
  {
    ref_sResult = m_InputHistory[m_iCurrentInputHistoryElement];
  }
}

nsResult nsConsole::SaveInputHistory(nsStringView sFile)
{
  nsFileWriter file;
  NS_SUCCEED_OR_RETURN(file.Open(sFile));

  nsStringBuilder str;

  for (const nsString& line : m_InputHistory)
  {
    if (line.IsEmpty())
      continue;

    str.Set(line, "\n");

    NS_SUCCEED_OR_RETURN(file.WriteBytes(str.GetData(), str.GetElementCount()));
  }

  return NS_SUCCESS;
}

void nsConsole::LoadInputHistory(nsStringView sFile)
{
  nsFileReader file;
  if (file.Open(sFile).Failed())
    return;

  nsStringBuilder str;
  str.ReadAll(file);

  nsHybridArray<nsStringView, 32> lines;
  str.Split(false, lines, "\n", "\r");

  for (nsUInt32 i = 0; i < lines.GetCount(); ++i)
  {
    AddToInputHistory(lines[lines.GetCount() - 1 - i]);
  }
}
