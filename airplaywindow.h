#pragma once
#include <QByteArray>
#include <QLabel>
#include <QMainWindow>
#include <QMutex>
#include <QThread>
#include <QVector>

// ─── Server Thread ───────────────────────────────────────────────────────────
class AirPlayServerThread : public QThread {
    Q_OBJECT
public:
    explicit AirPlayServerThread(QObject *parent = nullptr);
    ~AirPlayServerThread() override;

    void setArguments(const QStringList &args);
    void requestStop();          // <── nuovo

protected:
    void run() override;

signals:
    void statusChanged(bool running);
    void clientConnectionChanged(bool connected);
    void videoFrameReady(QByteArray frameData, int width, int height);
    void errorOccurred(const QString &message);

private:
    QMutex m_mutex;
    QVector<QByteArray> m_argData;
    QVector<char *>     m_argv;
};

// ─── Window ──────────────────────────────────────────────────────────────────

class AirPlayWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit AirPlayWindow(QWidget *parent = nullptr);
    ~AirPlayWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onServerStatusChanged(bool running);
    void onClientConnectionChanged(bool connected);
    void updateVideoFrame(QByteArray frameData, int width, int height);

private:
    void setupUI();
    void startAirPlayServer();
    void stopAirPlayServer();

    QLabel *m_videoLabel   = nullptr;
    QLabel *m_statusLabel  = nullptr;

    AirPlayServerThread *m_serverThread  = nullptr;
    bool                 m_serverRunning = false;
    bool                 m_clientConnected = false;
};
