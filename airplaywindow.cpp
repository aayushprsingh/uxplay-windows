#include "airplaywindow.h"

#include <renderers/video_renderer.h>
#include <uxplay.h>

#include <QCloseEvent>
#include <QDebug>
#include <QImage>
#include <QLabel>
#include <QMutexLocker>
#include <QPixmap>
#include <QTimer>
#include <QVBoxLayout>

// ─── Callbacks globali (C-style, devono stare fuori da classi) ────────────────

// Puntatore al thread corrente, usato dalle callback per emettere i segnali
static AirPlayServerThread *g_currentServerThread = nullptr;

static void frame_callback(const unsigned char *data,
                            int width, int height,
                            int stride, int format)
{
    Q_UNUSED(stride)
    Q_UNUSED(format)
    if (!g_currentServerThread)
        return;

    // Copia i dati del frame (RGB888 = width * height * 3 bytes)
    QByteArray frameData(reinterpret_cast<const char *>(data),
                         width * height * 3);
    emit g_currentServerThread->videoFrameReady(frameData, width, height);
}

static void connection_callback(bool connected)
{
    qDebug() << "[AirPlay]" << (connected ? "Connected" : "Disconnected");
    if (!g_currentServerThread)
        return;
    emit g_currentServerThread->clientConnectionChanged(connected);
}

// ─── AirPlayServerThread ─────────────────────────────────────────────────────

AirPlayServerThread::AirPlayServerThread(QObject *parent)
    : QThread(parent)
{
}

AirPlayServerThread::~AirPlayServerThread()
{
    uxplay_cleanup();
    wait(); // aspetta che run() finisca
}

void AirPlayServerThread::setArguments(const QStringList &args)
{
    QMutexLocker locker(&m_mutex);

    m_argData.clear();
    m_argv.clear();

    m_argData.append("uxplay"); // argv[0]
    for (const QString &arg : args)
        m_argData.append(arg.toUtf8());

    for (QByteArray &ba : m_argData)
        m_argv.append(ba.data());
}

void AirPlayServerThread::run()
{
    // Crea e registra un contesto GLib per questo thread
    // DEVE stare prima di init_uxplay, che chiama g_main_loop_new(NULL,…)
    GMainContext *ctx = g_main_context_new();
    g_main_context_push_thread_default(ctx);

    g_currentServerThread = this;

    callbacks_t callbacks;
    callbacks.frame_callback      = frame_callback;
    callbacks.connection_callback = connection_callback;
    uxplay_callbacks = &callbacks;

    emit statusChanged(true);
    qDebug() << "[AirPlay] Starting, argc =" << m_argv.size();

    int res = init_uxplay(m_argv.size(), m_argv.data());
    // init_uxplay ora crea il loop, lo avvia e ritorna solo quando
    // uxplay_stop() chiama g_main_loop_quit()

    qDebug() << "[AirPlay] Server exited, code =" << res;
    if (res != 0)
        emit errorOccurred(
            QString("AirPlay server exited with code %1").arg(res));

    uxplay_callbacks      = nullptr;
    g_currentServerThread = nullptr;

    g_main_context_pop_thread_default(ctx);
    g_main_context_unref(ctx);

    emit statusChanged(false);
}

// ─── AirPlayWindow ───────────────────────────────────────────────────────────

AirPlayWindow::AirPlayWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    // Piccolo delay prima di avviare, dà tempo a Qt di mostrare la finestra
    QTimer::singleShot(500, this, &AirPlayWindow::startAirPlayServer);
}

AirPlayWindow::~AirPlayWindow()
{
    stopAirPlayServer();
}

void AirPlayWindow::setupUI()
{
    setWindowTitle("AirPlay Receiver");
    resize(1280, 720);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);

    m_statusLabel = new QLabel("Starting AirPlay server...");
    m_statusLabel->setAlignment(Qt::AlignCenter);

    m_videoLabel = new QLabel();
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setStyleSheet("background-color: black;");

    layout->addWidget(m_statusLabel);
    layout->addWidget(m_videoLabel, 1);
}

void AirPlayWindow::startAirPlayServer()
{
    if (m_serverRunning)
        return;

    m_serverThread = new AirPlayServerThread(this);

    connect(m_serverThread, &AirPlayServerThread::statusChanged,
            this, &AirPlayWindow::onServerStatusChanged);
    connect(m_serverThread, &AirPlayServerThread::videoFrameReady,
            this, &AirPlayWindow::updateVideoFrame);
    connect(m_serverThread, &AirPlayServerThread::clientConnectionChanged,
            this, &AirPlayWindow::onClientConnectionChanged);
    connect(m_serverThread, &AirPlayServerThread::errorOccurred,
            this, [this](const QString &msg) {
                qWarning() << "[AirPlay] Error:" << msg;
            });

    // Argomenti passati a uxplay (aggiungi quello che ti serve)
    QStringList args;
    args << "-n" << "MyAirPlay"
         << "-nh";   // no finestra GStreamer separata

    m_serverThread->setArguments(args);
    m_serverThread->start();
}

void AirPlayServerThread::requestStop()
{
    uxplay_stop();
}

void AirPlayWindow::stopAirPlayServer()
{
    if (m_serverThread) {
        m_serverThread->requestStop();

        // Aspetta max 5s che il thread finisca prima di distruggerlo
        if (!m_serverThread->wait(2000)) {
            qWarning() << "[AirPlay] Thread non terminato, forzo terminate";
            m_serverThread->terminate();
            m_serverThread->wait();
        }

        delete m_serverThread;   // delete diretto invece di deleteLater
        m_serverThread = nullptr;
    }
    m_serverRunning = false;
}

void AirPlayWindow::onServerStatusChanged(bool running)
{
    m_serverRunning = running;
    m_statusLabel->setText(running
        ? "Waiting for device connection..."
        : "Server stopped.");
    qDebug() << "[AirPlay] Server running:" << running;
}

void AirPlayWindow::onClientConnectionChanged(bool connected)
{
    m_clientConnected = connected;
    m_statusLabel->setText(connected
        ? "Device connected - receiving stream..."
        : "Waiting for device connection...");
    if (!connected)
        m_videoLabel->clear();
}

void AirPlayWindow::updateVideoFrame(QByteArray frameData,
                                     int width, int height)
{
    if (frameData.size() != width * height * 3) {
        qDebug() << "[AirPlay] Frame size mismatch";
        return;
    }

    QImage image(reinterpret_cast<const uchar *>(frameData.constData()),
                 width, height,
                 QImage::Format_RGB888);

    // Scala mantenendo aspect ratio
    QPixmap pixmap = QPixmap::fromImage(image).scaled(
        m_videoLabel->size(),
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

    m_videoLabel->setPixmap(pixmap);
}

void AirPlayWindow::closeEvent(QCloseEvent *event)
{
    stopAirPlayServer();
    QMainWindow::closeEvent(event);
}
