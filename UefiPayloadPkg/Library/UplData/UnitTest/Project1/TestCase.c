#include "Declarations.h"

VOID* Buffer;
UINTN UsedSize;

// Encode UINT64 value and decode Boolean value
RETURN_STATUS
TestCase1(
  VOID
)
{
  EFI_STATUS Status;
  BOOLEAN    Value;

  SetUplInteger("key64", 0x2000);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplBoolean("key64", &Value);
  return Status;
}

// Encode UINT64 value and decode UINT64 value but with different key
RETURN_STATUS
TestCase2(
  VOID
)
{
  EFI_STATUS Status;
  UINT64     Value;

  SetUplInteger("newkey", 0x5000);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplInteger("key", &Value);
  return Status;
}

// Encode UINT64 value and decode UINT64 value. Works fine
RETURN_STATUS
TestCase3(
  VOID
)
{
  EFI_STATUS Status;
  UINT64     SetValue;
  UINT64     Value;

  SetValue = 0x5000;
  SetUplInteger("Work", SetValue);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplInteger("Work", &Value);
  assert (SetValue == Value);
  return Status;
}

// Encode BOOLEAN value and decode UINT64 value
RETURN_STATUS
TestCase4(
  VOID
)
{
  EFI_STATUS Status;
  UINT64     Value;

  SetUplBoolean("boolkey", FALSE);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplInteger("boolkey", &Value);
  return Status;
}

// Encode BOOLEAN value and decode BOOLEAN value but with different key
RETURN_STATUS
TestCase5(
  VOID
)
{
  EFI_STATUS Status;
  BOOLEAN    Value;

  SetUplBoolean("BooleanNewKey", FALSE);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplBoolean("bool", &Value);
  return Status;
}

// Encode BOOLEAN value and decode BOOLEAN value. Works fine
RETURN_STATUS
TestCase6(
  VOID
)
{
  EFI_STATUS Status;
  BOOLEAN    SetValue;
  BOOLEAN    Value;

  SetValue = TRUE;
  SetUplBoolean("Random", SetValue);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplBoolean("Random", &Value);
  assert(SetValue == Value);
  return Status;
}

// Decode UINT64 value with a boolean encoded key
RETURN_STATUS
TestCase7(
  VOID
)
{
  EFI_STATUS Status;
  UINT64     Value;

  InitUplFromBuffer(Buffer);
  Status = GetUplInteger("Random", &Value);
  return Status;
}

// Encode Binary value and decode UINT64 value
RETURN_STATUS
TestCase8(
  VOID
)
{
  EFI_STATUS Status;
  UINT64     Value;

  UINT32 Array[] = {0x1234, 0x4537};
  SetUplBinary ("BinaryKey", &Array, 8);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplInteger("BinaryKey", &Value);
  return Status;
}

// Encode Binary value and decode Binary value. Work fine
RETURN_STATUS
TestCase9(
  VOID
)
{
  EFI_STATUS Status;
  UINT32     Value[20];
  UINTN      Size;

  UINT32 Array[] = { 0x37, 0x4537, 0x14, 0x7356, 0x12345678};
  SetUplBinary("Binary", &Array, 20);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplBinary("Binary", &Value, &Size);
  for (int i = 0; i < 5; i++) {
    assert(Array[i] == Value[i]);
  }
  return Status;
}

// Encode Ascii string and decode UINT64 Value
RETURN_STATUS
TestCase10(
  VOID
)
{
  EFI_STATUS Status;
  UINT64     Value;

  SetUplAsciiString("StringKey", "NewString", 10);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplInteger("StringKey", &Value);
  return Status;
}

// Encode Ascii string and decode Ascii string. Work fine
RETURN_STATUS
TestCase11(
  VOID
)
{
  EFI_STATUS Status;
  UINT8      *SetValue;
  UINT8      *Value;
  UINTN      Size;

  Value = malloc(20);
  SetValue = "GetString";
  SetUplAsciiString("AsciiKey", SetValue, 10);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplAsciiString("AsciiKey", Value, &Size);
  if (strcmp(SetValue, Value)) {
    assert(FALSE);
  }
  free(Value);
  return Status;
}

