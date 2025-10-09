#include "MainWindow.h"

#ifdef _WIN32
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#endif

#include "JDebugViewportWidget.h"
#include "LogDockWidget.h"

#include <QAbstractItemView>
#include <QAction>
#include <QByteArray>
#include <QDockWidget>
#include <QFileDialog>
#include <QMenu>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QStringList>
#include <QStatusBar>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

#include <Foundation/Logging/Log.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Time/Time.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

namespace
{
  constexpr double s_fDefaultPlaybackFps = 60.0;
}

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
  setWindowTitle(tr("JDebug"));
  resize(1280, 800);

  (void)nsFileSystem::DetectSdkRootDirectory();

  m_SessionFrameHandler = nsMakeDelegate(&MainWindow::AppendLiveFrame, this);
  m_SessionClipHandler = nsMakeDelegate(&MainWindow::ReplaceClip, this);

  m_RecordSettings.Reset();
  m_RecordSettings.m_TargetFrameInterval = nsTime::MakeFromSeconds(1.0 / s_fDefaultPlaybackFps);
  m_RecordSettings.m_bCaptureSleepingBodies = true;
  m_RecordSettings.m_bRecordVelocities = true;

  InitializeUi();

  m_PlaybackTimer = new QTimer(this);
  m_PlaybackTimer->setInterval(static_cast<int>(1000.0 / s_fDefaultPlaybackFps));
  connect(m_PlaybackTimer, &QTimer::timeout, this, &MainWindow::OnPlaybackTick);

  UpdateTimelineControls();
  UpdateStatusBar();
}

MainWindow::~MainWindow()
{
  OnSessionDisconnect();
}

void MainWindow::InitializeUi()
{
  CreateMenus();
  CreateToolBar();
  CreateDockWidgets();
  CreateStatusBar();

  auto* centralWidget = new QWidget(this);
  auto* centralLayout = new QVBoxLayout();
  centralLayout->setContentsMargins(18, 18, 18, 18);
  centralLayout->setSpacing(12);

  m_ViewportWidget = new JDebugViewportWidget(this);
  m_ViewportWidget->setMinimumHeight(420);
  m_ViewportWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  centralLayout->addWidget(m_ViewportWidget, 2);

  auto* playbackLayout = new QHBoxLayout();
  playbackLayout->setContentsMargins(0, 0, 0, 0);
  playbackLayout->setSpacing(10);

  m_PlayButton = new QPushButton(tr("Play"), this);
  auto* stopButton = new QPushButton(tr("Stop"), this);
  m_RecordButton = new QPushButton(tr("Record Live"), this);
  m_RetryRendererButton = new QPushButton(tr("Retry Renderer"), this);

  m_PlayButton->setMinimumWidth(90);
  stopButton->setMinimumWidth(90);
  m_RecordButton->setMinimumWidth(120);
  m_RetryRendererButton->setMinimumWidth(130);
  m_RetryRendererButton->setEnabled(false);

  playbackLayout->addWidget(m_PlayButton);
  playbackLayout->addWidget(stopButton);
  playbackLayout->addWidget(m_RecordButton);
  playbackLayout->addWidget(m_RetryRendererButton);

  m_TimeSlider = new QSlider(Qt::Horizontal, this);
  m_TimeSlider->setEnabled(false);
  m_TimeSlider->setFixedHeight(24);
  playbackLayout->addWidget(m_TimeSlider, 1);

  centralLayout->addLayout(playbackLayout);

  m_BodyTable = new QTableWidget(this);
  m_BodyTable->setColumnCount(6);
  QStringList headers;
  headers << tr("Body ID") << tr("Position") << tr("Rotation") << tr("Linear Velocity") << tr("Angular Velocity") << tr("State");
  m_BodyTable->setHorizontalHeaderLabels(headers);
  m_BodyTable->horizontalHeader()->setStretchLastSection(true);
  m_BodyTable->verticalHeader()->setVisible(false);
  m_BodyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_BodyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_BodyTable->setSelectionMode(QAbstractItemView::SingleSelection);
  m_BodyTable->setAlternatingRowColors(true);
  m_BodyTable->setShowGrid(false);
  m_BodyTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

  centralLayout->addWidget(m_BodyTable, 1);

  centralWidget->setLayout(centralLayout);
  setCentralWidget(centralWidget);

  connect(m_PlayButton, &QPushButton::clicked, this, &MainWindow::OnPlayPause);
  connect(stopButton, &QPushButton::clicked, this, &MainWindow::OnStopPlayback);
  connect(m_RecordButton, &QPushButton::clicked, this, &MainWindow::OnToggleRecording);
  connect(m_RetryRendererButton, &QPushButton::clicked, this, &MainWindow::OnRetryRenderer);
  connect(m_TimeSlider, &QSlider::valueChanged, this, &MainWindow::OnTimelineChanged);

  connect(m_ViewportWidget, &JDebugViewportWidget::RendererStateChanged, this, [this](bool /*initialized*/, bool bFailed) {
    if (!m_RetryRendererButton)
      return;

    m_RetryRendererButton->setEnabled(bFailed);
  });

  m_RetryRendererButton->setEnabled(m_ViewportWidget->HasRendererFailed());
}

