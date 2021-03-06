#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QByteArray>
#include <QTimer>

#include "rawreader.h"

class QLabel;

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

	void on_sb_lshift_valueChanged(int arg1);

	void on_rb_type1_clicked(bool checked);

	void on_rb_type2_clicked(bool checked);

	void on_pb_recompute_clicked();

	void on_sb_height_valueChanged(int arg1);

	void onLogMessage(RawReader::STATE_TYPE type, const QString& text);

private:
	Ui::MainWindow *ui;
	QTimer m_timer;
	QString m_fileName;

	QLabel* m_statusLabel;

	RawReaderWorker* m_rawReader;

	void loadXml();
	void saveXml();

	void start_work();

	void open_file(const QString& fileName);
};

#endif // MAINWINDOW_H
