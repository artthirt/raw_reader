#include "rawreader.h"

#include <QFile>
#include <QRegExp>

/////////////////////////////////
/// \brief for set alpha in uint
#define MASK_ALPHAMAX_UCHAR		(0xff000000)
/// \brief for crpp color value
#define MAX_UCHAR				(255)

/////////////////////////////////
/// \brief minFast
/// get min value with logic operations
/// \param a
/// \param b
/// \return
inline int minFast(int a, int b)
{
	int z = a - b;
	int i = (~(z >> 31)) & 0x1;
	return a - i * z;
}

/////////////////////////////////

const int reg_raw_type = qRegisterMetaType<RawReader::STATE_TYPE>("RawReader::STATE_TYPE");

/////////////////////////////////

RawReader::RawReader()
	: m_width(0)
	, m_height(0)
	, m_shift(4)
	, m_lshift(0)
	, m_raw_type(RAW_TYPE_NONE)
	, m_demoscaling(GRAY)
{
}

RawReader::~RawReader()
{
}

bool RawReader::set_bayer_data(const QByteArray &data)
{
	if(data.isNull())
		return false;

	QDataStream stream(data);
	stream.setByteOrder(QDataStream::LittleEndian);

	switch (m_raw_type) {
		case RAW_TYPE_NONE:
		case RAW_TYPE_1:
			stream >> m_width;
			stream >> m_height;
			break;
		default:
			break;
	}

	if(!m_width || !m_height || qAbs(m_width) > 0xffffff || qAbs(m_height) > 0xffffff)
		return false;

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

	m_initial = m_bayer;

	left_shift();

	return true;
}

bool RawReader::set_bayer_data(const QImage &image)
{
	if(image.isNull())
		return false;

	m_width = image.width();
	m_height = image.height();

	m_bayer = Mat< ushort >(m_height, m_width);
	m_tmp = Mat< ushort >(m_height, m_width);

	for(int i = 0; i < m_height; i++){
		const QRgb* sl = reinterpret_cast< const QRgb* >(image.scanLine(i));
		ushort *d = m_bayer.at(i);
		for(int j = 0; j < m_width; j++){
			d[j] = (sl[j] & 0xff);
		}
	}
	m_initial = m_bayer;

	left_shift();

	return true;
}

void RawReader::clear_bayer()
{
	m_bayer.clear();
	m_initial.clear();
	m_tmp.clear();
	m_width = m_height = 0;
}

bool RawReader::empty() const
{
	return m_bayer.empty();
}

void RawReader::set_shift(int shift)
{
	if(shift <= 0)
		return;
	m_shift = shift;
}

void RawReader::set_lshift(int value)
{
	m_lshift = value;
	left_shift();
}

int RawReader::lshift() const
{
	return m_lshift;
}

void RawReader::set_demoscaling(TYPE_DEMOSCALE value)
{
	m_demoscaling = value;
}

void RawReader::compute()
{
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
}

int RawReader::width() const
{
	return m_width;
}

int RawReader::height() const
{
	return m_height;
}

void RawReader::set_size(int w, int h)
{
	if(m_raw_type == RAW_TYPE_2){
		m_width = w;
		m_height = h;
	}else{
		emit log_message(WARNING, "size not set. different type");
	}
}

const QImage &RawReader::image() const
{
	return m_image;
}

int RawReader::shift() const
{
	return m_shift;
}

void RawReader::set_type(RawReader::RAW_TYPE type)
{
	m_raw_type = type;
}

RawReader::RAW_TYPE RawReader::type() const
{
	return m_raw_type;
}