void MainWindow::CreateMenus()
{
  auto* fileMenu = menuBar()->addMenu(tr("&File"));
  m_OpenAction = fileMenu->addAction(tr("&Open Recording..."));
  m_OpenAction->setShortcut(QKeySequence::Open);
  connect(m_OpenAction, &QAction::triggered, this, &MainWindow::OnOpenRecording);

  m_SaveAction = fileMenu->addAction(tr("&Save Recording As..."));
  m_SaveAction->setShortcut(QKeySequence::SaveAs);
  connect(m_SaveAction, &QAction::triggered, this, &MainWindow::OnSaveRecording);
  m_SaveAction->setEnabled(false);

  m_LoadSampleAction = fileMenu->addAction(tr("Load Sample Recording"));
  connect(m_LoadSampleAction, &QAction::triggered, this, &MainWindow::OnLoadSampleRecording);

  fileMenu->addSeparator();
  m_ExitAction = fileMenu->addAction(tr("E&xit"));
  m_ExitAction->setShortcut(QKeySequence::Quit);
  connect(m_ExitAction, &QAction::triggered, this, &QWidget::close);

  auto* sessionMenu = menuBar()->addMenu(tr("&Session"));
  m_ConnectAction = sessionMenu->addAction(tr("Connect..."));
  connect(m_ConnectAction, &QAction::triggered, this, &MainWindow::OnSessionConnect);

  m_DisconnectAction = sessionMenu->addAction(tr("Disconnect"));
  connect(m_DisconnectAction, &QAction::triggered, this, &MainWindow::OnSessionDisconnect);
  m_DisconnectAction->setEnabled(false);

  m_ViewMenu = menuBar()->addMenu(tr("&View"));

  auto* helpMenu = menuBar()->addMenu(tr("&Help"));
  m_AboutAction = helpMenu->addAction(tr("&About"));
  connect(m_AboutAction, &QAction::triggered, this, [this]() {
    QMessageBox::about(this, tr("About JDebug"),
      tr("<b>JDebug</b><br/>Visual debugger for the Jolt Physics Engine.\n"
         "Connect to a running simulation to inspect live body transforms, or open \".jvdrec\" recordings."
         "<br/><br/>Developed by: <a href=\"https://wdstudios.tech\">WD Studios</a> & <a href=\"https://scrumpysfindings.dev/\">Mikael K. Aboagye</a>."));
  });
}

void MainWindow::CreateToolBar()
{
  auto* toolbar = addToolBar(tr("Main"));
  toolbar->setMovable(false);
  toolbar->addAction(m_OpenAction);
  toolbar->addAction(m_SaveAction);
  toolbar->addSeparator();
  toolbar->addAction(m_ConnectAction);
  toolbar->addAction(m_DisconnectAction);
}

void MainWindow::CreateDockWidgets()
{
  auto* infoWidget = new QWidget(this);
  auto* layout = new QVBoxLayout();
  layout->setContentsMargins(16, 16, 16, 16);
  layout->setSpacing(12);

  auto* label = new QLabel(tr("Use Session > Connect to attach to a Jolt instance. Incoming frames will populate the live preview."
                              " Toggle \"Record Live\" to persist the stream into a .jvdrec clip."), this);
  label->setWordWrap(true);
  layout->addWidget(label);
  layout->addStretch(1);
  infoWidget->setLayout(layout);

  m_GuidanceDock = new QDockWidget(tr("Guidance"), this);
  m_GuidanceDock->setObjectName(QStringLiteral("GuidanceDock"));
  m_GuidanceDock->setWidget(infoWidget);
  addDockWidget(Qt::RightDockWidgetArea, m_GuidanceDock);

  m_LogDockWidget = new LogDockWidget(this);
  m_LogDockWidget->setObjectName(QStringLiteral("LogDock"));
  m_LogDockWidget->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
  m_LogDockWidget->setMinimumHeight(160);
  addDockWidget(Qt::BottomDockWidgetArea, m_LogDockWidget);

  if (m_ViewMenu)
  {
    m_ViewMenu->addAction(m_GuidanceDock->toggleViewAction());
    m_ViewMenu->addAction(m_LogDockWidget->toggleViewAction());
  }
}

