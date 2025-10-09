
void OutputToConsole_Platform(nsUInt8 uiColor, nsTestOutput::Enum type, nsInt32 iIndentation, const char* szMsg)
{
  printf("%*s%s\n", iIndentation, "", szMsg);
}
