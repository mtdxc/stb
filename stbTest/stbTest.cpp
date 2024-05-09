#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */
#ifdef _WIN32
#include <tchar.h>
#else
#define TCHAR char
#endif
#include <memory>
int enc_unicode_to_utf8_one(unsigned long unic, unsigned char *pOutput, int outSize)
{
	assert(pOutput != NULL);
	assert(outSize >= 6);

	if (unic <= 0x0000007F)
	{
		// * U-00000000 - U-0000007F:  0xxxxxxx
		*pOutput = (unic & 0x7F);
		return 1;
	}
	else if (unic >= 0x00000080 && unic <= 0x000007FF)
	{
		// * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
		*(pOutput + 1) = (unic & 0x3F) | 0x80;
		*pOutput = ((unic >> 6) & 0x1F) | 0xC0;
		return 2;
	}
	else if (unic >= 0x00000800 && unic <= 0x0000FFFF)
	{
		// * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
		*(pOutput + 2) = (unic & 0x3F) | 0x80;
		*(pOutput + 1) = ((unic >> 6) & 0x3F) | 0x80;
		*pOutput = ((unic >> 12) & 0x0F) | 0xE0;
		return 3;
	}
	else if (unic >= 0x00010000 && unic <= 0x001FFFFF)
	{
		// * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		*(pOutput + 3) = (unic & 0x3F) | 0x80;
		*(pOutput + 2) = ((unic >> 6) & 0x3F) | 0x80;
		*(pOutput + 1) = ((unic >> 12) & 0x3F) | 0x80;
		*pOutput = ((unic >> 18) & 0x07) | 0xF0;
		return 4;
	}
	else if (unic >= 0x00200000 && unic <= 0x03FFFFFF)
	{
		// * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		*(pOutput + 4) = (unic & 0x3F) | 0x80;
		*(pOutput + 3) = ((unic >> 6) & 0x3F) | 0x80;
		*(pOutput + 2) = ((unic >> 12) & 0x3F) | 0x80;
		*(pOutput + 1) = ((unic >> 18) & 0x3F) | 0x80;
		*pOutput = ((unic >> 24) & 0x03) | 0xF8;
		return 5;
	}
	else if (unic >= 0x04000000 && unic <= 0x7FFFFFFF)
	{
		// * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		*(pOutput + 5) = (unic & 0x3F) | 0x80;
		*(pOutput + 4) = ((unic >> 6) & 0x3F) | 0x80;
		*(pOutput + 3) = ((unic >> 12) & 0x3F) | 0x80;
		*(pOutput + 2) = ((unic >> 18) & 0x3F) | 0x80;
		*(pOutput + 1) = ((unic >> 24) & 0x3F) | 0x80;
		*pOutput = ((unic >> 30) & 0x01) | 0xFC;
		return 6;
	}

	return 0;
}

int enc_get_utf8_size(unsigned char pInput) {
	int ret = 0;
	for (; ret < 7; ret++) {
		if (!(pInput & (1 << (8 - ret))))
			break;
	}
	return ret;
}