void MainWindow::CreateStatusBar()
{
  m_StatusLabel = new QLabel(this);
  statusBar()->addPermanentWidget(m_StatusLabel, 1);
}

void MainWindow::UpdateTimelineControls()
{
  const nsUInt32 frameCount = m_CurrentClip.GetFrames().GetCount();
  if (frameCount > 0)
  {
    m_TimeSlider->setEnabled(true);
    m_TimeSlider->setRange(0, static_cast<int>(frameCount - 1));
  }
  else
  {
    m_TimeSlider->setEnabled(false);
    m_TimeSlider->setRange(0, 0);
  }

  m_SaveAction->setEnabled(frameCount > 0);
}

void MainWindow::UpdateStatusBar()
{
  QString sessionState = m_Session.IsRunning() ? tr("Connected") : tr("Disconnected");
  QString framesInfo = tr("Frames: %1").arg(m_CurrentClip.GetFrames().GetCount());
  const nsTime duration = m_CurrentClip.GetDuration();
  QString durationInfo = tr("Duration: %1 s").arg(QString::number(duration.GetSeconds(), 'f', 2));

  QString status = tr("%1 | %2 | %3").arg(sessionState, framesInfo, durationInfo);
  m_StatusLabel->setText(status);
}

void MainWindow::OnOpenRecording()
{
  const QString path = PromptRecordingPath(false);
  if (path.isEmpty())
    return;

  nsJvdClip clip;
  if (nsJvdSerialization::LoadClipFromFile(path.toStdString().c_str(), clip).Failed())
  {
    QMessageBox::critical(this, tr("Failed to open"), tr("Unable to load recording from %1").arg(path));
    return;
  }

  SetClip(std::move(clip));
  UpdateStatusBar();
}

void MainWindow::OnSaveRecording()
{
  if (m_CurrentClip.IsEmpty())
    return;

  const QString path = PromptRecordingPath(true);
  if (path.isEmpty())
    return;

  if (nsJvdSerialization::SaveClipToFile(path.toStdString().c_str(), m_CurrentClip).Failed())
  {
    QMessageBox::critical(this, tr("Failed to save"), tr("Unable to save recording to %1").arg(path));
  }
}

void MainWindow::OnLoadSampleRecording()
{
  nsJvdClip clip = CreateSampleClip();
  const nsUInt64 uiFrameCount = clip.GetFrames().GetCount();

  SetClip(std::move(clip));
  UpdateStatusBar();

  nsLog::Success("Loaded sample recording with {0} frames", uiFrameCount);
}

void MainWindow::OnPlayPause()
{
  if (m_CurrentClip.IsEmpty())
    return;

  m_bIsPlaying = !m_bIsPlaying;
  m_PlayButton->setText(m_bIsPlaying ? tr("Pause") : tr("Play"));

  if (m_bIsPlaying)
  {
    const nsDynamicArray<nsJvdFrame>& frames = m_CurrentClip.GetFrames();
    const nsTime sampleInterval = m_CurrentClip.GetSampleInterval();
    nsTime startTime = nsTime::MakeZero();
    const int index = m_TimeSlider->value();
    if (index >= 0 && index < static_cast<int>(frames.GetCount()))
    {
      startTime = frames[index].m_Timestamp;
      if (sampleInterval.IsPositive() && startTime >= sampleInterval)
      {
        startTime -= sampleInterval;
      }
      else
      {
        startTime = nsTime::MakeZero();
      }
    }

    m_PlaybackController.LoadClip(m_CurrentClip);
    m_PlaybackController.SetPlaybackPosition(startTime);
    m_PlaybackTimer->start();
  }
  else
  {
    m_PlaybackTimer->stop();
  }
}

