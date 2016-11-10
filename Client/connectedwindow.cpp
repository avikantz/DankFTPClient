#include "connectedwindow.h"
#include "ui_connectedwindow.h"
#include "entrywindow.h"

char* encrypt_buffer (char *buffer, int nob) {
    int i;
    char *nb = (char *)calloc(BUFFER_SIZE + 1, sizeof(char));
    for (i = 0; i < nob; ++i) {
        nb[i] = (buffer[i] + (i % 8)) % 256;
    }
    return nb;
}

char* decrypt_buffer (char *buffer, int nob) {
    int i;
    char *nb = (char *)calloc(BUFFER_SIZE + 1, sizeof(char));
    for (i = 0; i < nob; ++i) {
        nb[i] = (buffer[i] - (i % 8)) % 256;
    }
    return nb;
}

QModelIndex selectedIndex;

ConnectedWindow::ConnectedWindow(server_info serv, QWidget *parent) :
	QMainWindow(parent),

    ui(new Ui::ConnectedWindow) {

	is_playing = false;
	is_destroyed = true;
	s_info = serv;
	ui->setupUi(this);

    setAcceptDrops(true);

	char address[100];
    snprintf(address, 100, "Connected to %s:%d", inet_ntoa(s_info.serv.sin_addr), s_info.serv.sin_port);
	ui->label->setText(address);

	// Actions
	connect(ui->actionDisconnect, SIGNAL(triggered(bool)), this, SLOT(kill_client()));
    connect(ui->refreshButton, SIGNAL(clicked(bool)), this, SLOT(list_files()));
    connect(ui->listView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(fetch_file(QModelIndex)));
    connect(ui->listView, SIGNAL(clicked(QModelIndex)), this, SLOT(lookup_file(QModelIndex)));
	connect(ui->pauseButton, SIGNAL(clicked(bool)), this, SLOT(change_state()));
	connect(ui->stopButton, SIGNAL(clicked(bool)), this, SLOT(stop_music()));
	connect(ui->uploadButton, SIGNAL(clicked(bool)), this, SLOT(open_file_browser()));
    connect(ui->openButton, SIGNAL(clicked(bool)), this, SLOT(open_action()));
    connect(ui->downloadButton, SIGNAL(clicked(bool)), this, SLOT(save_prompt()));
	connect(this, SIGNAL(progressChanged(int)), this, SLOT(change_bar(int)));

	ui->playedProgress->setValue(0);
	ui->playedProgress->setTextVisible(false);
	ui->progressBar->hide();

    ui->pauseButton->hide();
    ui->stopButton->hide();
    ui->playedProgress->hide();

    list_files();
}

ConnectedWindow::~ConnectedWindow() {
    delete ui;
}

void ConnectedWindow::dragEnterEvent(QDragEnterEvent *event) {
    event->acceptProposedAction();
}

void ConnectedWindow::dropEvent(QDropEvent *event) {
    foreach (const QUrl &url, event->mimeData()->urls()) {
        QString fileName = url.toLocalFile();
        qDebug() << "Dropped file: " << fileName;
        upload_file(fileName);
    }
}

void ConnectedWindow::open_file_browser() {

    QString file_name = QFileDialog::getOpenFileName(this, tr("Open File"), "~");
    qDebug() << "File to upload: " << file_name;

    upload_file(file_name);

}

void ConnectedWindow::upload_file (QString file_name) {

    _control ctrl;
    ctrl.command = FUPLOAD;
    ctrl.is_error = 0;
    ::send(s_info.sockfd, (_control *)&ctrl, sizeof(_control), 0);

    int file_name_starts_at = file_name.lastIndexOf("/");
    QString file_name_to_send = file_name.mid(file_name_starts_at + 1);
    qDebug() << "Sending file: " << file_name_to_send;

    int i;
    char filename[NAME_SIZE];
    for (i = 0; i < file_name_to_send.size(); ++i) {
        filename[i] = file_name_to_send.at(i).toLatin1();
    }
    filename[i] = '\0';

    ::send(s_info.sockfd, filename, NAME_SIZE, 0);
    int nob;
    int fd = open(file_name.toStdString().c_str(), O_RDONLY);
    if (fd != -1) {
        char buff[BUFFER_SIZE + 1];
        while ((nob = read(fd, buff, BUFFER_SIZE)) > 0) {
            char *nb = encrypt_buffer(buff, nob);
            ::send(s_info.sockfd, nb, nob, 0);
            if (nob < BUFFER_SIZE) {
                break;
            }
        }
        char flush[10];
        ::send(s_info.sockfd, flush, 10, 0);
        qDebug() << "File uploaded :O";
        QMessageBox::information(this, tr("DankFTP"), tr("File upload success.\nGo fish!"));
        list_files();
    } else {
        qDebug() << "Unable to send :(";
        QMessageBox::information(this, tr("DankFTP"), tr("File upload failed :("));
    }

}

