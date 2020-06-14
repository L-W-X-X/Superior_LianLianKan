#include "main_game_window.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainGameWindow w;
    w.showMaximized();

    return a.exec();
}