// Encode Universal Payload Extra data and decode Universal Payload extra data. Work fine
RETURN_STATUS
TestCase12(
  VOID
)
{
  EFI_STATUS Status;
  UINTN      Count;
  UINTN      Index;
  CHAR8      Entry0[] = "OldExtraData";
  CHAR8      Entry1[] = "PayloadData";
  CHAR8      Entry2[] = "NewEntry";
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY Entry[3];
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY Output[3];

  Count = 3;
  Index = 0;
  Entry[0].Base = 0x40;
  Entry[1].Base = 0x85;
  Entry[0].Size = 0x20;
  Entry[1].Size = 0x10;
  Entry[0].Base = 0x100;
  Entry[1].Size = 0x30;

  AsciiStrCpyS(Entry[0].Identifier, sizeof(Entry0), Entry0);
  AsciiStrCpyS(Entry[1].Identifier, sizeof(Entry1), Entry1);
  AsciiStrCpyS(Entry[2].Identifier, sizeof(Entry2), Entry2);

  SetUplExtraData(Entry, 3);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplExtraData(Output, &Count, Index);

  for (int i = 0; i < Count; i++) {
    assert(Entry[i].Base == Output[i].Base);
    assert(Entry[i].Size == Output[i].Size);
    if (strcmp(Entry[i].Identifier, Output[i].Identifier)) {
      assert(FALSE);
    }
  }

  return Status;
}

// Decode Universal payload Extra Data where Index is greater than or equal to total array count
// returns unsupported
RETURN_STATUS
TestCase13(
  VOID
)
{
  EFI_STATUS Status;
  UINTN      Count;
  UINTN      Index;
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY Entry[2];
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY Output[2];

  Count = 2;
  Index = 4;
  Entry[0].Base = 100;
  Entry[1].Base = 150;

  SetUplExtraData(Entry, 2);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplExtraData(Output, &Count, Index);
  return Status;
}

// Decode Universal payload Extra Data where count is zero on output
// returns RETURN_BUFFER_TOO_SMALL
RETURN_STATUS
TestCase14(
  VOID
)
{
  EFI_STATUS Status;
  UINTN      Count;
  UINTN      Index;
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY Entry[1];
  UNIVERSAL_PAYLOAD_EXTRA_DATA_ENTRY Output[1];

  Count = 0;
  Index = 0;
  Entry[0].Base = 20;

  SetUplExtraData(Entry, 1);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplExtraData(Output, &Count, Index);
  return Status;
}

// Encode Universal Payload memory map information and decode Universal Payload memory map information. 
// Work fine
RETURN_STATUS
TestCase15(
  VOID
)
{
  EFI_MEMORY_DESCRIPTOR In[2];
  EFI_MEMORY_DESCRIPTOR Out[2];
  EFI_STATUS            Status;
  UINTN                 Count;
  UINTN                 Index;

  Count = 2;
  Index = 0;

  In[0].PhysicalStart = 0x1000;
  In[0].NumberOfPages = 2;
  In[0].Type          = EfiBootServicesData;
  In[0].Attribute = EFI_RESOURCE_ATTRIBUTE_PRESENT |
                  EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                  EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
                  EFI_RESOURCE_ATTRIBUTE_TESTED;

  In[1].PhysicalStart = 0x5000;
  In[1].NumberOfPages = 1;
  In[1].Type          = EfiReservedMemoryType;
  In[1].Attribute = EFI_RESOURCE_ATTRIBUTE_PRESENT |
                  EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                  EFI_RESOURCE_ATTRIBUTE_TESTED;

  SetUplMemoryMap(In, 2);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplMemoryMap(Out, &Count, Index);

  for (int i = 0; i < Count; i++) {
    assert(In[i].PhysicalStart == Out[i].PhysicalStart);
    assert(In[i].NumberOfPages == Out[i].NumberOfPages);
    assert(In[i].Type == Out[i].Type);
    assert(In[i].Attribute == Out[i].Attribute);
  }

  return Status;
}

// Decode Universal Payload memory map information where count is zero on output.
RETURN_STATUS
TestCase16(
  VOID
)
{
  EFI_MEMORY_DESCRIPTOR Input[3];
  EFI_MEMORY_DESCRIPTOR Output[3];
  EFI_STATUS            Status;
  UINTN                 Count;
  UINTN                 Index;

  Count = 0;
  Index = 0;

  Input[0].PhysicalStart = 0x2000;
  Input[0].NumberOfPages = 1;
  Input[1].PhysicalStart = 0x4000;
  Input[1].NumberOfPages = 2;
  Input[2].PhysicalStart = 0x8000;
  Input[2].NumberOfPages = 3;

  SetUplMemoryMap(Input, 3);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplMemoryMap(Output, &Count, Index);
  return Status;
}

// Decode Universal Payload memory map information where Index is greater than or equal to total array count.
RETURN_STATUS
TestCase17(
  VOID
)
{
  EFI_MEMORY_DESCRIPTOR Input[2];
  EFI_MEMORY_DESCRIPTOR Output[2];
  EFI_STATUS            Status;
  UINTN                 Count;
  UINTN                 Index;

  Count = 2;
  Index = 2;

  Input[0].PhysicalStart = 0x3000;
  Input[0].NumberOfPages = 2;
  Input[1].PhysicalStart = 0x9000;
  Input[1].NumberOfPages = 1;

  SetUplMemoryMap(Input, 2);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplMemoryMap(Output, &Count, Index);
  return Status;
}

