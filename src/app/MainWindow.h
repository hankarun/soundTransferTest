#pragma once

#include <QMainWindow>

class QComboBox;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QProgressBar;
class QLabel;

namespace sound { class AudioEngine; }

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void toggle();
    void onError(const QString& message);

private:
    void buildUi();
    void populateDevices();
    void setRunning(bool running);

    sound::AudioEngine* engine_ = nullptr;

    QComboBox*    inputDevices_  = nullptr;
    QComboBox*    outputDevices_ = nullptr;
    QLineEdit*    peerHost_      = nullptr;
    QSpinBox*     localPort_     = nullptr;
    QSpinBox*     peerPort_      = nullptr;
    QPushButton*  startStop_     = nullptr;
    QProgressBar* txMeter_       = nullptr;
    QProgressBar* rxMeter_       = nullptr;
    QLabel*       status_        = nullptr;
};
