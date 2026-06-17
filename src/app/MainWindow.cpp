#include "MainWindow.h"

#include "AudioEngine.h"
#include "AudioCapture.h"
#include "AudioPlayback.h"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHostAddress>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , engine_(new sound::AudioEngine(this))
{
    buildUi();
    populateDevices();

    connect(engine_, &sound::AudioEngine::txLevel, this, [this](qreal rms) {
        txMeter_->setValue(int(rms * 100));
    });
    connect(engine_, &sound::AudioEngine::rxLevel, this, [this](qreal rms) {
        rxMeter_->setValue(int(rms * 100));
    });
    connect(engine_, &sound::AudioEngine::errorOccurred, this, &MainWindow::onError);
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    auto* devBox = new QGroupBox(tr("Devices"), central);
    auto* devForm = new QFormLayout(devBox);
    inputDevices_  = new QComboBox(devBox);
    outputDevices_ = new QComboBox(devBox);
    devForm->addRow(tr("Microphone:"), inputDevices_);
    devForm->addRow(tr("Speaker:"), outputDevices_);

    auto* netBox = new QGroupBox(tr("Network"), central);
    auto* netForm = new QFormLayout(netBox);
    peerHost_  = new QLineEdit(QStringLiteral("127.0.0.1"), netBox);
    localPort_ = new QSpinBox(netBox);
    localPort_->setRange(1, 65535);
    localPort_->setValue(5000);
    peerPort_ = new QSpinBox(netBox);
    peerPort_->setRange(1, 65535);
    peerPort_->setValue(5000);
    netForm->addRow(tr("Peer IP:"), peerHost_);
    netForm->addRow(tr("Local port:"), localPort_);
    netForm->addRow(tr("Peer port:"), peerPort_);

    auto* meterBox = new QGroupBox(tr("Levels"), central);
    auto* meterForm = new QFormLayout(meterBox);
    txMeter_ = new QProgressBar(meterBox);
    txMeter_->setRange(0, 100);
    rxMeter_ = new QProgressBar(meterBox);
    rxMeter_->setRange(0, 100);
    meterForm->addRow(tr("Mic (TX):"), txMeter_);
    meterForm->addRow(tr("Speaker (RX):"), rxMeter_);

    startStop_ = new QPushButton(tr("Start"), central);
    startStop_->setCheckable(true);
    connect(startStop_, &QPushButton::clicked, this, &MainWindow::toggle);

    status_ = new QLabel(tr("Idle."), central);

    root->addWidget(devBox);
    root->addWidget(netBox);
    root->addWidget(meterBox);
    root->addWidget(startStop_);
    root->addWidget(status_);

    setCentralWidget(central);
    setWindowTitle(tr("Sound Transfer"));
}

void MainWindow::populateDevices()
{
    for (const auto& dev : sound::AudioCapture::availableDevices())
        inputDevices_->addItem(dev.deviceName(), QVariant::fromValue(dev));
    for (const auto& dev : sound::AudioPlayback::availableDevices())
        outputDevices_->addItem(dev.deviceName(), QVariant::fromValue(dev));
}

void MainWindow::toggle()
{
    if (engine_->isRunning()) {
        engine_->stop();
        setRunning(false);
        return;
    }

    const QString hostText = peerHost_->text().trimmed();
    QHostAddress peer(hostText);
    if (peer.isNull()) {
        onError(tr("Invalid peer IP address: %1").arg(hostText));
        startStop_->setChecked(false);
        return;
    }

    QAudioDeviceInfo in  = inputDevices_->currentData().value<QAudioDeviceInfo>();
    QAudioDeviceInfo out = outputDevices_->currentData().value<QAudioDeviceInfo>();

    if (engine_->start(quint16(localPort_->value()), peer,
                       quint16(peerPort_->value()), in, out)) {
        setRunning(true);
    } else {
        startStop_->setChecked(false);
    }
}

void MainWindow::setRunning(bool running)
{
    startStop_->setChecked(running);
    startStop_->setText(running ? tr("Stop") : tr("Start"));
    inputDevices_->setEnabled(!running);
    outputDevices_->setEnabled(!running);
    peerHost_->setEnabled(!running);
    localPort_->setEnabled(!running);
    peerPort_->setEnabled(!running);
    status_->setText(running ? tr("Running — talking to %1:%2")
                                   .arg(peerHost_->text()).arg(peerPort_->value())
                             : tr("Idle."));
    if (!running) {
        txMeter_->setValue(0);
        rxMeter_->setValue(0);
    }
}

void MainWindow::onError(const QString& message)
{
    QMessageBox::warning(this, tr("Sound Transfer"), message);
    status_->setText(message);
}