void RawReader::left_shift()
{
	for(int i = 0; i < m_height; i++){
		ushort *d = m_bayer.at(i);
		ushort *din = m_initial.at(i);
		for(int j = 0; j < m_width; j++){
			d[j] = din[j] << m_lshift;
		}
	}
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
	emit log_message(OK, "end slow demoscaling");
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
	return minFast(MAX_UCHAR, val >> m_shift);
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
	return minFast(MAX_UCHAR, val >> m_shift);
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
	return minFast(MAX_UCHAR, val >> m_shift);
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

			g00 = (dm1[j - 1] + dm1[j + 1] + d0[j] + dp1[j - 1]	+ dp1[j + 1]) / 5;
			g11 = (d0[j] + d0[j + 2] + dm1[j + 1] + dp2[j] + dp2[j + 2])/5;
			g01 = (dm1[j + 1] + d0[j] + d0[j + 2] + dp1[j + 1]) >> 2;
			g10 = (d0[j] + dp1[j - 1] + dp1[j + 1] + dp2[j]) >> 2;

			g00 = minFast(g00 >> m_shift, MAX_UCHAR);
			g01 = minFast(g01 >> m_shift, MAX_UCHAR);
			g10 = minFast(g10 >> m_shift, MAX_UCHAR);
			g11 = minFast(g11 >> m_shift, MAX_UCHAR);

			sl0[j]		= (g00 << 8) | MASK_ALPHAMAX_UCHAR;
			sl0[j + 1]	= (g01 << 8) | MASK_ALPHAMAX_UCHAR;
			sl1[j]		= (g10 << 8) | MASK_ALPHAMAX_UCHAR;
			sl1[j + 1]	= (g11 << 8) | MASK_ALPHAMAX_UCHAR;
		}
	}

	/// edge green red blue
	{
		QRgb* sl0 = reinterpret_cast< QRgb* >(m_image.scanLine(0));				// up image, 0 row
		QRgb* sl1 = reinterpret_cast< QRgb* >(m_image.scanLine(1));				// up image, 1 row
		QRgb* slu0 = reinterpret_cast< QRgb* >(m_image.scanLine(m_height - 1));	// down image
		ushort* d0 = m_tmp.at(0);			// bayer, 0 row
		ushort* dp1 = m_tmp.at(1);			// bayer, 1 row
		ushort* dp2 = m_tmp.at(2);			// bayer, 2 row

		ushort* du0 = m_tmp.at(m_height - 1);		// bayer, height-1 row
		ushort* dup1 = m_tmp.at(m_height - 2);		// bayer, height-2 row
		for(int j = 1; j < m_width - 2; j+= 2){
			int g00 = 0, g01 = 0, r00, r01, r10, r11, b00, b01, b10, b11;
			/// green
			g00 = (d0[j-1] + d0[j + 1] + dp1[j])/ 3;
			g01 = (d0[j + 1] + dp1[j] + dp1[j + 2]) / 3;
			g00 = minFast(g00 >> m_shift, MAX_UCHAR);	g01 = minFast(g01 >> m_shift, MAX_UCHAR);
			/// red
			r00 = d0[j];
			r01 = (d0[j] + d0[j + 2]) >> 1;
			r00 = minFast(r00 >> m_shift, MAX_UCHAR);	r01 = minFast(r01 >> m_shift, MAX_UCHAR);
			/// blue
			b00 = (dp1[j - 1] + dp1[j + 1]) >> 1;
			b01 = dp1[j + 1];
			b00 = minFast(b00 >> m_shift, MAX_UCHAR);	b01 = minFast(b01 >> m_shift, MAX_UCHAR);

			sl0[j]		= (b00) | (g00 << 8) | (r00 << 16) | MASK_ALPHAMAX_UCHAR;
			sl0[j + 1]	= (b01) | (g01 << 8) | (r01 << 16) | MASK_ALPHAMAX_UCHAR;

			//////////////////////////////////
			r10 = (d0[j] + dp2[j]) >> 1;
			r11 = (d0[j] + d0[j + 2] + dp2[j] + dp2[j + 2]) >> 2;

			b10 = (dp1[j - 1] + dp1[j + 1]) >> 1;
			b11 = dp1[j + 1];

			r10 = minFast(r10 >> m_shift, MAX_UCHAR);	r11 = minFast(r11 >> m_shift, MAX_UCHAR);
			b10 = minFast(b10 >> m_shift, MAX_UCHAR);	b11 = minFast(b11 >> m_shift, MAX_UCHAR);

			sl1[j] |= (b10) | (r10 << 16);
			sl1[j + 1] |= (b11) | (r11 << 16);
			//////////////////////////////////

			g00 = (du0[j-1] + du0[j + 1] + dup1[j])/3;
			g01 = (du0[j] + dup1[j - 1] + dup1[j + 1])/ 3;
			g00 = minFast(g00 >> m_shift, MAX_UCHAR);	g01 = minFast(g01 >> m_shift, MAX_UCHAR);

			slu0[j]		= (g00 << 8) | MASK_ALPHAMAX_UCHAR;
			slu0[j + 1]	= (g01 << 8) | MASK_ALPHAMAX_UCHAR;
		}
	}

