#include <CoreTest/CoreTestPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Memory/MemoryUtils.h>

NS_CREATE_SIMPLE_TEST_GROUP(Input);

static bool operator==(const nsInputActionConfig& lhs, const nsInputActionConfig& rhs)
{
  if (lhs.m_bApplyTimeScaling != rhs.m_bApplyTimeScaling)
    return false;
  if (lhs.m_fFilteredPriority != rhs.m_fFilteredPriority)
    return false;
  if (lhs.m_fFilterXMaxValue != rhs.m_fFilterXMaxValue)
    return false;
  if (lhs.m_fFilterXMinValue != rhs.m_fFilterXMinValue)
    return false;
  if (lhs.m_fFilterYMaxValue != rhs.m_fFilterYMaxValue)
    return false;
  if (lhs.m_fFilterYMinValue != rhs.m_fFilterYMinValue)
    return false;

  if (lhs.m_OnEnterArea != rhs.m_OnEnterArea)
    return false;
  if (lhs.m_OnLeaveArea != rhs.m_OnLeaveArea)
    return false;

  for (int i = 0; i < nsInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    if (lhs.m_sInputSlotTrigger[i] != rhs.m_sInputSlotTrigger[i])
      return false;
    if (lhs.m_fInputSlotScale[i] != rhs.m_fInputSlotScale[i])
      return false;
    if (lhs.m_sFilterByInputSlotX[i] != rhs.m_sFilterByInputSlotX[i])
      return false;
    if (lhs.m_sFilterByInputSlotY[i] != rhs.m_sFilterByInputSlotY[i])
      return false;
  }

  return true;
}

class nsTestInputDevide : public nsInputDevice
{
public:
  void ActivateAll()
  {
    m_InputSlotValues["testdevice_button"] = 0.1f;
    m_InputSlotValues["testdevice_stick"] = 0.2f;
    m_InputSlotValues["testdevice_wheel"] = 0.3f;
    m_InputSlotValues["testdevice_touchpoint"] = 0.4f;
    m_uiLastCharacter = '\42';
  }

private:
  void InitializeDevice() override {}
  void UpdateInputSlotValues() override {}
  void RegisterInputSlots() override
  {
    RegisterInputSlot("testdevice_button", "", nsInputSlotFlags::IsButton);
    RegisterInputSlot("testdevice_stick", "", nsInputSlotFlags::IsAnalogStick);
    RegisterInputSlot("testdevice_wheel", "", nsInputSlotFlags::IsMouseWheel);
    RegisterInputSlot("testdevice_touchpoint", "", nsInputSlotFlags::IsTouchPoint);
  }

  void ResetInputSlotValues() override { m_InputSlotValues.Clear(); }
};

static bool operator!=(const nsInputActionConfig& lhs, const nsInputActionConfig& rhs)
{
  return !(lhs == rhs);
}

