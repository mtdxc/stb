#include <sys/timeb.h>
#include <time.h>
#include <ctype.h>
#include <locale.h>
#include <assert.h>
#include <algorithm>
#include "VxText.h"
#include "freetype.h"
#include "stb_image.h" /* http://nothings.org/stb/stb_image_write.h */
#include "stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */
#ifdef _WIN32
#define strcasecmp _stricmp
#endif

using namespace cvx;

Mat::~Mat() {
	assign(nullptr, 0, 0, 0);
}
Mat::Mat(const Mat& m) : data(nullptr){
	assign(m.data, m.width, m.height, m.channel, false);
}
Mat::Mat(int w, int h, int c, uint8_t* b) : data(nullptr) {
	assign(b, w, h, c, false);
}
void Mat::assign(uint8_t* b, int w, int h, int c, bool ref) {
	if (data){
		stbi_image_free(data);
		data = nullptr;
	}
	width = height = channel = stride = 0;
	if (b && w && h) {
		if (ref){
			data = b;
		}
		else{
			int n = w*h*c;
			data = (uint8_t*)malloc(n);
			memcpy(data, b, n);
		}
		width = w;
		height = h;
		channel = c;
		stride = width * channel;
	}
}

bool Mat::load(const char* path) {
	bool ret = false;
	int w, h, c;
	uint8_t* b = stbi_load(path, &w, &h, &c, 3);
	if (b) {
		assign(b, w, h, c);
		ret = true;
	}
	return ret;
}

bool Mat::save(const char* path) {
	bool ret = false;
	if (data) {
		const char* pdot = strrchr(path, '.');
		if (!strcasecmp(pdot, ".bmp")) {
			ret = stbi_write_bmp(path, width, height, channel, data);
		}
		else if (!strcasecmp(pdot, ".png")) {
			ret = stbi_write_png(path, width, height, channel, data, stride);
		}
		else {
			ret = stbi_write_jpg(path, width, height, channel, data, 90);
		}
	}
	return ret;
}

// ���ֿ�
CvxText::CvxText(const char* freeType)
{
	assert(freeType != NULL);

	// ���ֿ��ļ�, ����һ������
	if (FT_Init_FreeType(&m_library)) throw;
	if (FT_New_Face(m_library, freeType, 0, &m_face)) throw;

	// ���������������
	restoreFont();

	// ����C���Ե��ַ�������
	setlocale(LC_ALL, "");
}

// �ͷ�FreeType��Դ
CvxText::~CvxText()
{
	FT_Done_Face(m_face);
	FT_Done_FreeType(m_library);
}

void CvxText::getFont(int* type, Scalar* size, bool* underline, float* diaphaneity)
{
	if (type) *type = m_fontType;
	if (size) *size = m_fontSize;
	if (underline) *underline = m_fontUnderline;
	if (diaphaneity) *diaphaneity = m_fontDiaphaneity;
}

void CvxText::setFont(int* type, Scalar* size, bool* underline, float* diaphaneity)
{
	// �����Ϸ��Լ��
	if (type) {
		if (type) m_fontType = *type;
	}
	if (size) {
		m_fontSize.val[0] = std::fabs(size->val[0]);
		m_fontSize.val[1] = std::fabs(size->val[1]);
		m_fontSize.val[2] = std::fabs(size->val[2]);
		m_fontSize.val[3] = std::fabs(size->val[3]);
	}
	if (underline) {
		m_fontUnderline = *underline;
	}
	if (diaphaneity) {
		m_fontDiaphaneity = *diaphaneity;
	}

	FT_Set_Pixel_Sizes(m_face, (int)m_fontSize.val[0], 0);
}

// �ָ�ԭʼ����������
void CvxText::restoreFont()
{
	m_fontType = 0;            // ��������(��֧��)

	m_fontSize.val[0] = 20;      // �����С
	m_fontSize.val[1] = 0.5;   // �հ��ַ���С����
	m_fontSize.val[2] = 0.1;   // �����С����
	m_fontSize.val[3] = 0;      // ��ת�Ƕ�(��֧��)

	m_fontUnderline = false;   // �»���(��֧��)

	m_fontDiaphaneity = 1.0;   // ɫ�ʱ���(�ɲ���͸��Ч��)

	// �����ַ���С
	FT_Set_Pixel_Sizes(m_face, (int)m_fontSize.val[0], 0);
}

// �������(��ɫĬ��Ϊ��ɫ)
int CvxText::putText(Mat& img, char* text, Point pos)
{
	return putText(img, text, pos, Scalar(255, 255, 255));
}

int CvxText::putText(Mat& img, const wchar_t* text, Point pos)
{
	return putText(img, text, pos, Scalar(255, 255, 255));
}

int CvxText::putText(Mat& img, const char* text, Point pos, Scalar color)
{
	if (img.data == NULL) return -1;
	if (text == NULL) return -1;

	int i;
	for (i = 0; text[i] != '\0'; ++i) {
		wchar_t wc = text[i];

		// ����˫�ֽڷ���
		if (!isascii(wc)) mbtowc(&wc, &text[i++], 2);

		// �����ǰ���ַ�
		putWChar(img, wc, pos, color);
	}

	return i;
}

