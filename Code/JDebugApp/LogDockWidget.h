#pragma once

#include <QDockWidget>

#include <Foundation/Logging/Log.h>

class QPlainTextEdit;

/// \brief Dockable widget that mirrors nsLog output inside the JDebug UI.
class LogDockWidget : public QDockWidget
{
  Q_OBJECT

public:
  explicit LogDockWidget(QWidget* parent = nullptr);
  ~LogDockWidget() override;

signals:
  void LogLineReady(const QString& line);

private slots:
  void AppendLogLine(const QString& line);

private:
  void HandleLogMessage(const nsLoggingEventData& data);

  QPlainTextEdit* m_pTextEdit = nullptr;
  nsEventSubscriptionID m_LogSubscription = 0;
};
