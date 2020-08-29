#pragma once

#include <string>

enum class RenderFont
{
	BITMAP_8_BY_13,
	BITMAP_9_BY_15,
	BITMAP_TIMES_ROMAN_10,
	BITMAP_TIMES_ROMAN_24,
	BITMAP_HELVETICA_10,
	BITMAP_HELVETICA_12,
	BITMAP_HELVETICA_18,
};

class Renderer
{
private:
	Renderer() {}

public:
	template<typename T>
	static constexpr auto BYTE_TO_FLOAT_COLOR(T b) { return b / 255.0f; }

	static void color3b(unsigned char r, unsigned char g, unsigned char b);
	static void drawQuad2D(int x, int y, int width, int height);
	static void clearColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
	static void color3f(float r, float g, float b);
	static void color4f(float r, float g, float b, float a);
	static void pushMatrix();
	static void translatef(float x, float y, float z);
	static void wireCube(double size);
	static void popMatrix();
	static void renderString(int x, int y, RenderFont font, const std::string& str);
	static int getStringWidth(RenderFont font, const std::string& string);
};