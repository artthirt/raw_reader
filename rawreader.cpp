#include "rawreader.h"

#include <QFile>

/////////////////////////////////

inline int minFast(int a, int b)
{
	int z = a - b;
	int i = (~(z >> 31)) & 0x1;
	return a - i * z;
}


/////////////////////////////////

RawReader::RawReader()
	: QThread(0)
	, m_made(false)
	, m_start(false)
	, m_done(false)
	, m_width(0)
	, m_height(0)
	, m_shift(4)
	, m_work(false)
	, m_demoscaling(GRAY)
{
}

RawReader::~RawReader()
{
	m_done = true;

	quit();
	wait();
}

bool RawReader::start_read_file(const QString &fn)
{
	if(!QFile::exists(fn))
		return false;

	if(m_fileName == fn){
		return true;
	}

	m_bayer.clear();
	m_width = m_height = 0;

	m_fileName = fn;

	m_start = true;

	return true;
}

bool RawReader::is_made() const
{
	return m_made;
}

bool RawReader::is_work() const
{
	return m_work;
}

void RawReader::set_shift(int shift)
{
	if(shift <= 0)
		return;
	m_shift = shift;

	m_start = true;
}

void RawReader::set_demoscaling(TYPE_DEMOSCALE value)
{
	m_demoscaling = value;

	m_start = true;
}

int RawReader::width() const
{
	return m_width;
}

int RawReader::height() const
{
	return m_height;
}

const QImage &RawReader::image() const
{
	return m_image;
}

int RawReader::time_exec() const
{
	return m_time_exec;
}

int RawReader::shift() const
{
	return m_shift;
}

void RawReader::run()
{
	while(!m_done){
		if(!m_start){
			usleep(300);
		}else{
			m_start = false;
			work();
		}
	}
}

void RawReader::work()
{
	m_made = false;
	m_work = true;

	if(m_bayer.empty()){
		QFile fl(m_fileName);

		QByteArray data;

		if(fl.open(QIODevice::ReadOnly)){
			data = fl.readAll();
			fl.close();

			QDataStream stream(data);
			stream.setByteOrder(QDataStream::LittleEndian);

			stream >> m_width;
			stream >> m_height;

			m_bayer = Mat< ushort >(m_height, m_width);
			m_tmp = Mat< ushort >(m_height, m_width);

			for(int i = 0; i < m_height; i++){
				for(int j = 0; j < m_width; j++){
					uchar ch1, ch2;
					stream >> ch1 >> ch2;
					ushort val = (ch1) | (ch2 << 8);
					m_bayer.at(i, j) = val;
				}
			}
		}
	}

	m_time_counter.start();
	int t1 = m_time_counter.elapsed();

	switch (m_demoscaling) {
		default:
		case GRAY:
			create_image();
			break;
		case SIMPLE:
			demoscaling();
			break;
		case LINEAR:
			demoscaling_linear();
			break;
	}

	m_time_exec = m_time_counter.elapsed() - t1;

	m_work = true;
}

void RawReader::demoscaling()
{
	if(m_bayer.empty())
		return;
	m_image = QImage(m_width, m_height, QImage::Format_ARGB32);

	for(int i = 1; i < m_height - 1; i++){
		QRgb* sl = reinterpret_cast< QRgb* >(m_image.scanLine(i));
		for(int j = 1; j < m_width - 1; j++){
			int red = 0, green = 0, blue = 0;
			green = getgreen(i, j);
			blue = getblue(i, j);
			red = getred(i, j);
			sl[j] = qRgb(red, green, blue);
		}
	}
	m_made = true;
}

