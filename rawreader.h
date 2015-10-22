#ifndef RAWREADER_H
#define RAWREADER_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QByteArray>
#include <QTime>

//////////////////////////////////////////////
/// Matrix

template< typename T >
struct Mat{
	explicit Mat(){
		rows = cols = 0;
	}
	Mat(int rows, int cols){
		this->rows = rows;
		this->cols = cols;

		data.resize(cols * rows);
	}
	Mat(const Mat< T >& m){
		rows = m.rows;
		cols = m.cols;
		data = m.data;
	}
	Mat& operator= (const Mat< T >& m){
		rows = m.rows;
		cols = m.cols;
		data = m.data;
		return *this;
	}
	inline T& operator() (int i0, int i1){
		return data[i0 * cols + i1];
	}
	inline T& operator[] (int i0){
		return data[i0 * cols];
	}
	inline const T& operator[] (int i0) const{
		return data[i0 * cols];
	}
	inline const T* at(int i0) const{
		return &data[i0 * cols];
	}
	inline T* at(int i0){
		return &data[i0 * cols];
	}
	inline T& at(int i0, int i1){
		return data[i0 * cols + i1];
	}
	inline const T& at(int i0, int i1) const{
		return data[i0 * cols + i1];
	}
	void clear(){
		rows = cols = 0;
		data.clear();
	}
	bool empty(){
		return data.size() == 0;
	}

	int rows;
	int cols;
	std::vector< T > data;
};

///////////////////////////////////////////////
/// \brief The RawReader class
///

class RawReader: public QThread
{
	Q_OBJECT
public:
	enum TYPE_DEMOSCALE{
		GRAY,
		SIMPLE,
		LINEAR
	};
	RawReader();
	~RawReader();

	bool start_read_file(const QString& fn);
	bool is_made() const;
	bool is_work() const;

	void set_shift(int shift);
	void set_demoscaling(TYPE_DEMOSCALE value);

	int width() const;
	int height() const;
	const QImage &image() const;

	int time_exec() const;

	int shift() const;

protected:
	virtual void run();

private:
	bool m_made;
	QString m_fileName;
	bool m_start;
	bool m_done;
	bool m_work;

	Mat< ushort > m_bayer;
	Mat< ushort > m_tmp;

	int m_shift;
	int m_width;
	int m_height;
	QImage m_image;
	QTime m_time_counter;
	int m_time_exec;

	TYPE_DEMOSCALE m_demoscaling;

	void create_image();
	void work();
	void demoscaling();

	inline int getred(int i, int j);
	inline int getblue(int i, int j);
	inline int getgreen(int i, int j);

	void demoscaling_linear();
};

#endif // RAWREADER_H