int enc_utf8_to_unicode_one(const unsigned char* pInput, unsigned long *Unic)
{
	assert(pInput != NULL && Unic != NULL);

	// b1 ��ʾUTF-8�����pInput�еĸ��ֽ�, b2 ��ʾ�θ��ֽ�, ...
	char b1, b2, b3, b4, b5, b6;

	*Unic = 0x0; // �� *Unic ��ʼ��Ϊȫ��
	int utfbytes = enc_get_utf8_size(*pInput);
	unsigned char *pOutput = (unsigned char *)Unic;

	switch (utfbytes)
	{
	case 0:
		*pOutput = *pInput;
		utfbytes += 1;
		break;
	case 2:
		b1 = *pInput;
		b2 = *(pInput + 1);
		if ((b2 & 0xE0) != 0x80)
			return 0;
		*pOutput = (b1 << 6) + (b2 & 0x3F);
		*(pOutput + 1) = (b1 >> 2) & 0x07;
		break;
	case 3:
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80))
			return 0;
		*pOutput = (b2 << 6) + (b3 & 0x3F);
		*(pOutput + 1) = (b1 << 4) + ((b2 >> 2) & 0x0F);
		break;
	case 4:
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		b4 = *(pInput + 3);
		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
			|| ((b4 & 0xC0) != 0x80))
			return 0;
		*pOutput = (b3 << 6) + (b4 & 0x3F);
		*(pOutput + 1) = (b2 << 4) + ((b3 >> 2) & 0x0F);
		*(pOutput + 2) = ((b1 << 2) & 0x1C) + ((b2 >> 4) & 0x03);
		break;
	case 5:
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		b4 = *(pInput + 3);
		b5 = *(pInput + 4);
		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
			|| ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80))
			return 0;
		*pOutput = (b4 << 6) + (b5 & 0x3F);
		*(pOutput + 1) = (b3 << 4) + ((b4 >> 2) & 0x0F);
		*(pOutput + 2) = (b2 << 2) + ((b3 >> 4) & 0x03);
		*(pOutput + 3) = (b1 << 6);
		break;
	case 6:
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		b4 = *(pInput + 3);
		b5 = *(pInput + 4);
		b6 = *(pInput + 5);
		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
			|| ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80)
			|| ((b6 & 0xC0) != 0x80))
			return 0;
		*pOutput = (b5 << 6) + (b6 & 0x3F);
		*(pOutput + 1) = (b5 << 4) + ((b6 >> 2) & 0x0F);
		*(pOutput + 2) = (b3 << 2) + ((b4 >> 4) & 0x03);
		*(pOutput + 3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);
		break;
	default:
		return 0;
		break;
	}

	return utfbytes;
}

// �����ļ�
class stbFont {
	stbtt_fontinfo info_;
	std::shared_ptr<uint8_t> buffer_;
	float scale = 1.0f;
	/**
	* ��ȡ��ֱ�����ϵĶ���
	* ascent������ӻ��ߵ������ĸ߶ȣ�
	* descent�����ߵ��ײ��ĸ߶ȣ�ͨ��Ϊ��ֵ��
	* lineGap����������֮��ļ�ࣻ
	* �м��Ϊ��ascent - descent + lineGap��
	*/
	int ascent = 0;
	int lineGap = 0;
	int descent = 0;
public:
	stbFont(){}
	bool open(const char* path, int index = 0) {
		FILE *fontFile = fopen(path, "rb");
		if (fontFile == NULL)
		{
			printf("Can not open font file %s!\n", path);
			return 0;
		}
		fseek(fontFile, 0, SEEK_END); /* �����ļ�ָ�뵽�ļ�β�������ļ�βƫ��0�ֽ� */
		long size = ftell(fontFile);       /* ��ȡ�ļ���С���ļ�β - �ļ�ͷ  ��λ���ֽڣ� */
		fseek(fontFile, 0, SEEK_SET); /* ���������ļ�ָ�뵽�ļ�ͷ */

		uint8_t* buffer = (uint8_t*)malloc(size);
		buffer_ = std::shared_ptr<uint8_t>(buffer, &free);
		fread(buffer, size, 1, fontFile);
		fclose(fontFile);

		int offset = 0;
		int nFonts = stbtt_GetNumberOfFonts(buffer);
		if (index >=0 && index < nFonts)
			offset = stbtt_GetFontOffsetForIndex(buffer, index);
		printf("%s with %d/%d fonts, use offeset %d\n", path, index, nFonts, offset);
		if (!stbtt_InitFont(&info_, buffer, offset))
		{
			printf("stb init font failed\n");
			return false;
		}
		return setFontSize(32);
	}

