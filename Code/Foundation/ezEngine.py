import lldb.formatters.Logger
import lldb
try:
  import debugpy
except ImportError:
  print("debugpy module not found")

def __lldb_init_module(debugger, internal_dict):
    # comment this in for debug output
    lldb.formatters.Logger._lldb_formatters_debug_level = 2

    try:
        print("debugpy is listening...")
        debugpy.listen(5678)
    except NameError:
        print("debugpy not installed.")

    # Strings
    debugger.HandleCommand('type synthetic add -x "^nsHybridString<.+>$" --python-class WDFramework.nsHybridStringSynthProvider -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsHybridString<.+>$" --python-function WDFramework.nsHybridString_SummaryProvider -w WDFramework')

    debugger.HandleCommand('type synthetic add nsStringBuilder --python-class WDFramework.nsHybridStringSynthProvider -w WDFramework')
    debugger.HandleCommand('type summary add nsStringBuilder --python-function WDFramework.nsHybridString_SummaryProvider -w WDFramework')

    debugger.HandleCommand('type synthetic add nsStringView --python-class WDFramework.nsStringViewSynthProvider -w WDFramework')
    debugger.HandleCommand('type summary add nsStringView --python-function WDFramework.nsStringView_SummaryProvider -w WDFramework')

    debugger.HandleCommand('type synthetic add nsHashedString --python-class WDFramework.nsHashedStringSynthProvider -w WDFramework')
    debugger.HandleCommand('type summary add nsHashedString --python-function WDFramework.nsHashedString_SummaryProvider -w WDFramework')

    debugger.HandleCommand('type summary add nsStringIterator --python-function WDFramework.nsStringIterator_SummaryProvider -w WDFramework')
    debugger.HandleCommand('type summary add nsStringReverseIterator --python-function WDFramework.nsStringIterator_SummaryProvider -w WDFramework') 

    # Containers
    debugger.HandleCommand('type synthetic add -x "^nsDynamicArray<.+>$" --python-class WDFramework.nsDynamicArrayBaseSynthProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add -x "^nsHybridArray<.+>$" --python-class WDFramework.nsDynamicArrayBaseSynthProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add -x "^nsSmallArray<.+>$" --python-class WDFramework.nsSmallArraySynthProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add -x "^nsStaticArray<.+>$" --python-class WDFramework.nsStaticArraySynthProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add -x "^nsStaticRingBuffer<.+>$" --python-class WDFramework.nsStaticRingBufferSynthProvider -w WDFramework')

    debugger.HandleCommand('type synthetic add -x "^nsArrayPtr<.+>$" --python-class WDFramework.nsArrayPtrSynthProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add nsByteArrayPtr --python-class WDFramework.nsArrayPtrSynthProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add nsConstByteArrayPtr --python-class WDFramework.nsArrayPtrSynthProvider -w WDFramework')

    debugger.HandleCommand('type synthetic add -x "^nsHashTable<.+>$" --python-class WDFramework.nsHashTableBaseSynthProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add -x "^nsHashSet<.+>$" --python-class WDFramework.nsHashTableBaseSynthProvider -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsHashTableBase<.+>::Entry$" --summary-string "key = ${var.key}, value = ${var.value}" -w WDFramework')
    
    debugger.HandleCommand('type synthetic add -x "^nsMap<.+>$" --python-class WDFramework.nsMapSynthProvider -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsMapBase<.+>::Node$" --summary-string "key = ${var.m_Key}, value = ${var.m_Value}" -w WDFramework')
    
    debugger.HandleCommand('type synthetic add -x "^nsSet<.+>$" --python-class WDFramework.nsMapSynthProvider -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsSetBase<.+>::Node$" --summary-string "${var.m_Key}" -w WDFramework')
    
    debugger.HandleCommand('type synthetic add -x "^nsList<.+>$" --python-class WDFramework.nsListSynthProvider -w WDFramework')

    debugger.HandleCommand('type synthetic add -x "^nsDeque<.+>$" --python-class WDFramework.nsDequeSynthProvider -w WDFramework')

    # Basic Types
    debugger.HandleCommand('type synthetic add -x "^nsEnum<.+>$" --python-class WDFramework.nsEnumSynthProvider -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsEnum<.+>$" --python-function WDFramework.nsEnum_SummaryProvider -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsBitflags<.+>$" --python-function WDFramework.nsBitflags_SummaryProvider -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsVec2Template<.+>$" --summary-string "\{ x=${var.x}, y=${var.y} \}" -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsVec3Template<.+>$" --summary-string "\{ x=${var.x}, y=${var.y}, z=${var.z} \}" -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsVec4Template<.+>$" --summary-string "\{ x=${var.x}, y=${var.y}, z=${var.z}, w=${var.w} \}" -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsQuatTemplate<.+>$" --summary-string "\{ x=${var.x}, y=${var.y}, z=${var.z}, w=${var.w} \}" -w WDFramework')
    debugger.HandleCommand('type summary add -x "^nsPlaneTemplate<.+>$" --summary-string "\{ nx=${var.m_vNormal.x}, ny=${var.m_vNormal.y}, nz=${var.m_vNormal.z}, negDist=${var.m_fNegDistance} \}" -w WDFramework')
    debugger.HandleCommand('type summary add "nsColor" --summary-string "\{ r=${var.r}, g=${var.g}, b=${var.b}, a=${var.a} \}" -w WDFramework')
    debugger.HandleCommand('type summary add "nsColorGammaUB" --summary-string "\{ r=${var.r:d}, g=${var.g:d}, b=${var.b:d}, a=${var.a:d} \}" -w WDFramework')
    debugger.HandleCommand('type summary add "nsColorLinearUB" --summary-string "\{ r=${var.r:d}, g=${var.g:d}, b=${var.b:d}, a=${var.a:d} \}" -w WDFramework')
    debugger.HandleCommand('type summary add "nsTime" --summary-string "\{ seconds=${var.m_fTime} \}" -w WDFramework')
    debugger.HandleCommand('type summary add "nsAngle" --python-function WDFramework.nsAngle_SummaryProvider -w WDFramework')
    debugger.HandleCommand('type summary add "nsUuid" --python-function WDFramework.nsUuid_SummaryProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add "nsMat3" --python-class WDFramework.nsMat3SynthProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add "nsMat4" --python-class WDFramework.nsMat4SynthProvider -w WDFramework')
    debugger.HandleCommand('type synthetic add "nsVariant" --python-class WDFramework.nsVariantSynthProvider -w WDFramework')
    debugger.HandleCommand('type summary add "nsVariant" --python-function WDFramework.nsVariant_SummaryProvider -w WDFramework')

    debugger.HandleCommand('type category enable WDFramework')

