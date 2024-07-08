#include "gui/MainWindow.hpp"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app{ argc, argv };
    MainWindow main_window{};
    main_window.start();
    return app.exec();
}
