#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QImage>
#include <QByteArray>
#include <QFile>
#include <QDebug>
#include <QDataStream>
#include <QDomDocument>
#include <QDomNodeList>

#include <QFileDialog>

const QString window_title = "RawReader";

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

	m_rawReader = new RawReaderWorker;
	m_rawReader->start();

	ui->spinBox->setValue(m_rawReader->reader().shift());
	ui->sb_lshift->setValue(m_rawReader->reader().lshift());

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
	m_timer.setInterval(300);

	loadXml();
}

MainWindow::~MainWindow()
{
	saveXml();

	delete m_rawReader;

	delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
	QFileDialog dlg;

	if(dlg.exec()){
		open_file(dlg.selectedFiles()[0]);
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
	m_rawReader->reader().set_shift(arg1);
	m_rawReader->start_compute();
	m_timer.start();
	ui->lb_work->setVisible(true);
}

void MainWindow::on_timeout()
{
	if(m_rawReader && m_rawReader->is_made()){
		m_timer.stop();

		ui->sb_width->setValue(m_rawReader->reader().width());
		ui->sb_height->setValue(m_rawReader->reader().height());
		ui->widget->setImage(m_rawReader->reader().image());
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
			m_rawReader->reader().set_demoscaling(RawReader::GRAY);
			break;
		case 1:
			m_rawReader->reader().set_demoscaling(RawReader::SIMPLE);
			break;
		case 2:
			m_rawReader->reader().set_demoscaling(RawReader::LINEAR);
			break;
		default:
			break;
	}
	m_rawReader->start_compute();
	m_timer.start();
	ui->lb_work->setVisible(true);
}

const QString xml_config("config.xml");

void MainWindow::loadXml()
{
	if(!QFile::exists(xml_config))
		return;

	QFile file(xml_config);

	if(!file.open(QIODevice::ReadOnly)){
		return;
	}
	QDomDocument dom;

	QByteArray data = file.readAll();
	dom.setContent(data);

	QDomNodeList list = dom.elementsByTagName("filename");
	if(list.size()){
		open_file(list.item(0).firstChild().toText().data());
	}
}

void MainWindow::saveXml()
{
	QDomDocument dom;
	QDomProcessingInstruction pr = dom.createProcessingInstruction("xml version=\"1.0\"", "encoding=\"utf-8\"");

	dom.appendChild(pr);

	QDomNode tree = dom.createElement("tree");
	dom.appendChild(tree);

	QDomNode node = dom.createElement("filename");
	tree.appendChild(node);

	QDomText text = dom.createTextNode(m_fileName);
	node.appendChild(text);

	QByteArray data = dom.toByteArray();
	QFile file(xml_config);
	if(file.open(QIODevice::WriteOnly)){
		file.write(data);
		file.close();
	}
}

void MainWindow::open_file(const QString &fileName)
{
	if(m_rawReader->start_read_file(fileName)){
		m_fileName = fileName;

		setWindowTitle(window_title + " [" + m_fileName + "]");

		m_timer.start();
		ui->lb_work->setVisible(true);
	}
}

void MainWindow::on_sb_lshift_valueChanged(int arg1)
{
	if(m_rawReader){
		m_rawReader->reader().set_lshift(arg1);
		m_rawReader->start_compute();
		m_timer.start();
		ui->lb_work->setVisible(true);
	}
}