int fileExists (const char *filename) {
   FILE *fp = fopen(filename, "r");
   if (fp != NULL)
       fclose (fp);
   return (fp != NULL);
}

int ConnectedWindow::download_file (QString fname) {

    char file_name[NAME_SIZE];
    int i;
    for (i = 0; i < fname.size(); ++i) {
        file_name[i] = fname.at(i).toLatin1();
    }
    file_name[i] = '\0';

    QString path = "./transfers/";
    path.append(fname);
    if (fileExists(path.toLatin1().data())) {
        qDebug() << "Already downloaded: " << file_name;
        return 1;
    }

    qDebug() << "Downloading: " << file_name;

    header_block file_head;

    send(s_info.sockfd, file_name, NAME_SIZE, 0);
    ::recv(s_info.sockfd, (header_block *)&file_head, sizeof(header_block), 0);

    if (!file_head.error_code) {
        ui->progressBar->show();
        ui->progressBar->setValue(0);
        qDebug() << "Header received. File size: " << file_head.filesize;
        char buff[BUFFER_SIZE];
        system("mkdir transfers");
        int fd = open(path.toStdString().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        int nob;
        int received = 0;
        int percentage;
        int limit = 10;
        while ((nob = ::recv(s_info.sockfd, buff, BUFFER_SIZE, 0)) > 0) {
            char *nb = decrypt_buffer(buff, nob);
            write(fd, nb, nob);
            received += nob;
            percentage = (received/file_head.filesize)*100;
            if(percentage > limit) {
                ui->progressBar->setValue(percentage);
                ui->progressBar->repaint();
            }
            if(received >= file_head.filesize)
                break;
        }
        ui->progressBar->hide();
        ::close(fd);
        qDebug() << "File received!";
        return 1;
    } else {
        qDebug() << "Error";
        return -1;
    }
}

void ConnectedWindow::change_state() {

    if (is_playing) {
		player->pause();
		is_playing = false;
		ui->pauseButton->setText("Resume");
		ui->statusbar->showMessage("Paused...");
    } else {
        if (is_destroyed) {
			ui->statusbar->showMessage("Music was either stopped or not selected!");
        } else if(player) {
			player->play();
			is_playing = true;
			ui->pauseButton->setText("Pause");
			ui->statusbar->showMessage("Playing...");
		}
	}
}

void ConnectedWindow::stop_music() {

	if(is_destroyed) {
		return;
	}
    if (player) {
		is_destroyed = true;        
		is_playing = false;
		ui->label_3->setText("");
		delete player;
		ui->pauseButton->setText("Pause");
		ui->statusbar->showMessage("Stopped playback.");
	}
}

void ConnectedWindow::updatebar() {
	qint64 played = 0;
	qint64 ticker = 0;
	qint64 old = 0;
	int seconds = 0;

	while(!is_destroyed) {
		try {
			played = player->position();
			old = ticker;
			ticker = played / 1000;
			if(old != ticker){
				seconds++;
				emit progressChanged(seconds);
			}
        } catch (...) {
			return;
		}

	}
}

void ConnectedWindow::change_bar(int seconds) {

	int total = player->duration()/1000;
	float percentage = (float)seconds/total;

	int minutes = seconds/60;
	seconds %= 60;

	ui->duration->setText(QString::number(minutes).append(":").append(QString::number(seconds)));
	ui->playedProgress->setValue(percentage*100);
	ui->playedProgress->repaint();

    ui->statusbar->showMessage("Playing audio...");
}

void ConnectedWindow::setup_music_player(QString song_name) {

	player = new QMediaPlayer;
	QString mediaPath = qApp->applicationDirPath().append("/transfers/");
	mediaPath.append(song_name);
	player->setMedia(QUrl::fromLocalFile(mediaPath));
	player->setVolume(50);
	player->play();
	QtConcurrent::run(this, &ConnectedWindow::updatebar);
	is_playing = true;
	is_destroyed = false;

	QString label_text = "Currently playing: ";
	label_text.append(song_name);

	ui->label_3->setText(label_text);

}

void ConnectedWindow::save_prompt() {
    if(!is_destroyed) {
        stop_music();
    }

    QString file_name = selectedIndex.data().toString();
    QString message(file_name);
    message.prepend("Saving: ");
    ui->statusbar->showMessage(message);

    _control ctrl;
    ctrl.command = REQ_FILE;
    ctrl.is_error = 0;
    send(s_info.sockfd, (_control *)&ctrl, sizeof(_control), 0);
    int is_downloaded = download_file(file_name);
    if(is_downloaded == -1) {
        QMessageBox::information(0, "Error", "Could not open file!");
    } else {
        save_file(file_name);
    }

}

void ConnectedWindow::open_action() {
    fetch_file(selectedIndex);
}


void ConnectedWindow::save_file(QString fname) {
	QString mediaPath = qApp->applicationDirPath().append("/transfers/");
	mediaPath.append(fname);
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As..."), mediaPath);
    qDebug() << "Filename = " << fileName;
    QFile::copy(mediaPath, fileName);
}

void ConnectedWindow::open_file(QString fname) {
    QString mediaPath = qApp->applicationDirPath().append("/transfers/");
    mediaPath.append(fname);
    QDesktopServices::openUrl(QUrl::fromLocalFile(mediaPath));
}

void ConnectedWindow::display_image(QString fname) {
    QString mediaPath = qApp->applicationDirPath().append("/transfers/");
    mediaPath.append(fname);
    QPixmap image(mediaPath);
    int w = ui->imageLabel->width();
    int h = ui->imageLabel->height();
    ui->imageLabel->setPixmap(image.scaled(w, h, Qt::KeepAspectRatio));
}

void ConnectedWindow::lookup_file(QModelIndex index) {

    selectedIndex = index;

    if(!is_destroyed) {
        stop_music();
    }

    // Preview file or something

    ui->pauseButton->hide();
    ui->stopButton->hide();
    ui->playedProgress->hide();
    ui->duration->hide();

    QString file_name = index.data().toString();
    QString message(file_name);
    message.prepend("Selected: ");
    ui->statusbar->showMessage(message);

    QString ext = file_name.right(3);
    QString extlist = QString("mp3 m4a jpg png gif");
    qDebug() << "Extension: " << ext;
    if (!(extlist.contains(ext))) {
        ui->statusbar->showMessage("No preview available");
        return;
    }

    _control ctrl;
    ctrl.command = REQ_FILE;
    ctrl.is_error = 0;
    send(s_info.sockfd, (_control *)&ctrl, sizeof(_control), 0);
    int is_downloaded = download_file(file_name);
    if(is_downloaded == -1) {
        QMessageBox::information(0, "Error", "Could not open file!");
    } else {
        if ((QString::compare("mp3", ext) == 0) || (QString::compare("m4a", ext) == 0)) {
            // Music file
            ui->pauseButton->show();
            ui->stopButton->show();
            ui->playedProgress->show();
            ui->duration->show();
            setup_music_player(file_name);
        } else if ((QString::compare("jpg", ext) == 0) || (QString::compare("png", ext) == 0) || (QString::compare("gif", ext) == 0)) {
            display_image(file_name);
        }
    }
}

void ConnectedWindow::fetch_file(QModelIndex index) {

    selectedIndex = index;

	if(!is_destroyed) {
		stop_music();
	}
	QString file_name = index.data().toString();
	QString message(file_name);
	message.prepend("Selected: ");
	ui->statusbar->showMessage(message);

	_control ctrl;
	ctrl.command = REQ_FILE;
	ctrl.is_error = 0;
	send(s_info.sockfd, (_control *)&ctrl, sizeof(_control), 0);
    int is_downloaded = download_file(file_name);
	if(is_downloaded == -1) {
		QMessageBox::information(0, "Error", "Could not open file!");
    } else {
        open_file(file_name);
	}

}

void ConnectedWindow::list_files() {

	_control ctrl;
	ctrl.command = REQ_LIST;
	ctrl.is_error = 0;
	send(s_info.sockfd, (_control *)&ctrl, sizeof(_control), 0);
	char list_data[BUFFER_SIZE];
	int fd = open("FileList.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	int nob;

    while ((nob = ::recv(s_info.sockfd, list_data, BUFFER_SIZE, 0)) > 0) {
		write(fd, list_data, nob);
		if(nob != BUFFER_SIZE)
			break;
	}

    ui->statusbar->showMessage("Ready...");
	::close(fd);

	QStringList stringList;

	QFile listing("FileList.txt");
    if(!listing.open(QIODevice::ReadOnly)) {
		QMessageBox::information(0, "Error", listing.errorString());
	}
	QTextStream textStream(&listing);

    while (YES) {

		QString line = textStream.readLine();
		if (line.isNull())
			break;
		else
			stringList.append(line);
	}

	QStringListModel *model;
	model = new QStringListModel(this);
	model->setStringList(stringList);
	ui->listView->setModel(model);
	ui->listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

	system("rm FileList.txt");

}

void ConnectedWindow::kill_client() {

	::close(s_info.sockfd);

	EntryWindow *ew = new EntryWindow();
	ew->show();
	ew->setWindowTitle("DankFTPClient");

	this->close();

}
