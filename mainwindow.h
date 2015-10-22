#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QByteArray>
#include <QTimer>

#include "rawreader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_actionOpen_triggered();

	void on_sb_width_valueChanged(const QString &arg1);

	void on_sb_width_valueChanged(int arg1);

	void on_spinBox_valueChanged(int arg1);

	void on_timeout();

	void on_chbscaled_clicked(bool checked);

	void on_cb_demoscale_currentIndexChanged(int index);

private:
	Ui::MainWindow *ui;
	QTimer m_timer;

	RawReader* m_rawReader;
};

#endif // MAINWINDOW_H