int CvxText::putText(Mat& img, const wchar_t* text, Point pos, Scalar color)
{
	if (img.data == NULL) return -1;
	if (text == NULL) return -1;

	int i;
	for (i = 0; text[i] != '\0'; ++i) {
		// �����ǰ���ַ�
		putWChar(img, text[i], pos, color);
	}

	return i;
}

// �����ǰ�ַ�, ����m_posλ��
void CvxText::putWChar(Mat& img, wchar_t wc, Point& pos, Scalar color)
{
	// ����unicode��������Ķ�ֵλͼ
	FT_UInt glyph_index = FT_Get_Char_Index(m_face, wc);
	FT_Load_Glyph(m_face, glyph_index, FT_LOAD_DEFAULT);
	FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_MONO);

	FT_GlyphSlot slot = m_face->glyph;

	// ������
	int rows = slot->bitmap.rows;
	int cols = slot->bitmap.width;

	for (int i = 0; i < rows; ++i) {
		for (int j = 0; j < cols; ++j) {
			int off = i * slot->bitmap.pitch + j / 8;

			if (slot->bitmap.buffer[off] & (0xC0 >> (j % 8))) {
				int r = pos.y - (rows - 1 - i);
				int c = pos.x + j;

				if (r >= 0 && r < img.height && c >= 0 && c < img.width) {
					auto pixel = img.at(c, r);
					Scalar scalar = Scalar(pixel[0], pixel[1], pixel[2]);

					// ����ɫ���ں�
					float p = m_fontDiaphaneity;
					for (int k = 0; k < 4; ++k) {
						scalar.val[k] = scalar.val[k] * (1 - p) + color.val[k] * p;
					}

					pixel[0] = (unsigned char)(scalar.val[0]);
					pixel[1] = (unsigned char)(scalar.val[1]);
					pixel[2] = (unsigned char)(scalar.val[2]);
				}
			}
		}
	}

	// �޸���һ���ֵ����λ��
	double space = m_fontSize.val[0] * m_fontSize.val[1];
	double sep = m_fontSize.val[0] * m_fontSize.val[2];

	pos.x += (int)((cols ? cols : space) + sep);
}

static wchar_t* ToWchar(char* &src, const char *locale = "zh_CN.utf8")
{
	if (src == NULL) {
		return 0;
	}

	// ���ݻ�����������locale
	setlocale(LC_CTYPE, locale);

	// �õ�ת��Ϊ��Ҫ�Ŀ��ַ���С
	int w_size = mbstowcs(NULL, src, 0) + 1;

	// w_size = 0 ˵��mbstowcs����ֵΪ-1���������й����������˷Ƿ��ַ�(���п���ʹlocale
	// û��������ȷ)
	if (w_size == 0) {
		return 0;
	}

	//wcout << "w_size" << w_size << endl;
	wchar_t* dest = new wchar_t[w_size];
	if (dest) {
		int ret = mbstowcs(dest, src, strlen(src) + 1);
		if (ret <= 0) {
			delete[] dest;
			dest = nullptr;
		}
	}
	return dest;
}

#if 1
/*ʱ���ӡ*/
char* log_Time(void)
{
	struct  tm      *ptm;
	struct  timeb   stTimeb;
	static  char    szTime[19];

	ftime(&stTimeb);
	ptm = localtime(&stTimeb.time);
	sprintf(szTime, "%02d-%02d %02d:%02d:%02d.%03d", ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, stTimeb.millitm);
	szTime[18] = 0;
	return szTime;
}

int main(int argc, char** argv) {
	int times = 1000;
	if (argc > 1)
		times = atoi(argv[1]);
	int step = times / 10;

	// printf("��ͼ֮ǰ[%s]\n", log_Time());
	CvxText text("c:/windows/fonts/msyh.ttc"); //ָ������
	Scalar size1{ 40, 0.5, 0.1, 0 }; // (�����С, ��Ч��, �ַ����, ��Ч�� }
	text.setFont(nullptr, &size1, nullptr, 0);

	// open image
	Mat input;
	if (!input.load("input.jpg")) {
		printf("=========================\n");
		return 0;
	}
	printf("start[%s]\n", log_Time());
	for (int i = 0; i<times; i++)
	{
		//printf("��ͼ֮��[%s]\n", log_Time());
		Mat image(input);
		/*ͼƬ��С*/
		//std::cout << "size (after reading): " << image.height << " , "<< image.width << std::endl;

		//����
		//rectangle(image, Point(155, 693), Point(349, 1073), Scalar(0, 0, 255), 3, 1);

		//��ĸ����
		//putText(image, "HELLO", Point(160, 1065), CV_FONT_HERSHEY_COMPLEX, 1, Scalar(0,0, 255), 2, 1);

		char* str = (char *)"���123��";
		if (wchar_t *w_str = ToWchar(str)) {
			text.putText(image, w_str, Point(160, 100), Scalar(0, 0, 255));
			delete[] w_str;
		}

		//printf("����ͼ֮��[%s]\n", log_Time());  
		if ((i % step) == 0){
			image.save("output.jpg");
			printf("��%d��ͼ����[%s]\n", i, log_Time());
		}
	}

	printf("end[%s]\n", log_Time());
	getchar();
	return 0;
}
#endif