#region Strings

def make_string(F):
    data = bytearray(F.GetData().uint8)
    if data[-1] == 0:
        data = data[:-1]
    return data.decode("utf-8")


class nsHybridStringSynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            m_Data = self.valobj.GetChildMemberWithName('m_Data')
            self.m_pElements = m_Data.GetChildMemberWithName('m_pElements')
            self.m_StaticData = m_Data.GetChildAtIndex(1).GetChildAtIndex(0)
            self.m_uiCount = m_Data.GetChildMemberWithName('m_uiCount')
            self.m_uiCapacity = m_Data.GetChildMemberWithName('m_uiCapacity')
            self.m_pAllocator = m_Data.GetChildMemberWithName('m_pAllocator')
            self.local_storage_size = self.m_StaticData.GetType().GetByteSize()
        except Exception as inst:
            logger >> "nsHybridStringSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        return 3

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            if index == 0:
                count = self.m_uiCount.GetValueAsUnsigned(0)
                # Detect if string is probably broken (uninitialized)
                if count > 0x10000000:
                    count = 0
                if count <= self.local_storage_size:
                    return self.m_StaticData.CreateValueFromData('contents', self.m_StaticData.GetData(), self.m_pElements.GetType().GetPointeeType().GetArrayType(count))
                else:
                    return self.m_pElements.CreateValueFromData('contents', self.m_pElements.GetPointeeData(0, count), self.m_pElements.GetType().GetPointeeType().GetArrayType(count))
            elif index == 1:
                return self.m_uiCount
            elif index == 2:
                return self.m_pAllocator
        except Exception as inst:
            logger >> "nsHybridStringSynthProvider.get_child_at_index: " + str(inst)
            return None

def nsHybridString_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        content = valobj.GetChildAtIndex(0)
        count = valobj.GetChildAtIndex(1).GetValueAsUnsigned(0)
        # Detect if string is probably broken (uninitialized)
        if count > 0x10000000:
            count = 0
        if count == 0:
            return "<empty>"
        if count > 1024:
            count = 1024
        return '"' + make_string(content) + '"'
    except Exception as inst:
        logger >> "nsHybridString_SummaryProvider: " + str(inst)
        return "<error>"

class nsStringViewSynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        self.valid = False
        try:
            self.m_pStart = self.valobj.GetChildMemberWithName('m_pStart')
            self.m_uiElementCount = self.valobj.GetChildMemberWithName('m_uiElementCount')

            start = self.m_pStart.GetValueAsUnsigned(0)
            count = self.m_uiElementCount.GetValueAsUnsigned(0)
            if start == 0 or count == 0:
                self.valid = False
            else:
                self.valid = True
                self.count = count
            
        except Exception as inst:
            logger >> "nsStringViewSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        return 1

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        if not self.valid:
            return None

        try:
            count = min(self.count, 256)
            return self.m_pStart.CreateValueFromData('contents', self.m_pStart.GetPointeeData(0, count), self.m_pStart.GetType().GetPointeeType().GetArrayType(count))
        except Exception as inst:
            logger >> "nsStringViewSynthProvider.get_child_at_index: " + str(inst)
            return None

def nsStringView_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        content = valobj.GetChildAtIndex(0)
        # Check if content is empty
        if content.IsValid() == False:
            return "<empty>"
        return '"' + make_string(content) + '"'
    except Exception as inst:
        logger >> "nsStringView_SummaryProvider: " + str(inst)
        return "<error>"

class nsHashedStringSynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        self.valid = False
        try:
            self.m_Data = self.valobj.GetChildMemberWithName('m_Data')
            self.m_pElement = self.m_Data.GetChildMemberWithName('m_pElement')
            self.m_Key = self.m_pElement.GetChildMemberWithName('m_Key')
            self.m_Value = self.m_pElement.GetChildMemberWithName('m_Value')
            self.m_sString = self.m_Value.GetChildMemberWithName('m_sString')
            self.valid = True

        except Exception as inst:
            logger >> "nsHashedStringSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        return 2

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()

        if not self.valid:
            return None

        try:
            if index == 0:
                return self.m_sString
            elif index == 1:
                return self.m_Key
        except Exception as inst:
            logger >> "nsHashedStringSynthProvider.get_child_at_index: " + str(inst)
            return None

