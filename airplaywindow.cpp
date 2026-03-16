#include "airplaywindow.h"

// Header del tuo fork di uxplay
#include <uxplay/uxplay.h>
#include <uxplay/renderers/video_renderer.h>

#include <QDebug>

// Puntatore globale ai callback (pattern usato da iDescriptor)
// Necessario perché le callback di uxplay sono C-style, non possono
// catturare this
static uxplay_callbacks_t *uxplay_callbacks = nullptr;

// ─── Callback C-style ───────────────────────────────────────────────────────

static void on_video_play(void *cls) {
    Q_UNUSED(cls)
    qDebug() << "[AirPlay] Video started";
}

static void on_video_pause(void *cls) {
    Q_UNUSED(cls)
    qDebug() << "[AirPlay] Video paused";
}

static void on_connected(const char *deviceName,
                         const unsigned char *uuid,
                         const char *model,
                         void *cls) {
    Q_UNUSED(cls)
    qDebug() << "[AirPlay] Connected:" << deviceName << model;
}

static void on_disconnected(void *cls) {
    Q_UNUSED(cls)
    qDebug() << "[AirPlay] Disconnected";
}

// ─── AirPlayWindow ──────────────────────────────────────────────────────────

AirPlayWindow::AirPlayWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("AirPlay");
    resize(1280, 720);
}

AirPlayWindow::~AirPlayWindow() {
    stopAirPlay();
}

void AirPlayWindow::buildArgv(const QString &serverName) {
    m_argData.clear();
    m_argv.clear();

    // Equivalente a lanciare:
    // uxplay -n "serverName" -nh -vs 0 -vs 1
    // -nh = no GStreamer window separata (il video va nel widget Qt)
    // Adatta le opzioni in base al tuo fork

    auto addArg = [&](const QString &arg) {
        m_argData.append(arg.toLocal8Bit());
    };

    addArg("uxplay");           // argv[0] (nome programma, ignorato)
    addArg("-n");
    addArg(serverName);
    addArg("-nh");              // no separate GStreamer window
    // Aggiungi altri flag se necessari, es: -p per porte fisse

    for (auto &ba : m_argData)
        m_argv.append(ba.data());
}

void AirPlayWindow::startAirPlay() {
    if (m_running) return;

    buildArgv("MyAirPlay");

    // Setup callbacks (struttura definita in uxplay.h del tuo fork)
    static uxplay_callbacks_t callbacks = {};
    callbacks.video_play       = on_video_play;
    callbacks.video_pause      = on_video_pause;
    callbacks.connected        = on_connected;
    callbacks.disconnected     = on_disconnected;

    uxplay_callbacks = &callbacks;

    int res = init_uxplay(
        static_cast<int>(m_argv.size()),
        m_argv.data()
    );

    if (res != 0) {
        qWarning() << "[AirPlay] init_uxplay fallito, codice:" << res;
        uxplay_callbacks = nullptr;
        return;
    }

    m_running = true;
    qDebug() << "[AirPlay] Server avviato";
}

void AirPlayWindow::stopAirPlay() {
    if (!m_running) return;

    uxplay_cleanup();           // funzione esposta dal tuo fork
    uxplay_callbacks = nullptr;
    m_running = false;
    qDebug() << "[AirPlay] Server fermato";
}

void AirPlayWindow::closeEvent(QCloseEvent *event) {
    stopAirPlay();
    QMainWindow::closeEvent(event);
}