// Encode and decode Universal Payload pci root bridge information. Work fine.
RETURN_STATUS
TestCase18(
  VOID
)
{
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE In[2];
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE Out[3];
  EFI_STATUS                        Status;
  UINTN                             Count;
  UINTN                             Index;

  Count = 2;
  Index = 0;

  In[0].Bus.Base = 0x35;
  In[0].Bus.Limit = 0x4A;
  In[0].Io.Base = 0x6000;
  In[0].Io.Limit = 0x7000;

  In[1].Bus.Base = 0x86;
  In[1].Bus.Limit = 0xA0;
  In[1].Io.Base = 0xB000;
  In[1].Io.Limit = 0xC000;

  SetUplPciRootBridges(In, 2);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplPciRootBridges(Out, &Count, Index);

  for (int i = 0; i < Count; i++) {
    assert(In[i].Bus.Base == Out[i].Bus.Base);
    assert(In[i].Bus.Limit == Out[i].Bus.Limit);
    assert(In[i].Io.Base == Out[i].Io.Base);
    assert(In[i].Io.Limit == Out[i].Io.Limit);
  }

  return Status;
}

// Encode and decode Universal Payload pci root bridge information where Count is zero on output.
RETURN_STATUS
TestCase19(
  VOID
)
{
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE In[1];
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE Out[1];
  EFI_STATUS                        Status;
  UINTN                             Count;
  UINTN                             Index;

  Count = 0;
  Index = 0;

  In[0].Bus.Base = 0x02;
  In[0].Bus.Limit = 0x07;
  In[0].Io.Base = 0x2000;
  In[0].Io.Limit = 0x3000;

  SetUplPciRootBridges(In, 1);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplPciRootBridges(Out, &Count, Index);
  return Status;
}

// Encode and decode Universal Payload pci root bridge information where Index is greater than or equal to total array count.
RETURN_STATUS
TestCase20(
  VOID
)
{
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE In[1];
  UNIVERSAL_PAYLOAD_PCI_ROOT_BRIDGE Out[1];
  EFI_STATUS                        Status;
  UINTN                             Count;
  UINTN                             Index;

  Count = 1;
  Index = 3;

  In[0].Bus.Base = 0x0B;
  In[0].Bus.Limit = 0x14;
  In[0].Io.Base = 0x5000;
  In[0].Io.Limit = 0x6000;

  SetUplPciRootBridges(In, 1);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplPciRootBridges(Out, &Count, Index);
  return Status;
}

// Encode and decode Universal Payload resource descriptor information. Work fine.
RETURN_STATUS
TestCase21(
  VOID
)
{
  UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR In[4];
  UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR Out[4];
  EFI_STATUS                            Status;
  UINTN                                 Count;
  UINTN                                 Index;

  Count = 4;
  Index = 0;

  In[0].PhysicalStart = 0x2000;
  In[0].ResourceLength = 0x3000;
  In[1].PhysicalStart = 0x7000;
  In[1].ResourceLength = 0x2000;
  In[2].PhysicalStart = 0xA000;
  In[2].ResourceLength = 0x1000;
  In[3].PhysicalStart = 0xC000;
  In[3].ResourceLength = 0x1000;

  SetUplResourceData(In, 4);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplResourceData(Out, &Count, Index);

  for (int i = 0; i < Count; i++) {
    assert(In[i].PhysicalStart == Out[i].PhysicalStart);
    assert(In[i].ResourceLength == Out[i].ResourceLength);
  }

  return Status;
}

// Encode and decode Universal Payload resource descriptor information where count is zero on output.
RETURN_STATUS
TestCase22(
  VOID
)
{
  UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR In[1];
  UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR Out[1];
  EFI_STATUS                            Status;
  UINTN                                 Count;
  UINTN                                 Index;

  Count = 0;
  Index = 0;

  In[0].PhysicalStart = 0x1000;
  In[0].ResourceLength = 0x5000;

  SetUplResourceData(In, 1);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplResourceData(Out, &Count, Index);
  return Status;
}

// Encode and decode Universal Payload resource descriptor information where Index is greater than or equal to total array count.
RETURN_STATUS
TestCase23(
  VOID
)
{
  UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR In[1];
  UNIVERSAL_PAYLOAD_RESOURCE_DESCRIPTOR Out[1];
  EFI_STATUS                            Status;
  UINTN                                 Count;
  UINTN                                 Index;

  Count = 1;
  Index = 6;

  In[0].PhysicalStart = 0x4000;
  In[0].ResourceLength = 0x4000;

  SetUplResourceData(In, 1);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplResourceData(Out, &Count, Index);
  return Status;
}