int RawReader::getblue(int i, int j)
{
	int val = 0;
	switch (j % 2) {
		case 0:
			switch (i % 2) {
				case 1:
					val = m_bayer(i, j);
					break;
				case 0:
					val += m_bayer(i-1, j);
					val += m_bayer(i+1, j);
					val >>= 1;
				default:
					break;
			}
			break;
		case 1:
			val = 0;
			switch (i % 2) {
				case 1:
					val += m_bayer(i, j-1);
					val += m_bayer(i, j+1);
					val >>= 1;
					break;
				case 0:
					val += m_bayer(i-1, j-1);
					val += m_bayer(i-1, j+1);
					val += m_bayer(i+1, j-1);
					val += m_bayer(i+1, j+1);
					val >>= 2;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return minFast(255, val >> m_shift);
}

int RawReader::getred(int i, int j)
{
	int val = 0;
	switch (j % 2) {
		case 0:
			switch (i % 2) {
				case 0:
					val += m_bayer(i, j-1);
					val += m_bayer(i, j+1);
					val >>= 1;
					break;
				case 1:
					val += m_bayer(i-1, j-1);
					val += m_bayer(i-1, j+1);
					val += m_bayer(i+1, j-1);
					val += m_bayer(i+1, j+1);
					val >>= 2;
					break;
				default:
					break;
			}
			break;
		case 1:
		default:
			switch (i % 2) {
				case 0:
					val = m_bayer(i, j);
					break;
				case 1:
					val += m_bayer(i-1, j);
					val += m_bayer(i+1, j);
					val >>= 1;
				default:
					break;
			}
			break;
	}
	return minFast(255, val >> m_shift);
}

int RawReader::getgreen(int i, int j)
{
	int val = 0;
	switch (j % 2) {
		case 0:
			switch (i % 2) {
				case 0:
					val += m_bayer(i, j);
					val += m_bayer(i-1, j-1);
					val += m_bayer(i-1, j+1);
					val += m_bayer(i+1, j-1);
					val += m_bayer(i+1, j+1);
					val /= 5;
					break;
				case 1:
				default:
					val += m_bayer(i-1, j);
					val += m_bayer(i, j-1);
					val += m_bayer(i, j+1);
					val += m_bayer(i+1, j);
					val >>= 2;
					break;
			}
			break;
		case 1:
		default:
			switch (i % 2) {
				case 0:
					val += m_bayer(i-1, j);
					val += m_bayer(i, j-1);
					val += m_bayer(i, j+1);
					val += m_bayer(i+1, j);
					val >>= 2;
					break;
				case 1:
				default:
					val += m_bayer(i, j);
					val += m_bayer(i-1, j-1);
					val += m_bayer(i-1, j+1);
					val += m_bayer(i+1, j-1);
					val += m_bayer(i+1, j+1);
					val /= 5;
					break;
			}
			break;
	}
	return minFast(255, val >> m_shift);
}

void RawReader::demoscaling_linear()
{
	if(m_bayer.empty())
		return;

	m_tmp = m_bayer;

	m_image = QImage(m_width, m_height, QImage::Format_ARGB32);

	/// green
	for(int i = 1; i < m_height - 2; i += 2){
		QRgb* sl0 = reinterpret_cast< QRgb* >(m_image.scanLine(i));
		QRgb* sl1 = reinterpret_cast< QRgb* >(m_image.scanLine(i + 1));
		ushort* dm1 = m_tmp.at(i - 1);
		ushort* d0 = m_tmp.at(i);
		ushort* dp1 = m_tmp.at(i + 1);
		ushort* dp2 = m_tmp.at(i + 2);
		for(int j = 1; j < m_width - 2; j+= 2){
			int g00 = 0, g01 = 0, g10 = 0, g11 = 0;

			g00 += dm1[j - 1];
			g00 += dm1[j + 1];
			g00 += d0[j];
			g00 += dp1[j - 1];
			g00 += dp1[j + 1];
			g00 /= 5;

			g11 += d0[j];
			g11 += d0[j + 2];
			g11 += dm1[j + 1];
			g11 += dp2[j];
			g11 += dp2[j + 2];
			g11 /= 5;

			g01 += dm1[j + 1];
			g01 += d0[j];
			g01 += d0[j + 2];
			g01 += dp1[j + 1];
			g01 >>= 2;

			g10 += d0[j];
			g10 += dp1[j - 1];
			g10 += dp1[j + 1];
			g10 += dp2[j];
			g10 >>= 2;

			g00 = minFast(g00 >> m_shift, 255);
			g01 = minFast(g01 >> m_shift, 255);
			g10 = minFast(g10 >> m_shift, 255);
			g11 = minFast(g11 >> m_shift, 255);

			sl0[j]		= (g00 << 8) | 0xff000000;
			sl0[j + 1]	= (g01 << 8) | 0xff000000;
			sl1[j]		= (g10 << 8) | 0xff000000;
			sl1[j + 1]	= (g11 << 8) | 0xff000000;
		}
	}

#if 1
	/// red & blue
	for(int i = 1; i < m_height/2 - 1; ++i){
		ushort *d1 = m_tmp.at(i << 1);
		ushort *d2 = m_tmp.at((i << 1) + 1);
		for(int j = 1; j < m_width/2 - 1; ++j){
			int red = 0, blue = 0;
			red += d1[(j << 1) - 1];
			red += d1[(j << 1) + 1];
			red >>= 1;
			blue += d2[(j << 1)];
			blue += d2[(j << 1) - 2];
			blue >>= 1;
			d1[(j << 1)] = red;
			d2[(j << 1) - 1] = blue;
		}
	}

	/// green red blue
	for(int i = 1; i < m_height/2 - 1; ++i){
		QRgb* sl0 = reinterpret_cast< QRgb* >(m_image.scanLine(2 * i));
		QRgb* sl1 = reinterpret_cast< QRgb* >(m_image.scanLine(2 * i + 1));
		ushort *dm1 = m_tmp.at((i << 1) - 1);
		ushort *d0 = m_tmp.at(i << 1);
		ushort *dp1 = m_tmp.at((i << 1) + 1);
		ushort *dp2 = m_tmp.at((i << 1) + 2);
		for(int j = 1; j < m_width - 1; ++j){
			int green = 0, red = 0, blue = 0;

			green = (sl0[j] >> 8) & 0xFF;
			red = d0[j];
			red >>= m_shift;
			red = minFast(255, red);

			blue += dm1[j];
			blue += dp1[j];
			blue >>= 1;
			blue >>= m_shift;
			blue = minFast(255, blue);

			sl0[j] = qRgb(red, green, blue);

			green = (sl1[j] >> 8) & 0xFF;
			red = 0;
			red += d0[j];
			red += dp2[j];
			red >>= 1;
			red >>= m_shift;
			red = minFast(255, red);
			blue = 0;
			blue = dp1[j];
			blue >>= m_shift;
			blue = minFast(255, blue);

			sl1[j] = qRgb(red, green, blue);
		}
	}
#endif

	m_made = true;
}

void RawReader::create_image()
{
	if(m_bayer.empty())
		return;

	m_image = QImage(m_width, m_height, QImage::Format_ARGB32);

	for(int i = 0; i < m_height; i++){
		QRgb* sl = reinterpret_cast< QRgb* >(m_image.scanLine(i));
		for(int j = 0; j < m_width; j++){
			ushort val = m_bayer.at(i, j);
			val >>= m_shift;
			val = minFast(val, 255);
			sl[j] = qRgb(val, val, val);
		}
	}

	m_made = true;
}