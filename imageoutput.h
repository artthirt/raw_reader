#ifndef IMAGEOUTPUT_H
#define IMAGEOUTPUT_H

#include <QWidget>

class ImageOutput : public QWidget
{
	Q_OBJECT
public:
	explicit ImageOutput(QWidget *parent = 0);

	void setImage(const QImage& image);
	void setScaled(bool value);

signals:

public slots:


	// QWidget interface
protected:
	virtual void paintEvent(QPaintEvent *);

private:
	QImage m_image;

	bool m_is_smooth;
	bool m_scaled;

	bool m_mouse_down;

	QPointF m_mouse_pt;
	QPointF m_image_pos;
	double m_scale_arg;

	// QWidget interface
protected:
	virtual void mousePressEvent(QMouseEvent *);
	virtual void mouseReleaseEvent(QMouseEvent *);
	virtual void mouseMoveEvent(QMouseEvent *);

	// QWidget interface
protected:
	virtual void wheelEvent(QWheelEvent *);
};

#endif // IMAGEOUTPUT_H
