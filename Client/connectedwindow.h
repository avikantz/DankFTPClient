#ifndef CONNECTEDWINDOW_H
#define CONNECTEDWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QMessageBox>
#include <QStringListModel>
#include <QString>
#include <QDir>
#include <QFileDialog>
#include <QThread>
#include <QtConcurrent/QtConcurrent>
#include <QtMultimedia/QMediaPlayer>
#include <QDesktopServices>
#include "../common.hpp"

namespace Ui {
class ConnectedWindow;
}

class ConnectedWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ConnectedWindow(server_info serv, QWidget *parent = 0);
    ~ConnectedWindow();

signals:
    void progressChanged(int seconds);

private slots:
    void fetch_file(QModelIndex);
    void lookup_file(QModelIndex);
    void list_files();
    void kill_client();
    void change_state();
    void stop_music();
    void open_file_browser();
    void change_bar(int);

private:
    Ui::ConnectedWindow *ui;
    void updatebar();
    server_info s_info;
    void setup_music_player(QString);
    void save_file(QString);
    void open_file(QString);
    int download_file(QString);
    bool is_playing;
    bool is_destroyed;
    QMediaPlayer *player;

};

#endif // CONNECTEDWINDOW_H