def nsHashedString_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        content = valobj.GetChildAtIndex(0)
        return nsHybridString_SummaryProvider(content, dict)
    except Exception as inst:
        logger >> "nsHashedString_SummaryProvider: " + str(inst)
        return "<error>"

def nsStringIterator_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        m_pCurPtr = valobj.GetChildMemberWithName("m_pCurPtr")
        m_pEndPtr = valobj.GetChildMemberWithName("m_pEndPtr")
        curr = m_pCurPtr.GetValueAsUnsigned(0)
        end = m_pEndPtr.GetValueAsUnsigned(0)
        size = min(end - curr, 4) # We need up to 4 bytes to decode one utf8 character
        bytes = m_pCurPtr.GetPointeeData(0, size)
        if size == 0:
            return "<empty>"
        
        for i in range(1, size):
            # Check if the next byte marks the start of a new UTF-8 character
            if (bytes.uint8s[i] & 0xC0) != 0x80:
                size = i
                break
  
        data = bytearray(bytes.uint8s[:size])
        if size > 0:
            char = data.decode('utf-8')[0]
            return f"'{char}'"

    except Exception as inst:
        logger >> "nsStringIterator_SummaryProvider: " + str(inst)
        return "<error>"
    
#endregion

#region Containers
class ArraySynthProvider:
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def set_fields(self, names):
        self.names = names
        self.fields = []
        for name in self.names:
            member = self.valobj.GetChildMemberWithName(name)
            self.fields.append(member)
        
    def get_field_count(self):
        return self.fields.__len__()

    def get_field(self, index):
        return self.fields[index]
    
    def get_array_size(self, countField):
        count = countField.GetValueAsUnsigned(0)
        # Clamp the count to a maximum of 256 elements for performance reasons
        return min(count, 256)
        
    def get_array_element(self, elementsField, elementType, elementSize, index):
        offset = index * elementSize
        return elementsField.CreateChildAtOffset('[' + str(index) + ']', offset, elementType)
        
    def get_array_element_with_fake_index(self, elementsField, elementType, elementSize, index, fakeIndex):
        offset = index * elementSize
        return elementsField.CreateChildAtOffset('[' + str(fakeIndex) + ']', offset, elementType)

        
class nsDynamicArrayBaseSynthProvider(ArraySynthProvider):
    def __init__(self, valobj, dict):
         super().__init__(valobj, dict)

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            super().set_fields(['m_uiCount', 'm_uiCapacity', 'm_pAllocator'])
            self.m_pElements = self.valobj.GetChildMemberWithName('m_pElements')
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.element_type = self.m_pElements.GetType().GetPointeeType()
            self.element_size = self.element_type.GetByteSize()
        except Exception as inst:
            logger >> "nsDynamicArrayBaseSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            return super().get_array_size(self.m_uiCount) + super().get_field_count()
        except Exception as inst:
            logger >> "nsDynamicArrayBaseSynthProvider.num_children: " + str(inst)
            return 0

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            fields = super().get_field_count()
            if index < fields:
                return super().get_field(index)
            
            return super().get_array_element(self.m_pElements, self.element_type, self.element_size, index - fields)
        except Exception as inst:
            logger >> "nsDynamicArrayBaseSynthProvider.get_child_at_index: " + str(inst)
            return None

class nsSmallArraySynthProvider(ArraySynthProvider):
    def __init__(self, valobj, dict):
        super().__init__(valobj, dict)

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            super().set_fields(['m_uiCount', 'm_uiCapacity', 'm_uiUserData'])
            self.m_pElements = self.valobj.GetChildAtIndex(0).GetChildAtIndex(3).GetChildAtIndex(1)
            self.m_StaticData = self.valobj.GetChildAtIndex(0).GetChildAtIndex(3).GetChildAtIndex(0).GetChildAtIndex(0)
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.m_uiCapacity = self.valobj.GetChildMemberWithName('m_uiCapacity')
            self.element_type = self.valobj.GetType().GetTemplateArgumentType(0)
            self.element_size = self.element_type.GetByteSize()
            self.local_storage_size = self.m_StaticData.GetType().GetByteSize() // self.element_size
        except Exception as inst:
            logger >> "nsSmallArraySynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            return super().get_array_size(self.m_uiCount) + super().get_field_count()
        except Exception as inst:
            logger >> "nsSmallArraySynthProvider.num_children: " + str(inst)
            return 0

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            fields = super().get_field_count()
            if index < fields:
                return super().get_field(index)
                
            if self.m_uiCapacity.GetValueAsUnsigned(0) <= self.local_storage_size:
                return super().get_array_element(self.m_StaticData, self.element_type, self.element_size, index - fields)
            else:
                return super().get_array_element(self.m_pElements, self.element_type, self.element_size, index - fields)
        except Exception as inst:
            logger >> "nsSmallArraySynthProvider.get_child_at_index: " + str(inst)
            return None
        
