#include <QApplication>
#include <QByteArray>
#include <QVector>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>
#include <QColor>
#include <QIODevice>

#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

#include "MainWindow.h"

class nsJDebugApplication : public nsApplication
{
public:
  using SUPER = nsApplication;

  nsJDebugApplication()
    : nsApplication("JDebug")
  {
  }

  virtual nsResult BeforeCoreSystemsStartup() override
  {
    nsStartup::AddApplicationTag("tool");
    nsStartup::AddApplicationTag("jdebug");
    return SUPER::BeforeCoreSystemsStartup();
  }

  virtual void Run() override
  {
    if (m_bRanOnce)
    {
      RequestApplicationQuit();
      return;
    }
    
    m_bRanOnce = true;

    const nsUInt32 argumentCount = GetArgumentCount();
    QVector<QByteArray> argumentStorage;
    argumentStorage.reserve(argumentCount);

    QVector<char*> argumentPointers;
    argumentPointers.reserve(argumentCount + 1);

    const char** engineArguments = GetArgumentsArray();
    for (nsUInt32 i = 0; i < argumentCount; ++i)
    {
      argumentStorage.push_back(QByteArray(engineArguments[i]));
      argumentPointers.push_back(argumentStorage.back().data());
    }
    argumentPointers.push_back(nullptr);

    int qtArgc = static_cast<int>(argumentCount);

    QApplication::setOrganizationName("WD Studios");
    QApplication::setApplicationName("JDebug");

    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QApplication app(qtArgc, argumentPointers.data());

    QPalette palette;
    palette.setColor(QPalette::Window, QColor(0x1e, 0x1f, 0x22));
    palette.setColor(QPalette::WindowText, QColor(0xf2, 0xf4, 0xf7));
    palette.setColor(QPalette::Base, QColor(0x23, 0x24, 0x29));
    palette.setColor(QPalette::AlternateBase, QColor(0x1b, 0x1c, 0x20));
    palette.setColor(QPalette::ToolTipBase, QColor(0x23, 0x24, 0x29));
    palette.setColor(QPalette::ToolTipText, QColor(0xf2, 0xf4, 0xf7));
    palette.setColor(QPalette::Text, QColor(0xf2, 0xf4, 0xf7));
    palette.setColor(QPalette::Button, QColor(0x2a, 0x2b, 0x32));
    palette.setColor(QPalette::ButtonText, QColor(0xf2, 0xf4, 0xf7));
    palette.setColor(QPalette::BrightText, QColor(0xff, 0x55, 0x55));
    palette.setColor(QPalette::Highlight, QColor(0x3d, 0x7c, 0xff));
    palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    palette.setColor(QPalette::Link, QColor(0x5a, 0x8c, 0xff));
    palette.setColor(QPalette::PlaceholderText, QColor(0x9b, 0xa0, 0xaa));
    app.setPalette(palette);

    QFile styleFile(QStringLiteral(":/styles/dark.qss"));
    if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      const QString styleSheet = QString::fromUtf8(styleFile.readAll());
      app.setStyleSheet(styleSheet);
    }

    MainWindow mainWindow;
    mainWindow.show();

    SetReturnCode(app.exec());
    RequestApplicationQuit();
  }

private:
  bool m_bRanOnce = false;
};

NS_APPLICATION_ENTRY_POINT(nsJDebugApplication);