#if 1
	/// red & blue
	for(int i = 1; i < m_height/2 - 1; ++i){
		ushort *d1 = m_tmp.at(i << 1);
		ushort *d2 = m_tmp.at((i << 1) + 1);
		for(int j = 1; j < m_width/2 - 1; ++j){
			int red = 0, blue = 0;
			red = (d1[(j << 1) - 1]	+ d1[(j << 1) + 1]) >> 1;
			blue = (d2[(j << 1)] + d2[(j << 1) - 2]) >> 1;
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
			int red = 0, blue = 0;

			red = d0[j];
			red = minFast(MAX_UCHAR, red >> m_shift);

			blue = (dm1[j] + dp1[j]) >> 1;
			blue = minFast(MAX_UCHAR, blue >> m_shift);

			sl0[j] |= (red << 16) | (blue);

			red = (d0[j] + dp2[j]) >> 1;
			red = minFast(MAX_UCHAR, red >> m_shift);

			blue = dp1[j];
			blue = minFast(MAX_UCHAR, blue >> m_shift);

			sl1[j] |= (red << 16) | (blue);
		}
	}
#endif

	emit log_message(OK, "end linear demoscaling");
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
			val = minFast(val, MAX_UCHAR);
			sl[j] = qRgb(val, val, val);
		}
	}
}

////////////////////////////////////////////////
////////////////////////////////////////////////

RawReaderWorker::RawReaderWorker()
	: QThread(0)
	, m_done(false)
	, m_made(false)
{

}

RawReaderWorker::~RawReaderWorker()
{
	m_done = true;

	quit();
	wait();
}

void RawReaderWorker::run()
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


bool RawReaderWorker::start_read_file(const QString &fn)
{
	if(!QFile::exists(fn))
		return false;

	if(m_fileName == fn){
		return true;
	}

	m_fileName = fn;

	m_start = true;

	return true;
}

void RawReaderWorker::start_compute()
{
	m_start = true;
}

bool RawReaderWorker::is_made() const
{
	return m_made;
}

bool RawReaderWorker::is_work() const
{
	return !m_made;
}

RawReader &RawReaderWorker::reader()
{
	return m_reader;
}

void RawReaderWorker::work()
{
	m_made = false;

	if(m_reader.empty()){
		if(!open_image(m_fileName)){
			if(!open_raw(m_fileName)){
				m_made = true;
				return;
			}
		}
	}

	m_time_counter.start();
	int t1 = m_time_counter.elapsed();

	m_reader.compute();

	m_time_exec = m_time_counter.elapsed() - t1;

	m_made = true;
}


bool RawReaderWorker::open_raw(const QString fileName)
{
	if(!fileName.contains(QRegExp("\.raw$|\.bin$", Qt::CaseInsensitive)))
		return false;

	QFile fl(fileName);

	QByteArray data;

	if(fl.open(QIODevice::ReadOnly)){
		data = fl.readAll();
		fl.close();

		return m_reader.set_bayer_data(data);
	}
	return false;
}

bool RawReaderWorker::open_image(const QString fileName)
{
	if(!fileName.contains(QRegExp("\.jpeg$|\.jpg$|\.bmp$|\.png$", Qt::CaseInsensitive)))
		return false;

	QImage image;
	image.load(fileName);

	m_reader.set_type(RawReader::RAW_TYPE_NONE);

	return m_reader.set_bayer_data(image);
}


int RawReaderWorker::time_exec() const
{
	return m_time_exec;
}