void MainWindow::OnStopPlayback()
{
  if (!m_bIsPlaying && m_TimeSlider->value() == 0)
    return;

  m_bIsPlaying = false;
  m_PlayButton->setText(tr("Play"));
  m_PlaybackTimer->stop();
  m_PlaybackController.Reset();
  if (!m_CurrentClip.IsEmpty())
  {
    const nsJvdFrame& frame = m_CurrentClip.GetFrames()[0];
    m_CurrentFrame = frame;
    m_TimeSlider->blockSignals(true);
    m_TimeSlider->setValue(0);
    m_TimeSlider->blockSignals(false);
    UpdateBodyTable(frame);
  }
}

void MainWindow::OnTimelineChanged(int value)
{
  if (m_CurrentClip.IsEmpty())
    return;

  const nsDynamicArray<nsJvdFrame>& frames = m_CurrentClip.GetFrames();
  if (value < 0 || value >= static_cast<int>(frames.GetCount()))
    return;

  m_CurrentFrame = frames[value];
  UpdateBodyTable(m_CurrentFrame);
}

void MainWindow::OnSessionConnect()
{
  nsJvdSessionConfiguration cfg = PromptSessionConfiguration();
  if (cfg.m_sEndpoint.IsEmpty())
    return;

  if (m_Session.Initialize(cfg).Failed())
  {
    QMessageBox::critical(this, tr("Connection failed"), tr("Unable to connect to %1:%2")
      .arg(QString::fromUtf8(cfg.m_sEndpoint.GetData()), QString::number(cfg.m_uiPort)));
    return;
  }

  ConnectSessionHandlers();
  m_ConnectAction->setEnabled(false);
  m_DisconnectAction->setEnabled(true);
  UpdateStatusBar();
}

void MainWindow::OnSessionDisconnect()
{
  DisconnectSessionHandlers();
  m_Session.Shutdown();
  m_ConnectAction->setEnabled(true);
  m_DisconnectAction->setEnabled(false);
  UpdateStatusBar();
}

void MainWindow::OnPlaybackTick()
{
  if (m_CurrentClip.IsEmpty())
    return;

  nsTime delta = m_CurrentClip.GetSampleInterval();
  if (!delta.IsPositive())
  {
    delta = nsTime::MakeFromSeconds(1.0 / s_fDefaultPlaybackFps);
  }

  nsJvdFrame frame;
  if (!m_PlaybackController.Step(delta, frame))
  {
    if (!m_PlaybackController.GetLoop())
    {
      OnStopPlayback();
    }
    return;
  }

  m_CurrentFrame = frame;
  UpdateBodyTable(m_CurrentFrame);

  const int index = static_cast<int>(frame.m_uiFrameIndex);
  if (index >= 0 && index <= m_TimeSlider->maximum())
  {
    m_TimeSlider->blockSignals(true);
    m_TimeSlider->setValue(index);
    m_TimeSlider->blockSignals(false);
  }
}

void MainWindow::OnToggleRecording()
{
  m_bRecordingLive = !m_bRecordingLive;

  if (m_bRecordingLive)
  {
    m_LiveMetadata.Reset();
    m_LiveMetadata.m_sClipName = "Live Capture";
    m_LiveMetadata.m_sAuthor = "JDebug";
    m_Recorder.StartRecording(m_RecordSettings, m_LiveMetadata);
    m_RecordButton->setText(tr("Stop Recording"));
  }
  else
  {
    m_RecordButton->setText(tr("Record Live"));

    nsJvdClip clip;
    if (m_Recorder.StopRecording(clip).Succeeded() && !clip.IsEmpty())
    {
      SetClip(std::move(clip));
      UpdateStatusBar();
    }
  }
}

void MainWindow::OnRetryRenderer()
{
  if (m_ViewportWidget)
  {
    m_ViewportWidget->RetryRendererInitialization();
  }
}

