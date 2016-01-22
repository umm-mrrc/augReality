#include <QtGui/QApplication>
#include "augreality.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    augReality w;
    w.show();
    
    return a.exec();
}
