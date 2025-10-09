
/// \brief Many Linux APIs will fill out error on failure. This converts the error into an error code and a human-readable error message.
/// Pass in the linux `errno` symbol. Be careful when printing multiple values, a function could clear `errno` as a side-effect so it is best to store it in a temp variable before printing a complex error message.
/// You may have to include #include <errno.h> use this.
/// \sa https://man7.org/linux/man-pages/man3/errno.3.html
struct nsArgErrno
{
  inline explicit nsArgErrno(nsInt32 iErrno)
    : m_iErrno(iErrno)
  {
  }

  nsInt32 m_iErrno;
};

NS_FOUNDATION_DLL nsStringView BuildString(char* szTmp, nsUInt32 uiLength, const nsArgErrno& arg);
