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

	connect(&m_rawReader->reader(), SIGNAL(log_message(RawReader::STATE_TYPE,QString)),
			this, SLOT(onLogMessage(RawReader::STATE_TYPE,QString)), Qt::QueuedConnection);

	ui->spinBox->setValue(m_rawReader->reader().shift());
	ui->sb_lshift->setValue(m_rawReader->reader().lshift());

	connect(&m_timer, SIGNAL(timeout()), this, SLOT(on_timeout()));
	m_timer.setInterval(300);

	m_statusLabel = new QLabel(this);
	m_statusLabel->setMinimumWidth(200);
	ui->statusBar->addWidget(m_statusLabel);

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
	m_rawReader->reader().set_size(arg1, ui->sb_height->value());
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
	m_rawReader->reader().set_shift(arg1);
	start_work();
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
	start_work();
}

const QString xml_config("config.xml");

QString get_from_xml(QDomDocument& dom, const QString& name)
{
	QDomNodeList list = dom.elementsByTagName(name);
	if(list.size()){
		return list.item(0).firstChild().toText().data();
	}
	return "";
}

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

	QString value = get_from_xml(dom, "filename");
	if(!value.isNull())
		open_file(value);
	ui->sb_width->setValue(get_from_xml(dom, "width").toInt());
	ui->sb_height->setValue(get_from_xml(dom, "height").toInt());

	int val = get_from_xml(dom, "type").toInt();
	if(val == 1)
		ui->rb_type1->setChecked(true);
	else
		ui->rb_type2->setChecked(true);
}

void create_text_node(QDomDocument& dom, QDomNode& tree, const QString& name, const QString& value)
{
	QDomNode node = dom.createElement(name);
	tree.appendChild(node);

	QDomText text = dom.createTextNode(value);
	node.appendChild(text);
}

void create_text_node(QDomDocument& dom, QDomNode& tree, const QString& name, int value)
{
	create_text_node(dom, tree, name, QString::number(value));
}

void MainWindow::saveXml()
{
	QDomDocument dom;
	QDomProcessingInstruction pr = dom.createProcessingInstruction("xml version=\"1.0\"", "encoding=\"utf-8\"");

	dom.appendChild(pr);

	QDomNode tree = dom.createElement("tree");
	dom.appendChild(tree);

	create_text_node(dom, tree, "filename", m_fileName);
	create_text_node(dom, tree, "width", ui->sb_width->value());
	create_text_node(dom, tree, "height", ui->sb_height->value());
	create_text_node(dom, tree, "type", ui->rb_type1->isChecked()? "1" : "2");

	QByteArray data = dom.toByteArray();
	QFile file(xml_config);
	if(file.open(QIODevice::WriteOnly)){
		file.write(data);
		file.close();
	}
}

void MainWindow::start_work()
{
	m_rawReader->start_compute();
	m_timer.start();
	ui->lb_work->setVisible(true);
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
		start_work();
	}
}

void MainWindow::on_rb_type1_clicked(bool checked)
{
	if(checked){
		m_rawReader->reader().set_type(RawReader::RAW_TYPE_1);
		start_work();
	}
}

void MainWindow::on_rb_type2_clicked(bool checked)
{
	if(checked){
		m_rawReader->reader().set_type(RawReader::RAW_TYPE_2);
		start_work();
	}
}

void MainWindow::on_pb_recompute_clicked()
{
	start_work();
}

void MainWindow::on_sb_height_valueChanged(int arg1)
{
	m_rawReader->reader().set_size(ui->sb_width->value(), arg1);
}

void MainWindow::onLogMessage(RawReader::STATE_TYPE type, const QString &text)
{
	m_statusLabel->setText(text);
	switch (type) {
		case RawReader::OK:
			m_statusLabel->setStyleSheet("background: #67ea3a;");
			break;
		case RawReader::WARNING:
			m_statusLabel->setStyleSheet("background: #f9a451;");
			break;
		case RawReader::ERROR:
			m_statusLabel->setStyleSheet("background: #e12500;");
			break;
		default:
			break;
	}
}