class nsStaticArraySynthProvider(ArraySynthProvider):
    def __init__(self, valobj, dict):
        super().__init__(valobj, dict)

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            super().set_fields(['m_uiCount', 'm_uiCapacity'])
            self.m_Data = self.valobj.GetChildAtIndex(1).GetChildAtIndex(0)
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.element_type = self.valobj.GetType().GetTemplateArgumentType(0)
            self.element_size = self.element_type.GetByteSize()
        except Exception as inst:
            logger >> "nsStaticArraySynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            return super().get_array_size(self.m_uiCount) + super().get_field_count()
        except Exception as inst:
            logger >> "nsStaticArraySynthProvider.num_children: " + str(inst)
            return 0

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            fields = super().get_field_count()
            if index < fields:
                return super().get_field(index)
                
            return super().get_array_element(self.m_Data, self.element_type, self.element_size, index - fields)
        except Exception as inst:
            logger >> "nsStaticArraySynthProvider.get_child_at_index: " + str(inst)
            return None

class nsStaticRingBufferSynthProvider(ArraySynthProvider):
    def __init__(self, valobj, dict):
        super().__init__(valobj, dict)

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            super().set_fields(['m_uiCount', 'm_uiFirstElement'])
            self.m_pElements = self.valobj.GetChildMemberWithName('m_pElements')
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.m_uiFirstElement = self.valobj.GetChildMemberWithName('m_uiFirstElement')
            self.element_type = self.valobj.GetType().GetTemplateArgumentType(0)
            self.element_size = self.element_type.GetByteSize()

            self.m_StaticData = self.valobj.GetChildAtIndex(0).GetChildAtIndex(0)
            self.local_storage_size = self.m_StaticData.GetType().GetByteSize() // self.element_size
        except Exception as inst:
            logger >> "nsStaticRingBufferSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            return super().get_array_size(self.m_uiCount) + super().get_field_count()
        except Exception as inst:
            logger >> "nsStaticRingBufferSynthProvider.update: " + str(inst)
            return 0

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            fields = super().get_field_count()
            if index < fields:
                return super().get_field(index)
            
            index = index - fields
            firstElement = self.m_uiFirstElement.GetValueAsUnsigned(0)
            actualIndex = (firstElement + index) % self.local_storage_size
            offset = actualIndex * self.element_size
            return super().get_array_element_with_fake_index(self.m_pElements, self.element_type, self.element_size, actualIndex, index)
        except Exception as inst:
            logger >> "nsStaticRingBufferSynthProvider.get_child_at_index: " + str(inst)
            return None

class nsArrayPtrSynthProvider(ArraySynthProvider):
    def __init__(self, valobj, dict):
        super().__init__(valobj, dict)

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            super().set_fields(['m_uiCount'])
            self.m_pPtr = self.valobj.GetChildMemberWithName('m_pPtr')
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.element_type = self.valobj.GetType().GetTemplateArgumentType(0)
            self.element_size = self.element_type.GetByteSize()
        except Exception as inst:
            logger >> "nsArrayPtrSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            return super().get_array_size(self.m_uiCount) + super().get_field_count()
        except Exception as inst:
            logger >> "nsArrayPtrSynthProvider.num_children: " + str(inst)
            return 0

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            fields = super().get_field_count()
            if index < fields:
                return super().get_field(index)
                
            return super().get_array_element(self.m_pPtr, self.element_type, self.element_size, index - fields)
        except Exception as inst:
            logger >> "nsArrayPtrSynthProvider.get_child_at_index: " + str(inst)
            return None
    
