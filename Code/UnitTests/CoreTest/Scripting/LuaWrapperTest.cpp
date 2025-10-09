#include <CoreTest/CoreTestPCH.h>

#include <Core/Scripting/LuaWrapper.h>

NS_CREATE_SIMPLE_TEST_GROUP(Scripting);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

static const char* g_Script = "\
globaltable = true;\n\
function f_globaltable()\n\
end\n\
function f_NotWorking()\n\
  DoNothing();\n\
end\n\
intvar1 = 4;\n\
intvar2 = 7;\n\
floatvar1 = 4.3;\n\
floatvar2 = 7.3;\n\
boolvar1 = true;\n\
boolvar2 = false;\n\
stringvar1 = \"zweiundvierzig\";\n\
stringvar2 = \"OhWhatsInHere\";\n\
\n\
\n\
function f1()\n\
end\n\
\n\
function f2()\n\
end\n\
\n\
MyTable =\n\
{\n\
  table1 = true;\n\
  \n\
  f_table1 = function()\n\
  end;\n\
  intvar1 = 14;\n\
  intvar2 = 17;\n\
  floatvar1 = 14.3;\n\
  floatvar2 = 17.3;\n\
  boolvar1 = false;\n\
  boolvar2 = true;\n\
  stringvar1 = \"+zweiundvierzig\";\n\
  stringvar2 = \"+OhWhatsInHere\";\n\
  \n\
  SubTable =\n\
  {\n\
    table2 = true;\n\
    f_table2 = function()\n\
    end;\n\
    intvar1 = 24;\n\
  };\n\
};\n\
\n\
";

class ScriptLog : public nsLogInterface
{
public:
  virtual void HandleLogMessage(const nsLoggingEventData& le) override
  {
    NS_TEST_FAILURE("Script Error", le.m_sText);
    NS_TEST_DEBUG_BREAK;
  }
};

class ScriptLogIgnore : public nsLogInterface
{
public:
  static nsInt32 g_iErrors;

  virtual void HandleLogMessage(const nsLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case nsLogMsgType::ErrorMsg:
      case nsLogMsgType::SeriousWarningMsg:
      case nsLogMsgType::WarningMsg:
        ++g_iErrors;
      default:
        break;
    }
  }
};

nsInt32 ScriptLogIgnore::g_iErrors = 0;

int MyFunc1(lua_State* pState)
{
  nsLuaWrapper s(pState);

  NS_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  return s.ReturnToScript();
}

int MyFunc2(lua_State* pState)
{
  nsLuaWrapper s(pState);

  NS_TEST_INT(s.GetNumberOfFunctionParameters(), 6);
  NS_TEST_BOOL(s.IsParameterBool(0));
  NS_TEST_BOOL(s.IsParameterFloat(1));
  NS_TEST_BOOL(s.IsParameterInt(2));
  NS_TEST_BOOL(s.IsParameterNil(3));
  NS_TEST_BOOL(s.IsParameterString(4));
  NS_TEST_BOOL(s.IsParameterString(5));

  NS_TEST_BOOL(s.GetBoolParameter(0) == true);
  NS_TEST_FLOAT(s.GetFloatParameter(1), 2.3f, 0.0001f);
  NS_TEST_INT(s.GetIntParameter(2), 42);
  NS_TEST_STRING(s.GetStringParameter(4), "test");
  NS_TEST_STRING(s.GetStringParameter(5), "tut");

  return s.ReturnToScript();
}

int MyFunc3(lua_State* pState)
{
  nsLuaWrapper s(pState);

  NS_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  s.PushReturnValue(false);
  s.PushReturnValue(2.3f);
  s.PushReturnValue(42);
  s.PushReturnValueNil();
  s.PushReturnValue("test");
  s.PushReturnValue("tuttut", 3);

  return s.ReturnToScript();
}