void MainWindow::UpdateBodyTable(const nsJvdFrame& frame)
{
  const nsUInt32 count = frame.m_Bodies.GetCount();
  m_BodyTable->setRowCount(static_cast<int>(count));

  for (nsUInt32 i = 0; i < count; ++i)
  {
    const nsJvdBodyState& state = frame.m_Bodies[i];

    auto setItem = [this, i](int column, const QString& text) {
      auto* item = new QTableWidgetItem(text);
      item->setFlags(item->flags() ^ Qt::ItemIsEditable);
      m_BodyTable->setItem(static_cast<int>(i), column, item);
    };

    setItem(0, QString::number(static_cast<qulonglong>(state.m_uiBodyId)));
    setItem(1, QStringLiteral("%1, %2, %3").arg(state.m_vPosition.x, 0, 'f', 2).arg(state.m_vPosition.y, 0, 'f', 2).arg(state.m_vPosition.z, 0, 'f', 2));
    setItem(2, QStringLiteral("%1, %2, %3, %4")
                 .arg(state.m_qRotation.x, 0, 'f', 2)
                 .arg(state.m_qRotation.y, 0, 'f', 2)
                 .arg(state.m_qRotation.z, 0, 'f', 2)
                 .arg(state.m_qRotation.w, 0, 'f', 2));
    setItem(3, QStringLiteral("%1, %2, %3").arg(state.m_vLinearVelocity.x, 0, 'f', 2).arg(state.m_vLinearVelocity.y, 0, 'f', 2).arg(state.m_vLinearVelocity.z, 0, 'f', 2));
    setItem(4, QStringLiteral("%1, %2, %3").arg(state.m_vAngularVelocity.x, 0, 'f', 2).arg(state.m_vAngularVelocity.y, 0, 'f', 2).arg(state.m_vAngularVelocity.z, 0, 'f', 2));

    const QString stateLabel = state.m_bIsSleeping ? tr("Sleeping") : tr("Active");
    setItem(5, stateLabel);
  }

  m_BodyTable->resizeColumnsToContents();

  if (m_ViewportWidget)
  {
    m_ViewportWidget->DisplayFrame(frame);
  }
}

void MainWindow::SetClip(nsJvdClip clip)
{
  m_CurrentClip = std::move(clip);
  m_PlaybackController.LoadClip(m_CurrentClip);
  m_PlaybackController.Reset();

  m_bIsPlaying = false;
  if (m_PlaybackTimer)
  {
    m_PlaybackTimer->stop();
  }
  if (m_PlayButton)
  {
    m_PlayButton->setText(tr("Play"));
  }

  UpdateTimelineControls();
  UpdateStatusBar();

  if (!m_CurrentClip.IsEmpty())
  {
    const nsJvdFrame& firstFrame = m_CurrentClip.GetFrames()[0];
    m_TimeSlider->blockSignals(true);
    m_TimeSlider->setValue(0);
    m_TimeSlider->blockSignals(false);
    UpdateBodyTable(firstFrame);
    m_CurrentFrame = firstFrame;
  }
  else
  {
    m_CurrentFrame = nsJvdFrame();
    m_BodyTable->setRowCount(0);
    if (m_ViewportWidget)
    {
      m_ViewportWidget->DisplayFrame(m_CurrentFrame);
    }
  }
}

void MainWindow::AppendLiveFrame(const nsJvdFrame& frame)
{
  if (m_bRecordingLive)
  {
    nsArrayPtr<const nsJvdBodyState> states(frame.m_Bodies.GetData(), frame.m_Bodies.GetCount());
    m_Recorder.AppendFrame(frame.m_Timestamp, states);
  }

  m_CurrentFrame = frame;
  nsJvdFrame copy = frame;
  if (m_CurrentClip.IsEmpty())
  {
    nsJvdClip clip;
    clip.SetMetadata(m_LiveMetadata);
    clip.AddFrame(std::move(copy));
    SetClip(std::move(clip));
  }
  else
  {
    m_CurrentClip.AddFrame(std::move(copy));
    UpdateTimelineControls();
  }

  UpdateBodyTable(frame);

  if (!m_CurrentClip.IsEmpty())
  {
    const int lastIndex = static_cast<int>(m_CurrentClip.GetFrames().GetCount() - 1);
    m_TimeSlider->blockSignals(true);
    m_TimeSlider->setValue(lastIndex);
    m_TimeSlider->blockSignals(false);
  }
}

void MainWindow::ReplaceClip(const nsJvdClip& clip)
{
  SetClip(clip);
  UpdateStatusBar();
}

