#include "tarpaulinviewer.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TarpaulinViewer w;
    w.show();
    return a.exec();
}
