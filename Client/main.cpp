#include "entrywindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {

    QApplication a(argc, argv);
    EntryWindow w;
    w.show();
    w.setWindowTitle("DankFTPClient");
    return a.exec();
}