nsJvdClip MainWindow::CreateSampleClip() const
{
  nsJvdClip clip;

  nsJvdClipMetadata metadata;
  metadata.Reset();
  metadata.m_sClipName = "Sample Recording";
  metadata.m_sAuthor = "JDebug";
  metadata.m_sSourceHost = "Synthetic";
  metadata.m_SampleInterval = nsTime::MakeFromSeconds(1.0 / s_fDefaultPlaybackFps);
  clip.SetMetadata(metadata);

  constexpr nsUInt32 uiFrameCount = 240;
  constexpr nsUInt32 uiBodyCount = 3;

  nsTime timestamp = nsTime::MakeZero();
  const nsTime sampleInterval = metadata.m_SampleInterval;

  for (nsUInt32 frameIndex = 0; frameIndex < uiFrameCount; ++frameIndex)
  {
    nsJvdFrame frame;
    frame.m_uiFrameIndex = frameIndex;
    frame.m_Timestamp = timestamp;
    frame.m_Bodies.SetCount(uiBodyCount);

    for (nsUInt32 bodyIndex = 0; bodyIndex < uiBodyCount; ++bodyIndex)
    {
      nsJvdBodyState& state = frame.m_Bodies[bodyIndex];
      state.m_uiBodyId = 100 + bodyIndex;

      const float offset = static_cast<float>(bodyIndex) * (2.0f * nsMath::Pi<float>() / static_cast<float>(uiBodyCount));
      const float angle = static_cast<float>(frameIndex) * 0.06f + offset;
      const float radius = 2.5f + static_cast<float>(bodyIndex);

  const nsAngle radAngle = nsAngle::MakeFromRadian(angle);
  const float cosVal = nsMath::Cos(radAngle);
  const float sinVal = nsMath::Sin(radAngle);

  state.m_vPosition = nsVec3(cosVal * radius, sinVal * radius, static_cast<float>(bodyIndex) * 1.2f);
  state.m_qRotation = nsQuat::MakeFromAxisAndAngle(nsVec3(0.0f, 0.0f, 1.0f), nsAngle::MakeFromRadian(angle * 0.4f));
  state.m_vLinearVelocity = nsVec3(-sinVal, cosVal, 0.0f) * (radius * 0.45f);
      state.m_vAngularVelocity = nsVec3(0.0f, 0.0f, 0.35f + static_cast<float>(bodyIndex) * 0.1f);
      state.m_bIsSleeping = false;
    }

    clip.AddFrame(std::move(frame));
    timestamp += sampleInterval;
  }

  return clip;
}

void MainWindow::ConnectSessionHandlers()
{
  if (!m_Session.IsRunning())
    return;

  m_Session.OnFrameReceived().AddEventHandler(m_SessionFrameHandler);
  m_Session.OnClipReceived().AddEventHandler(m_SessionClipHandler);
}

void MainWindow::DisconnectSessionHandlers()
{
  if (!m_Session.IsRunning())
    return;

  m_Session.OnFrameReceived().RemoveEventHandler(m_SessionFrameHandler);
  m_Session.OnClipReceived().RemoveEventHandler(m_SessionClipHandler);
}

nsJvdSessionConfiguration MainWindow::PromptSessionConfiguration()
{
  nsJvdSessionConfiguration cfg;
  cfg.m_bStartAsServer = false;
  cfg.m_sSessionName = "JDebug Client";

  bool ok = false;
  const QString host = QInputDialog::getText(this, tr("Connect to Session"), tr("Host"),
    QLineEdit::Normal, QStringLiteral("localhost"), &ok);
  if (!ok || host.isEmpty())
    return cfg;

  const int port = QInputDialog::getInt(this, tr("Connect to Session"), tr("Port"), 1040, 1, 65535, 1, &ok);
  if (!ok)
    return cfg;

  const QByteArray hostUtf8 = host.toUtf8();
  cfg.m_sEndpoint = hostUtf8.constData();
  cfg.m_uiPort = static_cast<nsUInt16>(port);
  return cfg;
}

QString MainWindow::PromptRecordingPath(bool bSave)
{
  QString filter = tr("JDebug Recordings (*.jvdrec)");
  if (bSave)
  {
    QString path = QFileDialog::getSaveFileName(this, tr("Save Recording"), QString(), filter);
    if (!path.isEmpty() && !path.endsWith(QStringLiteral(".jvdrec"), Qt::CaseInsensitive))
    {
      path.append(QStringLiteral(".jvdrec"));
    }
    return path;
  }
  else
  {
    return QFileDialog::getOpenFileName(this, tr("Open Recording"), QString(), filter);
  }
}

NS_STATICLINK_FILE(JDebugApp, MainWindow);
