#pragma once
#include <QMainWindow>
#include <QCloseEvent>
#include <QVector>
#include <QString>

// Forward declaration: evita di includere uxplay.h nell'header
struct uxplay_callbacks_t;

class AirPlayWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AirPlayWindow(QWidget *parent = nullptr);
    ~AirPlayWindow() override;

    void startAirPlay();
    void stopAirPlay();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // Argomenti passati a init_uxplay()
    QVector<QByteArray> m_argData;
    QVector<char *>     m_argv;

    bool m_running = false;

    void buildArgv(const QString &serverName);
};