class nsHashTableBaseSynthProvider(ArraySynthProvider):
    def __init__(self, valobj, dict):
        super().__init__(valobj, dict)

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            super().set_fields(['m_uiCount', 'm_uiCapacity', 'm_pAllocator'])
            self.m_pEntries = self.valobj.GetChildMemberWithName('m_pEntries')
            self.m_pEntryFlags = self.valobj.GetChildMemberWithName('m_pEntryFlags')
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.m_uiCapacity = self.valobj.GetChildMemberWithName('m_uiCapacity')
            self.element_type = self.m_pEntries.GetType().GetPointeeType()
            self.element_size = self.element_type.GetByteSize()
            self.flags_type = self.m_pEntryFlags.GetType().GetPointeeType()
            self.flags_size = self.flags_type.GetByteSize()
            # reading the data through the debugger can take a long time
            # Limit the loops to this number of iterations
            self.maxSteps = 255 
            self.compactedEntries = []
            capacity = self.m_uiCapacity.GetValueAsUnsigned(0)
            fakeIndex = 0
            for index in range(0, capacity):
                flags = super().get_array_element(self.m_pEntryFlags, self.flags_type, self.flags_size, index // 16)
                flags_value = flags.GetValueAsUnsigned(0)
                flags_value = flags_value >> ((index & 15) * 2) & 3
                isvalid = (flags_value & 0x01) == 0x01
                if isvalid:
                    self.compactedEntries.append(super().get_array_element_with_fake_index(self.m_pEntries, self.element_type, self.element_size, index, fakeIndex))
                    fakeIndex += 1
                if fakeIndex >= self.maxSteps:
                    break  # Limit the number of entries to avoid performance issues in the debugger

        except Exception as inst:
            logger >> "nsHashTableBaseSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            return super().get_array_size(self.m_uiCount) + super().get_field_count()
        except Exception as inst:
            logger >> "nsHashTableBaseSynthProvider.num_children: " + str(inst)
            return 0

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            fields = super().get_field_count()
            if index < fields:
                return super().get_field(index)
                
            index = index - fields
            return self.compactedEntries[index]
        except Exception as inst:
            logger >> "nsHashTableBaseSynthProvider.get_child_at_index: " + str(inst)
            return None

class nsListSynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.element_type = self.valobj.GetType().GetTemplateArgumentType(0)

            current = self.valobj.GetChildMemberWithName('m_First').GetChildMemberWithName('m_pNext')
            count = self.m_uiCount.GetValueAsUnsigned(0)

            self.compactedEntries = []
            for index in range(0, count):
                value = current.GetChildMemberWithName('m_Data')
                self.compactedEntries.append(value)
                current = current.GetChildMemberWithName('m_pNext')

        except Exception as inst:
            logger >> "nsListSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            numElements = self.m_uiCount.GetValueAsUnsigned(0)
            return numElements + 1
        except Exception as inst:
            logger >> "nsListSynthProvider.num_children: " + str(inst)
            return 0
    
    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            if index < 1:
                return self.m_uiCount

            index = index - 1
            return self.compactedEntries[index].CreateChildAtOffset('[' + str(index) + ']', 0, self.element_type)
        except Exception as inst:
            logger >> "nsListSynthProvider.get_child_at_index: " + str(inst)
            return None

class nsDequeSynthProvider(ArraySynthProvider):
    def __init__(self, valobj, dict):
        super().__init__(valobj, dict)

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            super().set_fields(['m_uiCount', 'm_pAllocator'])
            self.m_pChunks = self.valobj.GetChildMemberWithName('m_pChunks')
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.first_element = self.valobj.GetChildMemberWithName('m_uiFirstElement').GetValueAsUnsigned(0)
            self.element_type = self.valobj.GetType().GetTemplateArgumentType(0)
            self.element_size = self.element_type.GetByteSize()

            self.chunk_type = self.m_pChunks.GetType().GetPointeeType()
            self.chunk_pointer_size = self.chunk_type.GetByteSize()

            self.m_uiChunkSize = self.valobj.GetChildMemberWithName('m_uiChunkSize')
            self.chunk_size = self.m_uiChunkSize.GetValueAsUnsigned(0)
            if (4096 // self.element_size) < 32:
                self.chunk_size = 32
            else:
                self.chunk_size = 4096 // self.element_size
            
        except Exception as inst:
            logger >> "nsDequeSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            return super().get_array_size(self.m_uiCount) + super().get_field_count()
        except Exception as inst:
            logger >> "nsDequeSynthProvider.num_children: " + str(inst)
            return 0

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            fields = super().get_field_count()
            if index < fields:
                return super().get_field(index)
            
            index = index - fields
            realIndex = self.first_element + index
            chunkIndex = realIndex // self.chunk_size
            chunkOffset = realIndex % self.chunk_size
            chunk = super().get_array_element(self.m_pChunks, self.chunk_type, self.chunk_pointer_size, chunkIndex)
            element = super().get_array_element_with_fake_index(chunk, self.element_type, self.element_size, chunkOffset, index)
            return element
        except Exception as inst:
            logger >> "nsDequeSynthProvider.get_child_at_index: " + str(inst)
            return None

class nsMapSynthProvider:
    def __init__(self, valobj, dict):
        logger = lldb.formatters.Logger.Logger()
        self.valobj = valobj
        # reading the data through the debugger can take a long time
        # Limit the loops to this number of iterations
        self.maxSteps = 1000 
        self.lastNodeIndex = -2
        self.lastNode = None

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            self.lastNodeIndex = -2
            self.lastNode = None
            self.m_uiCount = self.valobj.GetChildMemberWithName('m_uiCount')
            self.m_pRoot = self.valobj.GetChildMemberWithName('m_pRoot')
            self.m_NilNodeAddr = self.valobj.GetChildMemberWithName('m_NilNode').GetLoadAddress()
            self.element_type = self.m_pRoot.GetType().GetPointeeType()
            self.element_size = self.element_type.GetByteSize()
        except Exception as inst:
            logger >> "nsMapSynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            numElements = self.m_uiCount.GetValueAsUnsigned(0)
            if numElements > 0xff000000:
                return 1
            numElements = min(numElements, 256)
            return numElements + 1
        except Exception as inst:
            logger >> "nsMapSynthProvider.num_children: " + str(inst)
            return 0

    def GetLink(self, node, index):
        return node.Dereference().GetChildMemberWithName('m_pLink').GetChildAtIndex(index)

    def GetParent(self, node):
        return node.Dereference().GetChildMemberWithName('m_pParent')

    def GetLeftMost(self):
        node = self.m_pRoot
        steps = self.maxSteps

        while self.GetLink(node, 0).GetValueAsUnsigned() != self.m_NilNodeAddr and steps > 0:
            node = self.GetLink(node, 0)
            steps = steps -1

        if steps == 0:
            return None

        return node

    def NextNode(self, node):
        rightNode = self.GetLink(node, 1)

        steps = self.maxSteps

        # if this element has a right child, go there and then search for the left most child of that
        if rightNode.GetValueAsUnsigned() != self.GetLink(rightNode, 1).GetValueAsUnsigned():
            node = rightNode

            while self.GetLink(node, 0).GetValueAsUnsigned() != self.GetLink(self.GetLink(node, 0), 0).GetValueAsUnsigned() and steps > 0:
                node = self.GetLink(node, 0)
                steps = steps - 1

            if steps == 0:
                return None

            return node

        parent = self.GetParent(node)
        parentParent = self.GetParent(parent)

        # if this element has a parent and this element is that parents left child, go directly to the parent
        if parent.GetValueAsUnsigned() != parentParent.GetValueAsUnsigned() and self.GetLink(parent, 0).GetValueAsUnsigned() == node.GetValueAsUnsigned():
            return parent

        # if this element has a parent and this element is that parents right child, search for the next parent, whose left child this is
        if parent.GetValueAsUnsigned() != parentParent.GetValueAsUnsigned() and self.GetLink(parent, 1).GetValueAsUnsigned() == node.GetValueAsUnsigned():
            while self.GetLink(self.GetParent(node), 1).GetValueAsUnsigned() == node.GetValueAsUnsigned() and steps > 0:
                node = self.GetParent(node)
                steps = steps - 1

            if steps == 0:
                return None

            # if we are the root node
            parent = self.GetParent(node)
            if (parent.GetValueAsUnsigned() == 0) or (parent.GetValueAsUnsigned() == self.GetParent(parent).GetValueAsUnsigned()):
                return None

            return parent

        return None   

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            if index < 1:
                if index == 0:
                    return self.m_uiCount
            else:
                index = index - 1

            if index >= self.num_children():
                return None
            node = self.GetLeftMost()
            stepsLeft = index

            # Can we re-use the last cached result?
            if index == self.lastNodeIndex + 1:
                node = self.lastNode
                stepsLeft = 1

            while node and stepsLeft > 0:
                node = self.NextNode(node)
                stepsLeft = stepsLeft - 1

            if node:
                self.lastNode = node
                self.lastNodeIndex = index
                return node.CreateChildAtOffset('[' + str(index) + ']', 0, node.GetType().GetPointeeType())

            return None

        except Exception as inst:
            logger >> "nsMapSynthProvider.get_child_at_index: " + str(inst)
            return None

#endregion

#region Enums and Bitflags

class nsEnumSynthProvider:
    def __init__(self, valobj, dict):
        self.valobj = valobj


    def update(self):
        self.value = "?"
        logger = lldb.formatters.Logger.Logger()
        try:
            enum_name = self.valobj.GetType().GetTemplateArgumentType(0).GetName() + "::Enum"
            enum_type = self.valobj.GetTarget().FindTypes(enum_name).GetTypeAtIndex(0)
            value = self.valobj.GetChildMemberWithName("m_Value").GetValueAsUnsigned()
            self.value = "? ({})".format(value)
            for v in enum_type.GetEnumMembers():
                if value == v.GetValueAsUnsigned():
                    self.value = "{} ({})".format(v.GetName(), v.GetValueAsUnsigned())
                    break
        except Exception as inst:
            logger >> "nsEnumSynthProvider.update: " + str(inst)
        return False

    def num_children(self):
        return 1

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            return self.valobj.CreateValueFromExpression('Value', '"' + self.value + '"')
        except Exception as inst:
            logger >> "nsEnumSynthProvider.get_child_at_index: " + str(inst)
            return None

def nsEnum_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        content = valobj.GetChildAtIndex(0)
        return make_string(content)
    except Exception as inst:
        logger >> "nsEnum_SummaryProvider: " + str(inst)
        return "<error>"

def nsBitflags_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        content = valobj.GetChildAtIndex(0).GetChildAtIndex(0)
        type = valobj.GetType().GetTemplateArgumentType(0)
        enum_name = type.GetName() + "::Enum"
        enum_type = valobj.GetTarget().FindTypes(enum_name).GetTypeAtIndex(0)

        value = content.GetValueAsUnsigned(0)
        flags_list = []
        for v in enum_type.GetEnumMembers():
            bit_value = v.GetValueAsUnsigned()
            # Skip if none or more than one bit is set (must be a power of 2 to have exactly one bit)
            if bit_value == 0 or (bit_value & (bit_value - 1)) != 0:
                continue
            if value & bit_value:
                flags_list.append(v.GetName())     

        if not flags_list:
            return "None (0)"
        return " | ".join(flags_list) + f" ({value})"
    except Exception as inst:
        logger >> "nsBitflags_SummaryProvider: " + str(inst)
        return "<error>"
    
#endregion

#region Basic Types

def nsAngle_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        m_fRadian = valobj.GetChildAtIndex(0)
        radian_value = float(m_fRadian.GetValue())

        degree_value = radian_value * 180.0 / 3.14159265358979323846
        return f"{{ Degree={degree_value:.2f}° }}"
    except Exception as inst:
        logger >> "nsAngle_SummaryProvider: " + str(inst)
        return "<error>"
    
def nsUuid_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        m_uiHigh = valobj.GetChildAtIndex(0)
        m_uiLow = valobj.GetChildAtIndex(1)
        m_uiHighValue = m_uiHigh.GetValueAsUnsigned(0)
        m_uiLowValue = m_uiLow.GetValueAsUnsigned(0)

        high_bytes = m_uiHighValue.to_bytes(8, byteorder='little')
        low_bytes = m_uiLowValue.to_bytes(8, byteorder='little')
        time_low = int.from_bytes(high_bytes[0:4], byteorder='little')
        time_mid = int.from_bytes(high_bytes[4:6], byteorder='little')
        time_hi_version = int.from_bytes(high_bytes[6:8], byteorder='little')
        clock_seq = int.from_bytes(low_bytes[0:2], byteorder='big')
        node = int.from_bytes(low_bytes[2:8], byteorder='big')

        uuid_str = f"{time_low:08x}-{time_mid:04x}-{time_hi_version:04x}-{clock_seq:04x}-{node:012x}"
        return uuid_str
        
    except Exception as inst:
        logger >> "nsUuid_SummaryProvider: " + str(inst)
        return "<error>"

class nsMat3SynthProvider:
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            self.m_fElementsCM = self.valobj.GetChildMemberWithName('m_fElementsCM')
            self.element_type = self.valobj.GetType().GetTemplateArgumentType(0)
            self.element_size = self.element_type.GetByteSize()
            
        except Exception as inst:
            logger >> "nsMat3SynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        return 4

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            if index == 0:
                expr = f"(nsVec3*)" +str(self.m_fElementsCM.GetAddress())
                Column0 = self.valobj.CreateValueFromExpression('Column0', expr)
                return Column0
            if index == 1:
                address = self.m_fElementsCM.GetAddress()
                address.OffsetAddress(self.element_size * 3)
                expr = f"(nsVec3*)" +str(address)
                Column1 = self.valobj.CreateValueFromExpression('Column1', expr)
                return Column1
            if index == 2:
                address = self.m_fElementsCM.GetAddress()
                address.OffsetAddress(self.element_size * 6)
                expr = f"(nsVec3*)" +str(address)
                Column2 = self.valobj.CreateValueFromExpression('Column2', expr)
                return Column2
            if index == 3:
                return self.m_fElementsCM
        except Exception as inst:
            logger >> "nsMat3SynthProvider.get_child_at_index: " + str(inst)
            return None
        
class nsMat4SynthProvider:
    def __init__(self, valobj, dict):
        self.valobj = valobj

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            self.m_fElementsCM = self.valobj.GetChildMemberWithName('m_fElementsCM')
            self.element_type = self.valobj.GetType().GetTemplateArgumentType(0)
            self.element_size = self.element_type.GetByteSize()
            
        except Exception as inst:
            logger >> "nsMat4SynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        return 5

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            if index == 0:
                expr = f"(nsVec4*)" +str(self.m_fElementsCM.GetAddress())
                Column0 = self.valobj.CreateValueFromExpression('Column0', expr)
                return Column0
            if index == 1:
                address = self.m_fElementsCM.GetAddress()
                address.OffsetAddress(self.element_size * 4)
                expr = f"(nsVec4*)" +str(address)
                Column1 = self.valobj.CreateValueFromExpression('Column1', expr)
                return Column1
            if index == 2:
                address = self.m_fElementsCM.GetAddress()
                address.OffsetAddress(self.element_size * 8)
                expr = f"(nsVec4*)" +str(address)
                Column2 = self.valobj.CreateValueFromExpression('Column2', expr)
                return Column2
            if index == 3:
                address = self.m_fElementsCM.GetAddress()
                address.OffsetAddress(self.element_size * 12)
                expr = f"(nsVec4*)" +str(address)
                Column3 = self.valobj.CreateValueFromExpression('Column3', expr)
                return Column3
            if index == 4:
                return self.m_fElementsCM
        except Exception as inst:
            logger >> "nsMat4SynthProvider.get_child_at_index: " + str(inst)
            return None

class nsVariantSynthProvider:

    variantMap = {}
    variantTargetTypeMap = {}

    def __init__(self, valobj, dict):
        self.valobj = valobj
        if not nsVariantSynthProvider.variantMap:
            enum_type = self.valobj.GetTarget().FindTypes("nsVariantType::Enum").GetTypeAtIndex(0)
            for v in enum_type.GetEnumMembers():
                bit_value = v.GetValueAsUnsigned()
                nsVariantSynthProvider.variantMap[bit_value] = v.GetName()
            
            nsVariantSynthProvider.variantTargetTypeMap["Bool"] = "bool"
            nsVariantSynthProvider.variantTargetTypeMap["Int8"] = "nsInt8"
            nsVariantSynthProvider.variantTargetTypeMap["UInt8"] = "nsUInt8"
            nsVariantSynthProvider.variantTargetTypeMap["Int16"] = "nsInt16"
            nsVariantSynthProvider.variantTargetTypeMap["UInt16"] = "nsUInt16"
            nsVariantSynthProvider.variantTargetTypeMap["Int32"] = "nsInt32"
            nsVariantSynthProvider.variantTargetTypeMap["UInt32"] = "nsUInt32"
            nsVariantSynthProvider.variantTargetTypeMap["Int64"] = "nsInt64"
            nsVariantSynthProvider.variantTargetTypeMap["UInt64"] = "nsUInt64"
            nsVariantSynthProvider.variantTargetTypeMap["Float"] = "float"
            nsVariantSynthProvider.variantTargetTypeMap["Double"] = "double"
            nsVariantSynthProvider.variantTargetTypeMap["Color"] = "nsColor"
            nsVariantSynthProvider.variantTargetTypeMap["Vector2"] = "nsVec2"
            nsVariantSynthProvider.variantTargetTypeMap["Vector3"] = "nsVec3"
            nsVariantSynthProvider.variantTargetTypeMap["Vector4"] = "nsVec4"
            nsVariantSynthProvider.variantTargetTypeMap["Vector2I"] = "nsVec2I32"
            nsVariantSynthProvider.variantTargetTypeMap["Vector3I"] = "nsVec3I32"
            nsVariantSynthProvider.variantTargetTypeMap["Vector4I"] = "nsVec4I32"
            nsVariantSynthProvider.variantTargetTypeMap["Vector2U"] = "nsVec2U32"
            nsVariantSynthProvider.variantTargetTypeMap["Vector3U"] = "nsVec3U32"
            nsVariantSynthProvider.variantTargetTypeMap["Vector4U"] = "nsVec4U32"
            nsVariantSynthProvider.variantTargetTypeMap["Quaternion"] = "nsQuat"
            nsVariantSynthProvider.variantTargetTypeMap["Matrix3"] = "nsMat3"
            nsVariantSynthProvider.variantTargetTypeMap["Matrix4"] = "nsMat4"
            nsVariantSynthProvider.variantTargetTypeMap["Transform"] = "nsTransform"
            nsVariantSynthProvider.variantTargetTypeMap["String"] = "nsString"
            nsVariantSynthProvider.variantTargetTypeMap["StringView"] = "nsStringView"
            nsVariantSynthProvider.variantTargetTypeMap["DataBuffer"] = "nsDataBuffer"
            nsVariantSynthProvider.variantTargetTypeMap["Time"] = "nsTime"
            nsVariantSynthProvider.variantTargetTypeMap["Uuid"] = "nsUuid"
            nsVariantSynthProvider.variantTargetTypeMap["Angle"] = "nsAngle"
            nsVariantSynthProvider.variantTargetTypeMap["ColorGamma"] = "nsColorGammaUB"
            nsVariantSynthProvider.variantTargetTypeMap["HashedString"] = "nsHashedString"
            nsVariantSynthProvider.variantTargetTypeMap["TempHashedString"] = "nsTempHashedString"
            nsVariantSynthProvider.variantTargetTypeMap["VariantArray"] = "nsVariantArray"
            nsVariantSynthProvider.variantTargetTypeMap["VariantDictionary"] = "nsVariantDictionary"
            nsVariantSynthProvider.variantTargetTypeMap["TypedPointer"] = "nsTypedPointer"

    def update(self):
        logger = lldb.formatters.Logger.Logger()
        try:
            self.m_Data = self.valobj.GetChildMemberWithName('m_Data')
            self.m_Ptr = self.m_Data.EvaluateExpression('shared->m_Ptr')
            self.m_sharedType = self.m_Data.EvaluateExpression('shared->m_pType')
            self.m_inlineType = self.m_Data.EvaluateExpression('inlined.m_pType')
            self.m_uiType = self.valobj.GetChildMemberWithName('m_uiType')
            self.m_bIsShared = self.valobj.GetChildMemberWithName('m_bIsShared')
            
        except Exception as inst:
            logger >> "nsMat4SynthProvider.update: " + str(inst)

        return False

    def num_children(self):
        return 3

    def get_child_at_index(self, index):
        logger = lldb.formatters.Logger.Logger()
        try:
            if index == 0:
                typeValue = self.m_uiType.GetValueAsUnsigned(0)
                return self.valobj.CreateValueFromExpression('Type', f"(nsVariantType::Enum) {typeValue}")
                return self.m_uiType
            if index == 1:
                typeValue = self.m_uiType.GetValueAsUnsigned(0)
                isShared = self.m_bIsShared.GetValueAsUnsigned(0)
                # Choose the correct data source based on isShared
                if isShared != 0:
                    address = self.m_Ptr.GetValueAsUnsigned()
                else:
                    address = self.m_Data.GetAddress()
                # Check if we can map the typeValue to a type name
                if typeValue in nsVariantSynthProvider.variantMap:
                    typeName = nsVariantSynthProvider.variantMap[typeValue]
                    if typeName in nsVariantSynthProvider.variantTargetTypeMap:
                        targetType = nsVariantSynthProvider.variantTargetTypeMap[typeName]
                        expr = f"*reinterpret_cast<{targetType}*>({address})"
                        return self.valobj.CreateValueFromExpression('Value', expr)

                if 'Invalid' == nsVariantSynthProvider.variantMap[typeValue]:
                    return self.valobj.CreateValueFromExpression('Invalid', 'nullptr')
                
                if 'TypedObject' == nsVariantSynthProvider.variantMap[typeValue]:
                    # For TypedObject, we extract the name of the RTTI type and build an expression to cast to it
                    if isShared != 0:
                        rtti = self.m_sharedType
                    else:
                        rtti = self.m_inlineType
                    typeName = rtti.GetChildMemberWithName('m_sTypeName')
                    typeNameStr = str(typeName.GetSummary()).replace('"', '') # A bit hacky, it depends on the nsStringView summary provider
                    expr = f"({typeNameStr}*){address}"
                    return self.valobj.CreateValueFromExpression('Value', expr)

                return self.m_Data
            if index == 2:
                return self.m_bIsShared
        except Exception as inst:
            logger >> "nsVariantSynthProvider.get_child_at_index: " + str(inst)
            return None
        
def nsVariant_SummaryProvider(valobj, dict):
    logger = lldb.formatters.Logger.Logger()
    try:
        type = valobj.GetChildAtIndex(0).GetValue()
        value = valobj.GetChildAtIndex(1)
        valueSummary = value.GetSummary()
        valueValue = value.GetValue()
        summaryString = f"({type}) "
        if valueSummary is not None and valueSummary != "":
            summaryString += valueSummary
        elif valueValue is not None and valueValue != "":
            summaryString += valueValue
        return summaryString
    except Exception as inst:
        logger >> "nsVariant_SummaryProvider: " + str(inst)
        return "<error>"
    
#endregion