int MyFunc4(lua_State* pState)
{
  nsLuaWrapper s(pState);

  NS_TEST_INT(s.GetNumberOfFunctionParameters(), 1);

  NS_TEST_BOOL(s.IsParameterTable(0));

  NS_TEST_BOOL(s.OpenTableFromParameter(0) == NS_SUCCESS);

  NS_TEST_BOOL(s.IsVariableAvailable("table1") == true);

  s.CloseAllTables();

  return s.ReturnToScript();
}

NS_CREATE_SIMPLE_TEST(Scripting, LuaWrapper)
{
  ScriptLog Log;
  ScriptLogIgnore LogIgnore;

  nsLuaWrapper sMain;
  NS_TEST_BOOL(sMain.ExecuteString(g_Script, "MainScript", &Log) == NS_SUCCESS);

  NS_TEST_BLOCK(nsTestBlock::Enabled, "ExecuteString")
  {
    nsLuaWrapper s;
    ScriptLogIgnore::g_iErrors = 0;

    NS_TEST_BOOL(s.ExecuteString(" pups ", "FailToCompile", &LogIgnore) == NS_FAILURE);
    NS_TEST_INT(ScriptLogIgnore::g_iErrors, 1);

    NS_TEST_BOOL(s.ExecuteString(" pups(); ", "FailToExecute", &LogIgnore) == NS_FAILURE);
    NS_TEST_INT(ScriptLogIgnore::g_iErrors, 2);

    NS_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == NS_SUCCESS);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Clear")
  {
    nsLuaWrapper s;
    NS_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == NS_SUCCESS);

    NS_TEST_BOOL(s.IsVariableAvailable("globaltable") == true);
    NS_TEST_BOOL(s.IsVariableAvailable("boolvar1") == true);
    NS_TEST_BOOL(s.IsVariableAvailable("boolvar2") == true);
    NS_TEST_BOOL(s.IsVariableAvailable("intvar1") == true);
    NS_TEST_BOOL(s.IsVariableAvailable("intvar2") == true);
    NS_TEST_BOOL(s.IsVariableAvailable("floatvar1") == true);
    NS_TEST_BOOL(s.IsVariableAvailable("floatvar2") == true);
    NS_TEST_BOOL(s.IsVariableAvailable("stringvar1") == true);
    NS_TEST_BOOL(s.IsVariableAvailable("stringvar2") == true);

    s.Clear();

    // after clearing the script, these variables should not be available anymore
    NS_TEST_BOOL(s.IsVariableAvailable("globaltable") == false);
    NS_TEST_BOOL(s.IsVariableAvailable("boolvar1") == false);
    NS_TEST_BOOL(s.IsVariableAvailable("boolvar2") == false);
    NS_TEST_BOOL(s.IsVariableAvailable("intvar1") == false);
    NS_TEST_BOOL(s.IsVariableAvailable("intvar2") == false);
    NS_TEST_BOOL(s.IsVariableAvailable("floatvar1") == false);
    NS_TEST_BOOL(s.IsVariableAvailable("floatvar2") == false);
    NS_TEST_BOOL(s.IsVariableAvailable("stringvar1") == false);
    NS_TEST_BOOL(s.IsVariableAvailable("stringvar2") == false);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "IsVariableAvailable (Global)")
  {
    NS_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    NS_TEST_BOOL(sMain.IsVariableAvailable("nonexisting1") == false);
    NS_TEST_BOOL(sMain.IsVariableAvailable("boolvar1") == true);
    NS_TEST_BOOL(sMain.IsVariableAvailable("boolvar2") == true);
    NS_TEST_BOOL(sMain.IsVariableAvailable("nonexisting2") == false);
    NS_TEST_BOOL(sMain.IsVariableAvailable("intvar1") == true);
    NS_TEST_BOOL(sMain.IsVariableAvailable("intvar2") == true);
    NS_TEST_BOOL(sMain.IsVariableAvailable("nonexisting3") == false);
    NS_TEST_BOOL(sMain.IsVariableAvailable("floatvar1") == true);
    NS_TEST_BOOL(sMain.IsVariableAvailable("floatvar2") == true);
    NS_TEST_BOOL(sMain.IsVariableAvailable("nonexisting4") == false);
    NS_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == true);
    NS_TEST_BOOL(sMain.IsVariableAvailable("stringvar2") == true);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "IsFunctionAvailable (Global)")
  {
    NS_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting1") == false);
    NS_TEST_BOOL(sMain.IsFunctionAvailable("f1") == true);
    NS_TEST_BOOL(sMain.IsFunctionAvailable("f2") == true);
    NS_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting2") == false);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetIntVariable (Global)")
  {
    NS_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    NS_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
    NS_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    NS_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetIntVariable (Table)")
  {
    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);

    NS_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    NS_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);
    NS_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    NS_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);

    sMain.CloseTable();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetFloatVariable (Global)")
  {
    NS_TEST_FLOAT(sMain.GetFloatVariable("nonexisting1", 13), 13, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 7.3f, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("nonexisting2", 14), 14, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 7.3f, nsMath::DefaultEpsilon<float>());
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetFloatVariable (Table)")
  {
    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);

    NS_TEST_FLOAT(sMain.GetFloatVariable("nonexisting1", 13), 13, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 17.3f, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("nonexisting2", 14), 14, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, nsMath::DefaultEpsilon<float>());
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 17.3f, nsMath::DefaultEpsilon<float>());

    sMain.CloseTable();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetBoolVariable (Global)")
  {
    NS_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    NS_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    NS_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
    NS_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    NS_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    NS_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetBoolVariable (Table)")
  {
    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);

    NS_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    NS_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    NS_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);
    NS_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    NS_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    NS_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);

    sMain.CloseTable();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetStringVariable (Global)")
  {
    NS_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");
    NS_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetStringVariable (Table)")
  {
    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);

    NS_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");
    NS_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");

    sMain.CloseTable();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (int, Global)")
  {
    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    sMain.SetVariable("intvar1", 27);
    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 27);
    sMain.SetVariable("intvar1", 4);
    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (int, Table)")
  {
    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);

    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    sMain.SetVariable("intvar1", 127);
    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 127);
    sMain.SetVariable("intvar1", 14);
    NS_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);

    sMain.CloseTable();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (float, Global)")
  {
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, nsMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 27.3f);
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 27.3f, nsMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 4.3f);
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, nsMath::DefaultEpsilon<float>());
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (float, Table)")
  {
    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);

    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, nsMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 127.3f);
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 127.3f, nsMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 14.3f);
    NS_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, nsMath::DefaultEpsilon<float>());

    sMain.CloseTable();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (bool, Global)")
  {
    NS_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    NS_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    NS_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (bool, Table)")
  {
    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);

    NS_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    NS_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    NS_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);

    sMain.CloseTable();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (string, Global)")
  {
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "test1");
    sMain.SetVariable("stringvar1", "zweiundvierzig");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1", 3);
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "tes");
    sMain.SetVariable("stringvar1", "zweiundvierzigabc", 14);
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (string, Table)")
  {
    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);

    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+test1");
    sMain.SetVariable("stringvar1", "+zweiundvierzig");
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1", 4);
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+tes");
    sMain.SetVariable("stringvar1", "+zweiundvierzigabc", 15);
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.CloseTable();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (nil, Global)")
  {
    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    NS_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "zweiundvierzig");
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetVariable (nil, Table)")
  {
    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);

    NS_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    NS_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "+zweiundvierzig");

    sMain.CloseTable();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "OpenTable")
  {
    NS_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    NS_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
    NS_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

    NS_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == true);
    NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
    NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

    NS_TEST_BOOL(sMain.OpenTable("NotMyTable") == NS_FAILURE);

    NS_TEST_BOOL(sMain.OpenTable("MyTable") == NS_SUCCESS);
    {
      NS_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      NS_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      NS_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      NS_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      NS_TEST_BOOL(sMain.OpenTable("NotMyTable") == NS_FAILURE);

      NS_TEST_BOOL(sMain.OpenTable("SubTable") == NS_SUCCESS);
      {
        NS_TEST_BOOL(sMain.OpenTable("NotMyTable") == NS_FAILURE);

        NS_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        NS_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        NS_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        NS_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      NS_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      NS_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      NS_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      NS_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      NS_TEST_BOOL(sMain.OpenTable("NotMyTable") == NS_FAILURE);

      NS_TEST_BOOL(sMain.OpenTable("SubTable") == NS_SUCCESS);
      {
        NS_TEST_BOOL(sMain.OpenTable("NotMyTable") == NS_FAILURE);

        NS_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        NS_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        NS_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        NS_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        NS_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      sMain.CloseTable();
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "RegisterCFunction")
  {
    NS_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == false);

    sMain.RegisterCFunction("Func1", MyFunc1);

    NS_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == true);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Call Lua Function")
  {
    NS_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    NS_TEST_BOOL(sMain.PrepareFunctionCall("f_globaltable") == true);
    NS_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == NS_SUCCESS);

    ScriptLogIgnore::g_iErrors = 0;
    NS_TEST_BOOL(sMain.PrepareFunctionCall("f_NotWorking") == true);
    NS_TEST_BOOL(sMain.CallPreparedFunction(0, &LogIgnore) == NS_FAILURE);
    NS_TEST_INT(ScriptLogIgnore::g_iErrors, 1);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Call C Function")
  {
    NS_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    if (sMain.IsFunctionAvailable("Func1") == false)
      sMain.RegisterCFunction("Func1", MyFunc1);

    NS_TEST_BOOL(sMain.PrepareFunctionCall("Func1") == true);

    NS_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == NS_SUCCESS);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Call C Function with Parameters")
  {
    if (sMain.IsFunctionAvailable("Func2") == false)
      sMain.RegisterCFunction("Func2", MyFunc2);

    NS_TEST_BOOL(sMain.PrepareFunctionCall("Func2") == true);

    sMain.PushParameter(true);
    sMain.PushParameter(2.3f);
    sMain.PushParameter(42);
    sMain.PushParameterNil();
    sMain.PushParameter("test");
    sMain.PushParameter("tuttut", 3);

    NS_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == NS_SUCCESS);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Call C Function with Return Values")
  {
    if (sMain.IsFunctionAvailable("Func3") == false)
      sMain.RegisterCFunction("Func3", MyFunc3);

    NS_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    NS_TEST_BOOL(sMain.CallPreparedFunction(6, &Log) == NS_SUCCESS);

    NS_TEST_BOOL(sMain.IsReturnValueBool(0));
    NS_TEST_BOOL(sMain.IsReturnValueFloat(1));
    NS_TEST_BOOL(sMain.IsReturnValueInt(2));
    NS_TEST_BOOL(sMain.IsReturnValueNil(3));
    NS_TEST_BOOL(sMain.IsReturnValueString(4));
    NS_TEST_BOOL(sMain.IsReturnValueString(5));

    NS_TEST_BOOL(sMain.GetBoolReturnValue(0) == false);
    NS_TEST_FLOAT(sMain.GetFloatReturnValue(1), 2.3f, 0.0001f);
    NS_TEST_INT(sMain.GetIntReturnValue(2), 42);
    NS_TEST_STRING(sMain.GetStringReturnValue(4), "test");
    NS_TEST_STRING(sMain.GetStringReturnValue(5), "tut");

    sMain.DiscardReturnValues();

    NS_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    NS_TEST_BOOL(sMain.CallPreparedFunction(6, &Log) == NS_SUCCESS);

    sMain.DiscardReturnValues();
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Call C Function with Table Parameter")
  {
    if (sMain.IsFunctionAvailable("Func4") == false)
      sMain.RegisterCFunction("Func4", MyFunc4);

    NS_TEST_BOOL(sMain.PrepareFunctionCall("Func4") == true);

    sMain.PushTable("MyTable", true);

    NS_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == NS_SUCCESS);
  }
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
