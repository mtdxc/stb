#include <sys/timeb.h>
#include <time.h>
#include <ctype.h>
#include <locale.h>
#include <assert.h>
#include <math.h>
#include "VxText.h"
#include "freetype.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" /* http://nothings.org/stb/stb_image_write.h */
#define STB_IMAGE_WRITE_IMPLEMENTATION
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

// 打开字库
CvxText::CvxText(const char* freeType)
{
	assert(freeType != NULL);

	// 打开字库文件, 创建一个字体
	if (FT_Init_FreeType(&m_library)) throw;
	if (FT_New_Face(m_library, freeType, 0, &m_face)) throw;

	// 设置字体输出参数
	restoreFont();

	// 设置C语言的字符集环境
	setlocale(LC_ALL, "");
}

// 释放FreeType资源
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
	// 参数合法性检查
	if (type) {
		if (type) m_fontType = *type;
	}
	if (size) {
		m_fontSize.val[0] = fabs(size->val[0]);
		m_fontSize.val[1] = fabs(size->val[1]);
		m_fontSize.val[2] = fabs(size->val[2]);
		m_fontSize.val[3] = fabs(size->val[3]);
	}
	if (underline) {
		m_fontUnderline = *underline;
	}
	if (diaphaneity) {
		m_fontDiaphaneity = *diaphaneity;
	}

	FT_Set_Pixel_Sizes(m_face, (int)m_fontSize.val[0], 0);
}

// 恢复原始的字体设置
void CvxText::restoreFont()
{
	m_fontType = 0;            // 字体类型(不支持)

	m_fontSize.val[0] = 20;      // 字体大小
	m_fontSize.val[1] = 0.5;   // 空白字符大小比例
	m_fontSize.val[2] = 0.1;   // 间隔大小比例
	m_fontSize.val[3] = 0;      // 旋转角度(不支持)

	m_fontUnderline = false;   // 下画线(不支持)

	m_fontDiaphaneity = 1.0;   // 色彩比例(可产生透明效果)

	// 设置字符大小
	FT_Set_Pixel_Sizes(m_face, (int)m_fontSize.val[0], 0);
}

// 输出函数(颜色默认为白色)
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

		// 解析双字节符号
		if (!isascii(wc)) mbtowc(&wc, &text[i++], 2);

		// 输出当前的字符
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
		// 输出当前的字符
		putWChar(img, text[i], pos, color);
	}

	return i;
}

// 输出当前字符, 更新m_pos位置
void CvxText::putWChar(Mat& img, wchar_t wc, Point& pos, Scalar color)
{
	// 根据unicode生成字体的二值位图
	FT_UInt glyph_index = FT_Get_Char_Index(m_face, wc);
	FT_Load_Glyph(m_face, glyph_index, FT_LOAD_DEFAULT);
	FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_MONO);

	FT_GlyphSlot slot = m_face->glyph;

	// 行列数
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

					// 进行色彩融合
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

	// 修改下一个字的输出位置
	double space = m_fontSize.val[0] * m_fontSize.val[1];
	double sep = m_fontSize.val[0] * m_fontSize.val[2];

	pos.x += (int)((cols ? cols : space) + sep);
}

static wchar_t* ToWchar(char* &src, const char *locale = "zh_CN.utf8")
{
	if (src == NULL) {
		return 0;
	}

	// 根据环境变量设置locale
	setlocale(LC_CTYPE, locale);

	// 得到转化为需要的宽字符大小
	int w_size = mbstowcs(NULL, src, 0) + 1;

	// w_size = 0 说明mbstowcs返回值为-1。即在运行过程中遇到了非法字符(很有可能使locale
	// 没有设置正确)
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
/*时间打印*/
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

	// printf("打开图之前[%s]\n", log_Time());
	CvxText text("c:/windows/fonts/msyh.ttc"); //指定字体
	Scalar size1{ 40, 0.5, 0.1, 0 }; // (字体大小, 无效的, 字符间距, 无效的 }
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
		//printf("打开图之后[%s]\n", log_Time());
		Mat image(input);
		/*图片大小*/
		//std::cout << "size (after reading): " << image.height << " , "<< image.width << std::endl;

		//画框
		//rectangle(image, Point(155, 693), Point(349, 1073), Scalar(0, 0, 255), 3, 1);

		//字母文字
		//putText(image, "HELLO", Point(160, 1065), CV_FONT_HERSHEY_COMPLEX, 1, Scalar(0,0, 255), 2, 1);

		char* str = (char *)"你好123！";
		if (wchar_t *w_str = ToWchar(str)) {
			text.putText(image, w_str, Point(160, 100), Scalar(0, 0, 255));
			delete[] w_str;
		}

		//printf("绘制图之后[%s]\n", log_Time());  
		if ((i % step) == 0){
			image.save("output.jpg");
			printf("第%d张图保存[%s]\n", i, log_Time());
		}
	}

	printf("end[%s]\n", log_Time());
	getchar();
	return 0;
}
#endif
