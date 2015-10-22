#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QImage>
#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QDataStream>

#include <QFileDialog>

//////////////////////////////////////////////
/// \brief MainWindow::MainWindow
/// \param parent
///

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->lb_work->setVisible(false);

	m_rawReader = new RawReader;
	m_rawReader->start();

	ui->spinBox->setValue(m_rawReader->shift());

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
	m_timer.setInterval(300);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
	QFileDialog dlg;

	if(dlg.exec()){
		m_rawReader->start_read_file(dlg.selectedFiles()[0]);
		m_timer.start();
		ui->lb_work->setVisible(true);
	}
}

void MainWindow::on_sb_width_valueChanged(const QString &arg1)
{
}

void MainWindow::on_sb_width_valueChanged(int arg1)
{
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
	m_rawReader->set_shift(arg1);
	m_timer.start();
	ui->lb_work->setVisible(true);
}

void MainWindow::on_timeout()
{
	if(m_rawReader && m_rawReader->is_made()){
		m_timer.stop();

		ui->sb_width->setValue(m_rawReader->width());
		ui->sb_height->setValue(m_rawReader->height());
		ui->widget->setImage(m_rawReader->image());
		ui->lb_work->setVisible(false);

		ui->lb_time_exec->setText(QString("time execute: %1 ms").arg(m_rawReader->time_exec()));
	}
}

void MainWindow::on_chbscaled_clicked(bool checked)
{
	ui->widget->setScaled(checked);
}

void MainWindow::on_cb_demoscale_currentIndexChanged(int index)
{
	switch (index) {
		case 0:
			m_rawReader->set_demoscaling(RawReader::GRAY);
			break;
		case 1:
			m_rawReader->set_demoscaling(RawReader::SIMPLE);
			break;
		case 2:
			m_rawReader->set_demoscaling(RawReader::LINEAR);
			break;
		default:
			break;
	}
	m_timer.start();
	ui->lb_work->setVisible(true);
}
