#include <QApplication>
#include "airplaywindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    AirPlayWindow w;
    w.show();
    w.startAirPlay();

    return app.exec();
}
