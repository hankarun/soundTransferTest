#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Sound Transfer"));

    MainWindow window;
    window.resize(380, 420);
    window.show();

    return app.exec();
}
