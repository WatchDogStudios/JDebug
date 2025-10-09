#pragma once

class nsRemoteInterface;

/// \brief Interface to give access to the FileServe client for additional tooling needs.
///
/// For now, this interface just gives access to the nsRemoteInterface that is used to communicate with the FileServe server.
/// This allows for maximum flexibility sending and receiving custom messages.
class nsRemoteToolingInterface
{
public:
  virtual nsRemoteInterface* GetRemoteInterface() = 0;
};