// Encode and decode Universal Payload memory allocation information. Work fine.
RETURN_STATUS
TestCase24(
  VOID
)
{
  UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION   In[3];
  UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION   Out[3];
  EFI_STATUS                            Status;
  UINTN                                 Count;
  UINTN                                 Index;

  Count = 3;
  Index = 0;

  In[0].MemoryBaseAddress = 0x1000;
  In[0].MemoryLength = 0x2000;
  In[1].MemoryBaseAddress = 0x5000;
  In[1].MemoryLength = 0x1000;
  In[2].MemoryBaseAddress = 0x8000;
  In[2].MemoryLength = 0x1000;

  SetUplMemoryAllocationData(In, 3);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplMemoryAllocationData(Out, &Count, Index);

  for (int i = 0; i < Count; i++) {
    assert(In[i].MemoryBaseAddress == Out[i].MemoryBaseAddress);
    assert(In[i].MemoryLength == Out[i].MemoryLength);
  }

  return Status;
}

// Encode and decode Universal Payload memory allocation information where count is zero on output.
RETURN_STATUS
TestCase25(
  VOID
)
{
  UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION   In[1];
  UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION   Out[1];
  EFI_STATUS                            Status;
  UINTN                                 Count;
  UINTN                                 Index;

  Count = 0;
  Index = 0;

  In[0].MemoryBaseAddress = 0x7000;
  In[0].MemoryLength = 0x3000;

  SetUplMemoryAllocationData(In, 1);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplMemoryAllocationData(Out, &Count, Index);
  return Status;
}

// Encode and decode Universal Payload memory allocation information where Index is greater than
// or equal to total array count
RETURN_STATUS
TestCase26(
  VOID
)
{
  UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION   In[1];
  UNIVERSAL_PAYLOAD_MEMORY_ALLOCATION   Out[1];
  EFI_STATUS                            Status;
  UINTN                                 Count;
  UINTN                                 Index;

  Count = 1;
  Index = 5;

  In[0].MemoryBaseAddress = 0x3000;
  In[0].MemoryLength = 0x4000;

  SetUplMemoryAllocationData(In, 1);
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplMemoryAllocationData(Out, &Count, Index);
  return Status;
}

// Case where encode function returns out of resources.
RETURN_STATUS
TestCase27(
  VOID
)
{
  EFI_STATUS Status;
  UINT64     Value;

  for (int i = 0; i < 2730; i++) {
    Status = SetUplInteger("CheckForResource", 0x123456789ABCDEF);
    if (Status != 0) {
    return Status;
    }
  }
  LockUplAndGetBuffer(&Buffer, &UsedSize);
  InitUplFromBuffer(Buffer);
  Status = GetUplInteger("CheckForResource", &Value);
  return Status;
}

TEST_CASE_STRUCT gTestCase[] = {
  {TestCase1, RETURN_INVALID_PARAMETER},
  {TestCase2, RETURN_NOT_FOUND},
  {TestCase3, RETURN_SUCCESS},
  {TestCase4, RETURN_INVALID_PARAMETER},
  {TestCase5, RETURN_NOT_FOUND},
  {TestCase6, RETURN_SUCCESS},
  {TestCase7, RETURN_INVALID_PARAMETER},
  {TestCase8, RETURN_INVALID_PARAMETER},
  {TestCase9, RETURN_SUCCESS},
  {TestCase10, RETURN_INVALID_PARAMETER},
  {TestCase11, RETURN_SUCCESS},
  {TestCase12, RETURN_SUCCESS},
  {TestCase13, RETURN_UNSUPPORTED},
  {TestCase14, RETURN_BUFFER_TOO_SMALL},
  {TestCase15, RETURN_SUCCESS},
  {TestCase16, RETURN_BUFFER_TOO_SMALL},
  {TestCase17, RETURN_UNSUPPORTED},
  {TestCase18, RETURN_SUCCESS},
  {TestCase19, RETURN_BUFFER_TOO_SMALL},
  {TestCase20, RETURN_UNSUPPORTED},
  {TestCase21, RETURN_SUCCESS},
  {TestCase22, RETURN_BUFFER_TOO_SMALL},
  {TestCase23, RETURN_UNSUPPORTED},
  {TestCase24, RETURN_SUCCESS},
  {TestCase25, RETURN_BUFFER_TOO_SMALL},
  {TestCase26, RETURN_UNSUPPORTED},
  {TestCase27, RETURN_OUT_OF_RESOURCES}
};

UINTN
GetTestCaseCount(
  VOID
)
{
  return ARRAY_SIZE(gTestCase);
}