NS_CREATE_SIMPLE_TEST(Input, InputManager)
{
  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetInputSlotDisplayName / GetInputSlotDisplayName")
  {
    nsInputManager::SetInputSlotDisplayName("test_slot_1", "Test Slot 1 Name");
    nsInputManager::SetInputSlotDisplayName("test_slot_2", "Test Slot 2 Name");
    nsInputManager::SetInputSlotDisplayName("test_slot_3", "Test Slot 3 Name");
    nsInputManager::SetInputSlotDisplayName("test_slot_4", "Test Slot 4 Name");

    NS_TEST_STRING(nsInputManager::GetInputSlotDisplayName("test_slot_1"), "Test Slot 1 Name");
    NS_TEST_STRING(nsInputManager::GetInputSlotDisplayName("test_slot_2"), "Test Slot 2 Name");
    NS_TEST_STRING(nsInputManager::GetInputSlotDisplayName("test_slot_3"), "Test Slot 3 Name");
    NS_TEST_STRING(nsInputManager::GetInputSlotDisplayName("test_slot_4"), "Test Slot 4 Name");
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetInputSlotDeadZone / GetInputSlotDisplayName")
  {
    nsInputManager::SetInputSlotDeadZone("test_slot_1", 0.1f);
    nsInputManager::SetInputSlotDeadZone("test_slot_2", 0.2f);
    nsInputManager::SetInputSlotDeadZone("test_slot_3", 0.3f);
    nsInputManager::SetInputSlotDeadZone("test_slot_4", 0.4f);

    NS_TEST_FLOAT(nsInputManager::GetInputSlotDeadZone("test_slot_1"), 0.1f, 0.0f);
    NS_TEST_FLOAT(nsInputManager::GetInputSlotDeadZone("test_slot_2"), 0.2f, 0.0f);
    NS_TEST_FLOAT(nsInputManager::GetInputSlotDeadZone("test_slot_3"), 0.3f, 0.0f);
    NS_TEST_FLOAT(nsInputManager::GetInputSlotDeadZone("test_slot_4"), 0.4f, 0.0f);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetInputActionConfig / GetInputActionConfig")
  {
    nsInputActionConfig iac1, iac2;
    iac1.m_bApplyTimeScaling = true;
    iac1.m_fFilteredPriority = 23.0f;
    iac1.m_fInputSlotScale[0] = 2.0f;
    iac1.m_fInputSlotScale[1] = 3.0f;
    iac1.m_fInputSlotScale[2] = 4.0f;
    iac1.m_sInputSlotTrigger[0] = nsInputSlot_Key0;
    iac1.m_sInputSlotTrigger[1] = nsInputSlot_Key1;
    iac1.m_sInputSlotTrigger[2] = nsInputSlot_Key2;

    iac2.m_bApplyTimeScaling = false;
    iac2.m_fFilteredPriority = 42.0f;
    iac2.m_fInputSlotScale[0] = 4.0f;
    iac2.m_fInputSlotScale[1] = 5.0f;
    iac2.m_fInputSlotScale[2] = 6.0f;
    iac2.m_sInputSlotTrigger[0] = nsInputSlot_Key3;
    iac2.m_sInputSlotTrigger[1] = nsInputSlot_Key4;
    iac2.m_sInputSlotTrigger[2] = nsInputSlot_Key5;

    nsInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac1, true);
    nsInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac2, true);

    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);

    nsInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, false);
    nsInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, false);

    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);
    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);

    nsInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, true);
    nsInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, true);

    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_1") != iac1);
    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_2") != iac2);
    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    NS_TEST_BOOL(nsInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);


    nsInputManager::RemoveInputAction("test_inputset", "test_action_1");
    nsInputManager::RemoveInputAction("test_inputset", "test_action_2");
    nsInputManager::RemoveInputAction("test_inputset", "test_action_3");
    nsInputManager::RemoveInputAction("test_inputset", "test_action_4");
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Input Slot State Changes / Dead Zones")
  {
    float f = 0;
    nsInputManager::InjectInputSlotValue("test_slot_1", 0.0f);
    nsInputManager::SetInputSlotDeadZone("test_slot_1", 0.25f);

    // just check the first state
    NS_TEST_BOOL(nsInputManager::GetInputSlotState("test_slot_1", &f) == nsKeyState::Up);
    NS_TEST_FLOAT(f, 0.0f, 0);

    // value is not yet propagated
    nsInputManager::InjectInputSlotValue("test_slot_1", 1.0f);
    NS_TEST_BOOL(nsInputManager::GetInputSlotState("test_slot_1", &f) == nsKeyState::Up);
    NS_TEST_FLOAT(f, 0.0f, 0);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    NS_TEST_BOOL(nsInputManager::GetInputSlotState("test_slot_1", &f) == nsKeyState::Pressed);
    NS_TEST_FLOAT(f, 1.0f, 0);

    nsInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    NS_TEST_BOOL(nsInputManager::GetInputSlotState("test_slot_1", &f) == nsKeyState::Down);
    NS_TEST_FLOAT(f, 0.5f, 0);

    nsInputManager::InjectInputSlotValue("test_slot_1", 0.3f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    NS_TEST_BOOL(nsInputManager::GetInputSlotState("test_slot_1", &f) == nsKeyState::Down);
    NS_TEST_FLOAT(f, 0.3f, 0);

    // below dead zone value
    nsInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    NS_TEST_BOOL(nsInputManager::GetInputSlotState("test_slot_1", &f) == nsKeyState::Released);
    NS_TEST_FLOAT(f, 0.0f, 0);

    nsInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    NS_TEST_BOOL(nsInputManager::GetInputSlotState("test_slot_1", &f) == nsKeyState::Pressed);
    NS_TEST_FLOAT(f, 0.5f, 0);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    NS_TEST_BOOL(nsInputManager::GetInputSlotState("test_slot_1", &f) == nsKeyState::Released);
    NS_TEST_FLOAT(f, 0.0f, 0);

    nsInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    NS_TEST_BOOL(nsInputManager::GetInputSlotState("test_slot_1", &f) == nsKeyState::Up);
    NS_TEST_FLOAT(f, 0.0f, 0);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "SetActionDisplayName / GetActionDisplayName")
  {
    nsInputManager::SetActionDisplayName("test_action_1", "Test Action 1 Name");
    nsInputManager::SetActionDisplayName("test_action_2", "Test Action 2 Name");
    nsInputManager::SetActionDisplayName("test_action_3", "Test Action 3 Name");
    nsInputManager::SetActionDisplayName("test_action_4", "Test Action 4 Name");

    NS_TEST_STRING(nsInputManager::GetActionDisplayName("test_action_0"), "test_action_0");
    NS_TEST_STRING(nsInputManager::GetActionDisplayName("test_action_1"), "Test Action 1 Name");
    NS_TEST_STRING(nsInputManager::GetActionDisplayName("test_action_2"), "Test Action 2 Name");
    NS_TEST_STRING(nsInputManager::GetActionDisplayName("test_action_3"), "Test Action 3 Name");
    NS_TEST_STRING(nsInputManager::GetActionDisplayName("test_action_4"), "Test Action 4 Name");
    NS_TEST_STRING(nsInputManager::GetActionDisplayName("test_action_5"), "test_action_5");
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Input Sets")
  {
    nsInputActionConfig iac;
    nsInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac, true);
    nsInputManager::SetInputActionConfig("test_inputset2", "test_action_2", iac, true);

    nsDynamicArray<nsString> InputSetNames;
    nsInputManager::GetAllInputSets(InputSetNames);

    NS_TEST_INT(InputSetNames.GetCount(), 2);

    NS_TEST_STRING(InputSetNames[0].GetData(), "test_inputset");
    NS_TEST_STRING(InputSetNames[1].GetData(), "test_inputset2");

    nsInputManager::RemoveInputAction("test_inputset", "test_action_1");
    nsInputManager::RemoveInputAction("test_inputset2", "test_action_2");
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetAllInputActions / RemoveInputAction")
  {
    nsHybridArray<nsString, 24> InputActions;

    nsInputManager::GetAllInputActions("test_inputset_3", InputActions);

    NS_TEST_BOOL(InputActions.IsEmpty());

    nsInputActionConfig iac;
    nsInputManager::SetInputActionConfig("test_inputset_3", "test_action_1", iac, true);
    nsInputManager::SetInputActionConfig("test_inputset_3", "test_action_2", iac, true);
    nsInputManager::SetInputActionConfig("test_inputset_3", "test_action_3", iac, true);

    nsInputManager::GetAllInputActions("test_inputset_3", InputActions);

    NS_TEST_INT(InputActions.GetCount(), 3);

    NS_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    NS_TEST_STRING(InputActions[1].GetData(), "test_action_2");
    NS_TEST_STRING(InputActions[2].GetData(), "test_action_3");


    nsInputManager::RemoveInputAction("test_inputset_3", "test_action_2");

    nsInputManager::GetAllInputActions("test_inputset_3", InputActions);

    NS_TEST_INT(InputActions.GetCount(), 2);

    NS_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    NS_TEST_STRING(InputActions[1].GetData(), "test_action_3");

    nsInputManager::RemoveInputAction("test_inputset_3", "test_action_1");
    nsInputManager::RemoveInputAction("test_inputset_3", "test_action_3");

    nsInputManager::GetAllInputActions("test_inputset_3", InputActions);

    NS_TEST_BOOL(InputActions.IsEmpty());
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Input Action State Changes")
  {
    nsInputActionConfig iac;
    iac.m_bApplyTimeScaling = false;
    iac.m_sInputSlotTrigger[0] = "test_input_slot_1";
    iac.m_sInputSlotTrigger[1] = "test_input_slot_2";
    iac.m_sInputSlotTrigger[2] = "test_input_slot_3";

    // bind the three slots to this action
    nsInputManager::SetInputActionConfig("test_inputset", "test_action", iac, true);

    // bind the same three slots to another action
    nsInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac, false);

    // the first slot to trigger the action is bound to it, the other slots can now trigger other actions
    // but not this one anymore
    nsInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);

    float f = 0;
    nsInt8 iSlot = 0;
    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == nsKeyState::Up);
    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == nsKeyState::Up);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == nsKeyState::Pressed);
    NS_TEST_INT(iSlot, 1);
    NS_TEST_FLOAT(f, 1.0f, 0.0f);

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == nsKeyState::Pressed);
    NS_TEST_INT(iSlot, 1);
    NS_TEST_FLOAT(f, 1.0f, 0.0f);

    // inject all three input slots
    nsInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    nsInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);
    nsInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == nsKeyState::Down);
    NS_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    NS_TEST_FLOAT(f, 1.0f, 0.0f);

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == nsKeyState::Down);
    NS_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    NS_TEST_FLOAT(f, 1.0f, 0.0f);

    nsInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    nsInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == nsKeyState::Released);
    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == nsKeyState::Released);

    nsInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    nsInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == nsKeyState::Up);
    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == nsKeyState::Up);

    nsInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == nsKeyState::Pressed);
    NS_TEST_INT(iSlot, 2);
    NS_TEST_FLOAT(f, 1.0f, 0.0f);

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == nsKeyState::Pressed);
    NS_TEST_INT(iSlot, 2);
    NS_TEST_FLOAT(f, 1.0f, 0.0f);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == nsKeyState::Released);
    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == nsKeyState::Released);

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == nsKeyState::Up);
    NS_TEST_BOOL(nsInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == nsKeyState::Up);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetPressedInputSlot")
  {
    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    nsStringView sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::None, nsInputSlotFlags::None);
    NS_TEST_BOOL(sSlot.IsEmpty());

    nsInputManager::InjectInputSlotValue("test_slot", 1.0f);

    sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::None, nsInputSlotFlags::None);
    NS_TEST_STRING(sSlot, "");

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::None, nsInputSlotFlags::None);
    NS_TEST_STRING(sSlot, "test_slot");


    {
      nsTestInputDevide dev;
      dev.ActivateAll();

      nsInputManager::InjectInputSlotValue("test_slot", 1.0f);

      nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

      sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::IsButton, nsInputSlotFlags::None);
      NS_TEST_STRING(sSlot, "testdevice_button");

      sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::IsAnalogStick, nsInputSlotFlags::None);
      NS_TEST_STRING(sSlot, "testdevice_stick");

      sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::IsMouseWheel, nsInputSlotFlags::None);
      NS_TEST_STRING(sSlot, "testdevice_wheel");

      sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::IsTouchPoint, nsInputSlotFlags::None);
      NS_TEST_STRING(sSlot, "testdevice_touchpoint");

      nsInputManager::InjectInputSlotValue("test_slot", 1.0f);

      nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

      sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::IsButton, nsInputSlotFlags::None);
      NS_TEST_STRING(sSlot, "");

      sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::IsAnalogStick, nsInputSlotFlags::None);
      NS_TEST_STRING(sSlot, "");

      sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::IsMouseWheel, nsInputSlotFlags::None);
      NS_TEST_STRING(sSlot, "");

      sSlot = nsInputManager::GetPressedInputSlot(nsInputSlotFlags::IsTouchPoint, nsInputSlotFlags::None);
      NS_TEST_STRING(sSlot, "");
    }
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "LastCharacter")
  {
    nsTestInputDevide dev;
    dev.ActivateAll();

    NS_TEST_BOOL(nsInputManager::RetrieveLastCharacter(true) == '\0');

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    NS_TEST_BOOL(nsInputManager::RetrieveLastCharacter(false) == '\42');
    NS_TEST_BOOL(nsInputManager::RetrieveLastCharacter(true) == '\42');
    NS_TEST_BOOL(nsInputManager::RetrieveLastCharacter(true) == '\0');
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "Time Scaling")
  {
    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    nsInputActionConfig iac;
    iac.m_bApplyTimeScaling = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    nsInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    nsTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    nsInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    NS_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 30.0));
    nsInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    NS_TEST_FLOAT(fVal, 0.1f * (1.0 / 30.0), 0.0001f);


    iac.m_bApplyTimeScaling = false;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    nsInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    dev.ActivateAll();

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    nsInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    NS_TEST_FLOAT(fVal, 0.1f, 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 30.0));
    nsInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    NS_TEST_FLOAT(fVal, 0.1f, 0.0001f);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "GetInputSlotFlags")
  {
    nsTestInputDevide dev;
    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 30.0));

    NS_TEST_BOOL(nsInputManager::GetInputSlotFlags("testdevice_button") == nsInputSlotFlags::IsButton);
    NS_TEST_BOOL(nsInputManager::GetInputSlotFlags("testdevice_stick") == nsInputSlotFlags::IsAnalogStick);
    NS_TEST_BOOL(nsInputManager::GetInputSlotFlags("testdevice_wheel") == nsInputSlotFlags::IsMouseWheel);
    NS_TEST_BOOL(nsInputManager::GetInputSlotFlags("testdevice_touchpoint") == nsInputSlotFlags::IsTouchPoint);
  }

  NS_TEST_BLOCK(nsTestBlock::Enabled, "ClearInputMapping")
  {
    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    nsInputActionConfig iac;
    iac.m_bApplyTimeScaling = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    nsInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    nsTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));
    nsInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    NS_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    // clear the button from the action
    nsInputManager::ClearInputMapping("test_inputset", "testdevice_button");

    dev.ActivateAll();

    nsInputManager::Update(nsTime::MakeFromSeconds(1.0 / 60.0));

    // should not receive input anymore
    nsInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    NS_TEST_FLOAT(fVal, 0.0f, 0.0001f);
  }
}
