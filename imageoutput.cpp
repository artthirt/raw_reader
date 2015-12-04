#include "imageoutput.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>

ImageOutput::ImageOutput(QWidget *parent) :
	QWidget(parent)
  , m_is_smooth(false)
  , m_scaled(true)
  , m_mouse_down(false)
  , m_scale_arg(1)
{
}

void ImageOutput::setImage(const QImage &image)
{
	m_image = image;
	update();
}

void ImageOutput::setScaled(bool value)
{
	m_scaled = value;
	update();
}


void ImageOutput::paintEvent(QPaintEvent *)
{
	QPainter painter(this);

	painter.fillRect(rect(), Qt::black);
	if(m_image.isNull()){
		return;
	}

	QImage tmp;
	QRect rt = rect();

	if(m_scaled){

		float ar = 1.0 * m_image.width()/m_image.height(),
				ar_wnd = 1.0 * rt.width()/rt.height();

		/// задание режима сглаживания
		Qt::TransformationMode tm = m_is_smooth? Qt::SmoothTransformation : Qt::FastTransformation;

		if(ar_wnd > ar){
			tmp = m_image.scaled(rt.height() * ar, rt.height(), Qt::IgnoreAspectRatio, tm);
		}else{
			tmp = m_image.scaled(rt.width(), rt.width()/ar, Qt::IgnoreAspectRatio, tm);
		}
		//m_mutex.unlock();

		painter.drawImage(QPoint(rt.width()/2 - tmp.width()/2, rt.height()/2 - tmp.height()/2), tmp);
	}else{
		QPointF pt = m_image_pos;
		int w = m_image.width() * m_scale_arg, h = m_image.height() * m_scale_arg;

		double ar = (double)m_image.height() / m_image.width();
		double ar_wnd = (double)rect().height() / rect().width();

		if(ar < ar_wnd){
			w = (double)rect().width() / m_scale_arg;
			h = (double)rect().width() / m_scale_arg * ar_wnd;
		}else{
			w = (double)rect().width() / m_scale_arg;
			h = (double)rect().width() / m_scale_arg * ar_wnd;
		}

		QRectF rt1(pt, QSize(w, h));
		//pt *= m_scale_arg;

		tmp = m_image.copy(rt1.toRect());
		tmp = tmp.scaled(w * m_scale_arg, h * m_scale_arg);

		painter.drawImage(QPoint(), tmp);
	}
}


void ImageOutput::mousePressEvent(QMouseEvent *e)
{
	m_mouse_down = true;

	m_mouse_pt = e->pos();
}

void ImageOutput::mouseReleaseEvent(QMouseEvent *e)
{
	m_mouse_down = false;

	m_mouse_pt = e->pos();
}

void ImageOutput::mouseMoveEvent(QMouseEvent *e)
{
	if(m_mouse_down){
		m_image_pos += (m_mouse_pt - e->windowPos()) / m_scale_arg;
		m_mouse_pt = e->windowPos();

		if(!m_image.isNull()){
			if(m_image_pos.x() < 0)
				m_image_pos.setX(0);
			if(m_image_pos.y() < 0)
				m_image_pos.setY(0);
			if(m_image_pos.x() > m_image.width() - rect().width())
				m_image_pos.setX(m_image.width() - rect().width());
			if(m_image_pos.y() > m_image.height() - rect().height())
				m_image_pos.setY(m_image.height() - rect().height());
		}
		update();
	}
}


void ImageOutput::wheelEvent(QWheelEvent *e)
{
	m_scale_arg += e->delta() > 0 ? 1: -1;
	if(m_scale_arg < 1)
		m_scale_arg = 1;
	update();
}
