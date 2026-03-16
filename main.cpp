#include <QApplication>

#include <gst/gst.h>

#include "airplaywindow.h"

int main(int argc, char *argv[])
{
    qputenv("GST_PLUGIN_PATH",
            "C:\\msys64\\mingw64\\lib\\gstreamer-1.0");

    qputenv("GST_DEBUG", "3");               // 3=WARNING, 4=INFO, 5=DEBUG
    //qputenv("GST_DEBUG_NO_COLOR", "1");

    gst_init(&argc, &argv);

    QApplication app(argc, argv);
    AirPlayWindow w;
    w.show();
    return app.exec();
}
