#pragma once
#include <string.h>
#include <stdint.h>
typedef struct FT_LibraryRec_  *FT_Library;
typedef struct FT_FaceRec_*  FT_Face;

namespace cvx {
	struct Scalar {
		Scalar(double v0 = 0, double v1 = 0, double v2 = 0, double v3 = 0) {
			val[0] = v0;
			val[1] = v1;
			val[2] = v2;
			val[3] = v3;
		}
		double val[4];
	};
	struct Point{
		Point(int v1 = 0, int v2 = 0) :x(v1), y(v2){}
		int x, y;
	};
	struct Rect {
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
	};
	struct Mat {
		Mat(){}
		Mat(const Mat& m);
		Mat(int w, int h, int c, uint8_t* b = nullptr);
		~Mat();

		bool load(const char* path);
		bool save(const char* path);
		void assign(uint8_t* b, int w, int h, int c, bool ref=true);
		uint8_t* at(int x, int y){ return data + y*stride + x*channel; }

		int width = 0; 
		int height = 0;
		int channel = 0;
		int stride = 0;
		uint8_t* data = nullptr;
	};

	class CvxText {
	public:
		CvxText(const char* freeType);
		virtual ~CvxText();

		void getFont(int* type, Scalar* size = NULL, bool* underline = NULL, float* diaphaneity = NULL);
		void setFont(int* type, Scalar* size = NULL, bool* underline = NULL, float* diaphaneity = NULL);
		void restoreFont();

		int putText(Mat& img, char* text, Point pos);
		int putText(Mat& img, const wchar_t* text, Point pos);
		int putText(Mat& img, const char* text, Point pos, Scalar color);
		int putText(Mat& img, const wchar_t* text, Point pos, Scalar color);

	private:
		CvxText& operator=(const CvxText&);
		void putWChar(Mat& img, wchar_t wc, Point& pos, Scalar color, int orgx);

		FT_Library   m_library;
		FT_Face      m_face;

		int         m_fontType;
		Scalar   m_fontSize;
		bool      m_fontUnderline;
		float      m_fontDiaphaneity;
	};

	// //ºº×Ö
	// int myputText(Mat img, const char *text, Point pos, Scalar color);
	// void myputWChar(Mat img, wchar_t wc, Point &pos, Scalar color);
}

