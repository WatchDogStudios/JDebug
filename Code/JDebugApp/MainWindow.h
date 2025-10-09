#pragma once

#include <QMainWindow>

#include <JVDSDK/JVDSDK.h>

class QTimer;
class QSlider;
class QLabel;
class QTableWidget;
class QPushButton;
class QAction;
class QMenu;
class QDockWidget;
class JDebugViewportWidget;
class LogDockWidget;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

private slots:
  void OnOpenRecording();
  void OnSaveRecording();
  void OnLoadSampleRecording();
  void OnPlayPause();
  void OnStopPlayback();
  void OnTimelineChanged(int value);
  void OnSessionConnect();
  void OnSessionDisconnect();
  void OnPlaybackTick();
  void OnToggleRecording();
  void OnRetryRenderer();

private:
  void InitializeUi();
  void CreateMenus();
  void CreateToolBar();
  void CreateDockWidgets();
  void CreateStatusBar();
  nsJvdClip CreateSampleClip() const;
  void UpdateTimelineControls();
  void UpdateStatusBar();
  void UpdateBodyTable(const nsJvdFrame& frame);
  void SetClip(nsJvdClip clip);
  void AppendLiveFrame(const nsJvdFrame& frame);
  void ReplaceClip(const nsJvdClip& clip);

  void ConnectSessionHandlers();
  void DisconnectSessionHandlers();

  nsJvdSessionConfiguration PromptSessionConfiguration();
  QString PromptRecordingPath(bool bSave);

  nsJvdClip m_CurrentClip;
  nsJvdPlaybackController m_PlaybackController;
  nsJvdSession m_Session;
  nsJvdRecorder m_Recorder;
  nsJvdRecordingSettings m_RecordSettings;
  nsJvdClipMetadata m_LiveMetadata;
  nsJvdFrame m_CurrentFrame;

  nsEvent<const nsJvdFrame&, nsMutex>::Handler m_SessionFrameHandler;
  nsEvent<const nsJvdClip&, nsMutex>::Handler m_SessionClipHandler;

  QTimer* m_PlaybackTimer = nullptr;
  QSlider* m_TimeSlider = nullptr;
  QLabel* m_StatusLabel = nullptr;
  QTableWidget* m_BodyTable = nullptr;
  QPushButton* m_PlayButton = nullptr;
  QPushButton* m_RecordButton = nullptr;
  QPushButton* m_RetryRendererButton = nullptr;
  JDebugViewportWidget* m_ViewportWidget = nullptr;
  LogDockWidget* m_LogDockWidget = nullptr;
  QDockWidget* m_GuidanceDock = nullptr;
  QMenu* m_ViewMenu = nullptr;

  QAction* m_OpenAction = nullptr;
  QAction* m_SaveAction = nullptr;
  QAction* m_LoadSampleAction = nullptr;
  QAction* m_ConnectAction = nullptr;
  QAction* m_DisconnectAction = nullptr;
  QAction* m_ExitAction = nullptr;
  QAction* m_AboutAction = nullptr;

  bool m_bIsPlaying = false;
  bool m_bRecordingLive = false;
};
