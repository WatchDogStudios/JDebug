#include "LogDockWidget.h"

#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextOption>
#include <QVBoxLayout>

#include <Foundation/Types/Delegate.h>

namespace
{
  constexpr qsizetype s_iMaxLogBlocks = 1000;
}

LogDockWidget::LogDockWidget(QWidget* parent)
  : QDockWidget(parent)
{
  setObjectName(QStringLiteral("LogDockWidget"));
  setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
  setWindowTitle(tr("Log"));

  auto* container = new QWidget(this);
  auto* layout = new QVBoxLayout(container);
  layout->setContentsMargins(0, 0, 0, 0);

  m_pTextEdit = new QPlainTextEdit(container);
  m_pTextEdit->setReadOnly(true);
  m_pTextEdit->setWordWrapMode(QTextOption::NoWrap);
  m_pTextEdit->setContextMenuPolicy(Qt::NoContextMenu);
  m_pTextEdit->setPlaceholderText(tr("Application log output will appear here."));

  layout->addWidget(m_pTextEdit);
  container->setLayout(layout);
  setWidget(container);

  connect(this, &LogDockWidget::LogLineReady, this, &LogDockWidget::AppendLogLine, Qt::QueuedConnection);

  m_LogSubscription = nsGlobalLog::AddLogWriter(nsMakeDelegate(&LogDockWidget::HandleLogMessage, this));
}

LogDockWidget::~LogDockWidget()
{
  if (m_LogSubscription != 0)
  {
    nsGlobalLog::RemoveLogWriter(m_LogSubscription);
  }
}

void LogDockWidget::AppendLogLine(const QString& line)
{
  if (!m_pTextEdit)
    return;

  QPlainTextEdit* edit = m_pTextEdit;
  edit->appendPlainText(line);

  const qsizetype blockCount = edit->blockCount();
  if (blockCount > s_iMaxLogBlocks)
  {
    QTextCursor cursor(edit->document());
    cursor.setPosition(0);
    cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, blockCount - s_iMaxLogBlocks);
    cursor.removeSelectedText();
    cursor.deleteChar();
  }

  edit->verticalScrollBar()->setValue(edit->verticalScrollBar()->maximum());
}

void LogDockWidget::HandleLogMessage(const nsLoggingEventData& data)
{
  if (data.m_EventType == nsLogMsgType::Flush)
    return;

  if (data.m_EventType == nsLogMsgType::BeginGroup || data.m_EventType == nsLogMsgType::EndGroup)
  {
    return;
  }

  QString tag;
  if (!data.m_sTag.IsEmpty())
  {
    tag = QString::fromUtf8(data.m_sTag.GetStartPointer(), static_cast<qsizetype>(data.m_sTag.GetElementCount()));
  }

  const QString message = QString::fromUtf8(data.m_sText.GetStartPointer(), static_cast<qsizetype>(data.m_sText.GetElementCount()));

  QString prefix;
  switch (data.m_EventType)
  {
    case nsLogMsgType::ErrorMsg:
      prefix = QStringLiteral("[Error] ");
      break;
    case nsLogMsgType::SeriousWarningMsg:
      prefix = QStringLiteral("[Serious] ");
      break;
    case nsLogMsgType::WarningMsg:
      prefix = QStringLiteral("[Warning] ");
      break;
    case nsLogMsgType::SuccessMsg:
      prefix = QStringLiteral("[Success] ");
      break;
    case nsLogMsgType::DevMsg:
      prefix = QStringLiteral("[Dev] ");
      break;
    case nsLogMsgType::DebugMsg:
      prefix = QStringLiteral("[Debug] ");
      break;
    default:
      break;
  }

  QString indent;
  if (data.m_uiIndentation > 0)
  {
    indent = QString(static_cast<int>(data.m_uiIndentation) * 2, QChar(' '));
  }

  QString text;
  if (!tag.isEmpty())
  {
    text = prefix + indent + QStringLiteral("[") + tag + QStringLiteral("] ") + message;
  }
  else
  {
    text = prefix + indent + message;
  }

  Q_EMIT LogLineReady(text);
}
