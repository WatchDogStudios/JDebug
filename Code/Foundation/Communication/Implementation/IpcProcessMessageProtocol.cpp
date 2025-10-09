#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/IpcProcessMessageProtocol.h>
// #include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
// #include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

nsIpcProcessMessageProtocol::nsIpcProcessMessageProtocol(nsIpcChannel* pChannel)
{
  m_pChannel = pChannel;
  m_pChannel->SetReceiveCallback(nsMakeDelegate(&nsIpcProcessMessageProtocol::ReceiveMessageData, this));
}

nsIpcProcessMessageProtocol::~nsIpcProcessMessageProtocol()
{
  m_pChannel->SetReceiveCallback({});

  while (nsUniquePtr<nsProcessMessage> msg = PopMessage())
  {
  }
}

bool nsIpcProcessMessageProtocol::Send(nsProcessMessage* pMsg)
{
  nsContiguousMemoryStreamStorage storage;
  nsMemoryStreamWriter writer(&storage);
  nsReflectionSerializer::WriteObjectToBinary(writer, pMsg->GetDynamicRTTI(), pMsg);
  return m_pChannel->Send(nsArrayPtr<const nsUInt8>(storage.GetData(), storage.GetStorageSize32()));
}

bool nsIpcProcessMessageProtocol::ProcessMessages()
{
  bool messagesPresent = false;

  while (nsUniquePtr<nsProcessMessage> msg = PopMessage())
  {
    messagesPresent = true;
    Event e;
    e.m_pMessage = msg.Borrow();
    e.m_bInterruptMessageProcessing = false;
    m_MessageEvent.Broadcast(e);
    if (e.m_bInterruptMessageProcessing)
      break;
  }

  return messagesPresent;
}

nsResult nsIpcProcessMessageProtocol::WaitForMessages(nsTime timeout)
{
  // Message processing can be interrupted via the m_bInterruptMessageProcessing flag. Thus, there is no guarantee that the queue is empty at this point. Only wait if the queue is empty.
  if (ProcessMessages())
  {
    return NS_SUCCESS;
  }

  nsResult res = m_pChannel->WaitForMessages(timeout);
  if (res.Succeeded())
  {
    ProcessMessages();
  }
  return res;
}

void nsIpcProcessMessageProtocol::ReceiveMessageData(nsArrayPtr<const nsUInt8> data)
{
  // Message complete, de-serialize
  nsRawMemoryStreamReader reader(data.GetPtr(), data.GetCount());
  const nsRTTI* pRtti = nullptr;

  nsProcessMessage* pMsg = (nsProcessMessage*)nsReflectionSerializer::ReadObjectFromBinary(reader, pRtti);
  nsUniquePtr<nsProcessMessage> msg(pMsg, nsFoundation::GetDefaultAllocator());
  if (msg != nullptr)
  {
    EnqueueMessage(std::move(msg));
  }
  else
  {
    nsLog::Error("Channel received invalid Message!");
  }
}

void nsIpcProcessMessageProtocol::EnqueueMessage(nsUniquePtr<nsProcessMessage>&& msg)
{
  NS_LOCK(m_IncomingQueueMutex);
  m_IncomingQueue.PushBack(std::move(msg));
}

nsUniquePtr<nsProcessMessage> nsIpcProcessMessageProtocol::PopMessage()
{
  NS_LOCK(m_IncomingQueueMutex);
  if (m_IncomingQueue.IsEmpty())
    return {};

  nsUniquePtr<nsProcessMessage> front = std::move(m_IncomingQueue.PeekFront());
  m_IncomingQueue.PopFront();
  return front;
}
