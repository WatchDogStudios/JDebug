
bool nsDefaultAssertHandler_Platform(const char* szSourceFile, nsUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg, const char* szTemp)
{
  NS_IGNORE_UNUSED(szSourceFile);
  NS_IGNORE_UNUSED(uiLine);
  NS_IGNORE_UNUSED(szFunction);
  NS_IGNORE_UNUSED(szExpression);
  NS_IGNORE_UNUSED(szAssertMsg);
  NS_IGNORE_UNUSED(szTemp);

  // always do a debug-break
  // in release-builds this will just crash the app
  return true;
}
