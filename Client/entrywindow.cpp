#include "entrywindow.h"
#include "ui_entrywindow.h"
#include <QApplication>


EntryWindow::EntryWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EntryWindow) {
    ui->setupUi(this);
    connect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(button_handle()));
    ui->statusBar->showMessage("Waiting for server input...");
}

EntryWindow::~EntryWindow() {
    delete ui;
}

void EntryWindow::button_handle() {

    ui->statusBar->showMessage("Trying to connect...");

    int sockfd;

    struct sockaddr_in serv;
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    serv.sin_addr.s_addr = inet_addr(ui->ip_addr->text().toStdString().c_str());
    serv.sin_family = AF_INET;
    serv.sin_port = htons(ui->port_no->text().toInt());

    int is_connected = ::connect(sockfd, (struct sockaddr *)&serv, sizeof(serv));

    if(is_connected != -1) {

        server_info si;
        si.sockfd = sockfd;
        si.serv = serv;

        ui->statusBar->showMessage("Connected!");

        popUpWindow = new ConnectedWindow(si);
        popUpWindow->show();
        popUpWindow->setWindowTitle("DankFTPClient");

        //Close the current window.
        this->close();

    } else {
        ui->statusBar->showMessage("Connection failed! Is the server running?");
    }
}