	bool setFontSize(float pixels) {
		/* ������������ */
		// float pixels = 64.0;                                    /* �����С���ֺţ� */
		scale = stbtt_ScaleForPixelHeight(&info_, pixels); /* scale = pixels / (ascent - descent) */

		lineGap = descent = ascent = 0;
		stbtt_GetFontVMetrics(&info_, &ascent, &descent, &lineGap);
		return true;
	}
	bool calcuteSize(const int* text, int& width, int& height) {
		width = height = 0;
		int nextX = 0;
		const int lineH = roundf(scale * (ascent - descent + lineGap));
		while (text) {
			// @todo getUtf8 codepage
			int codep = *text++;
			if (codep == '\n' || 0==codep) {
				height += lineH;
				if (width < nextX)
					width = nextX;
				nextX = 0;
				if (!codep) break;
			}
			else{
				/**
				* ��ȡˮƽ�����ϵĶ���
				* advanceWidth���ֿ�
				* leftSideBearing�����λ�ã�
				*/
				int advanceWidth = 0;
				int leftSideBearing = 0;
				stbtt_GetCodepointHMetrics(&info_, codep, &advanceWidth, &leftSideBearing);
				leftSideBearing = roundf(leftSideBearing * scale);
				advanceWidth = roundf(advanceWidth * scale);

				/* ��ȡ�ַ��ı߿򣨱߽磩
				int c_x1, c_y1, c_x2, c_y2;
				stbtt_GetCodepointBitmapBox(&info_, codep, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);
				// real write pos
				int y = height + roundf(scale * ascent) + c_y1;
				int x = nextX + leftSideBearing + c_x1;
				*/
				/* ����x */
				nextX += advanceWidth;

				/* �����־� */
				if (text[1]) {
					nextX += roundf(scale * stbtt_GetCodepointKernAdvance(&info_, codep, text[1]));
				}
			}
		}
		return true;
	}
	int drawText(const char* text, uint32_t color,
		uint8_t* bmp, int x, int y, int stride, int channel,
		int& owidth, int& oheight) {
		// utf-8 to unicode
		unsigned long code;
		std::vector<int> codes;
		while (*text) {
			text += enc_utf8_to_unicode_one((const uint8_t*)text,&code);
			codes.push_back(code);
		}
		codes.push_back(0);
		return drawText(codes.data(), color, bmp, x, y, stride, channel, owidth, oheight);
	}
	int drawText(const wchar_t* text, uint32_t color,
		uint8_t* bmp, int x, int y, int stride, int channel,
		int& owidth, int& oheight) {
		std::vector<int> codes;
		while (*text) {
			codes.push_back(*text++);
		}
		codes.push_back(0);
		return drawText(codes.data(), color, bmp, x, y, stride, channel, owidth, oheight);
	}
	// ����unicode
	int drawText(const int* text, ///< unicode�ı�������0��β 
		uint32_t color, ///< ������ɫ
		uint8_t* bmp, ///< ���λͼ
		int x, int y, ///< ���λ�� 
		int width, int channel, ///< ���λͼ���ͨ���� 1,3,4
		int& owidth, int& oheight ///< �ı�ռ�õĿ�͸�
		) {
		owidth = oheight = 0;
		int nextX = 0;
		int lineH = roundf(scale * (ascent - descent + lineGap));
		while (text){
			int codep = *text++;
			if (codep == '\n' || 0 == codep) {
				oheight += lineH;
				if (owidth < nextX)
					owidth = nextX;
				nextX = 0;
				if (!codep) break;
			}
			else{
				/**
				* ��ȡˮƽ�����ϵĶ���
				* advanceWidth���ֿ�
				* leftSideBearing�����λ�ã�
				*/
				int advanceWidth = 0;
				int leftSideBearing = 0;
				stbtt_GetCodepointHMetrics(&info_, codep, &advanceWidth, &leftSideBearing);
				leftSideBearing = roundf(leftSideBearing * scale);
				advanceWidth = roundf(advanceWidth * scale);

				/* ��ȡ�ַ��ı߿򣨱߽磩 */
				int c_x1, c_y1, c_x2, c_y2;
				stbtt_GetCodepointBitmapBox(&info_, codep, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

				int ry = oheight + roundf(scale *ascent) + c_y1;
				int rx = nextX + leftSideBearing + c_x1;
				int stride = width * channel;
				/* ��Ⱦ�ַ� */
				uint8_t* d = bmp + (x + rx) * channel + (ry + y) * stride;
				// stbtt_MakeCodepointBitmap(&info_, d, c_x2 - c_x1, c_y2 - c_y1, stride_x, scale, scale, codep);
				int w, h;
				uint8_t* s = stbtt_GetCodepointBitmap(&info_, scale, scale, codep, &w, &h, 0, 0);
				for (int j = 0; j < h; j++) {
					for (size_t i = 0; i < w; i++) {
						uint8_t& val8 = s[j*w + i];
						if (val8 > 0) {
							float alpha = val8 / 255.0;
							// uint32_t val = alpha * color;
							uint8_t* p = d + i*channel;
							uint8_t* s = (uint8_t*)&color;
							// ������ֽ��ڵ�λ
							int n = channel;
							if (n > 3) n = 3;
							for (int i = 0; i < n; i++) {
								p[i] = s[i] * alpha + (1 - alpha) * p[i];
							}
						}
					}
					d += stride;
				}
				stbtt_FreeBitmap(s, nullptr);

				/* ����x */
				nextX += advanceWidth;

				/* �����־� */
				if (text[1]) {
					nextX += roundf(scale * stbtt_GetCodepointKernAdvance(&info_, codep, text[1]));
				}
			}
		}
		return lineH;
	}
};

int main(int argc, const char *argv[])
{
	/* �������壨.ttf���ļ� */
	const TCHAR* word = _T("STB\nHello ����\n abcd!!!");
	const char* word2 = u8"���!!!";
	const char* fpath = "input.jpg";
	if (argc>1) fpath = argv[1];
	//if (argc>2) word = argv[2];

	stbFont font;
	if (!font.open("c:/windows/fonts/msyh.ttc")) {
		return -1;
	}
	font.setFontSize(64);
	/* ����λͼ */
	int bitmap_w = 512; /* λͼ�Ŀ� */
	int bitmap_h = 328; /* λͼ�ĸ� */
	int channel = 3;
	unsigned char* bitmap = stbi_load(fpath, &bitmap_w, &bitmap_h, &channel, 3);
	if (!bitmap) {
		printf("uanble to open file %s\n", fpath);
		return -1;
	}

	int width, height;
	font.drawText(word, 0x000000FF, bitmap, 10, 10, bitmap_w, channel, width, height);

	/* ��λͼ���ݱ��浽1ͨ����pngͼ���� */
	stbi_write_png("stb.png", bitmap_w, bitmap_h, channel, bitmap, bitmap_w * channel);

	stbi_image_free(bitmap);

	return 0;
}
/*
ʹ��stb_truetype�����ttf����Ĳ���ͨ�����£�
1�����ز���ʼ��ttf�����ļ���
2�����������С���ֺţ����������ű�����
3����ȡ��ֱ�����ϵĶ������������ű��������������ָߡ��м��ȣ�
4����ȡˮƽ�����ϵĶ����������ֿ��ּ��ȣ�
5����ȡ�ַ�ͼƬ�ı߿�ÿ���ַ�ת��Ϊͼ��ı߽磩��
6������ÿ������ͼ��Ŀ�ߣ������е�x��y��������Ⱦ���壻

��Ҫ�����Ĳ�����Ҫ�������С���ֺţ���ʹ�ù�������Ҫע���������㣺
1�������Ѿ��������������һ�飬�ڰ���stb_truetype.hͷ�ļ���ʱ����Ҫ����STB_TRUETYPE_IMPLEMENTATION�����򽫻��޷�ʹ�á�
2������stb_truetype�⺯��������ַ����������unicode���롣
*/
#if 0
if((str[i] & 0x80) == 0) {
    g = stbtt_FindGlyphIndex(&f, str[i]);
} else {
    if (str[i] >= 0xC0 && str[i] <= 0xDF) { 
        // then ���ֽ� UTF-8 ռ��2���ֽڣ�Ϊ����������ַ���
        /* for chinese */
        cn[0] = str[i];
        cn[1] = str[i+1];
        cn[2] = '\0';
    } else {
        /* for chinese */
        cn[0] = str[i];
        cn[1] = str[i+1];
        cn[2] = str[i+2];
        cn[3] = '\0';
    }
    int ret = Utf82Unicode(cn, unicode);
    int *un_number = (int *)unicode;
    //log_debug("[%s:%d] uni %x", __FUNCTION__, __LINE__, *un_number);
    g = stbtt_FindGlyphIndex(&f, *un_number);
}
#endif