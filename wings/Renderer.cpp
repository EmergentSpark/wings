#include "Renderer.h"

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

void Renderer::color3b(unsigned char r, unsigned char g, unsigned char b) { glColor3f(BYTE_TO_FLOAT_COLOR(r), BYTE_TO_FLOAT_COLOR(g), BYTE_TO_FLOAT_COLOR(b)); }

void Renderer::drawQuad2D(int x, int y, int width, int height)
{
	glBegin(GL_QUADS);
	glVertex2i(x, y);
	glVertex2i(x, y + height);
	glVertex2i(x + width, y + height);
	glVertex2i(x + width, y);
	glEnd();
}

void Renderer::clearColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { glClearColor(BYTE_TO_FLOAT_COLOR(r), BYTE_TO_FLOAT_COLOR(g), BYTE_TO_FLOAT_COLOR(b), BYTE_TO_FLOAT_COLOR(a)); }

void Renderer::color3f(float r, float g, float b) { glColor3f(r, g, b); }

void Renderer::color4f(float r, float g, float b, float a) { glColor4f(r, g, b, a); }

void Renderer::pushMatrix() { glPushMatrix(); }

void Renderer::translatef(float x, float y, float z) { glTranslatef(x, y, z); }

void Renderer::wireCube(double size) { glutWireCube(size); }

void Renderer::popMatrix() { glPopMatrix(); }

void* renderFonts[] = {
	GLUT_BITMAP_8_BY_13,
	GLUT_BITMAP_9_BY_15,
	GLUT_BITMAP_TIMES_ROMAN_10,
	GLUT_BITMAP_TIMES_ROMAN_24,
	GLUT_BITMAP_HELVETICA_10,
	GLUT_BITMAP_HELVETICA_12,
	GLUT_BITMAP_HELVETICA_18
};

void renderSpacedBitmapString(float x, float y, int spacing, void* font, const char* string)
{
	glPushMatrix();
	glLoadIdentity();

	const char* c;
	float x1 = x;

	for (c = string; *c != '\0'; c++)
	{
		glRasterPos2f(x1, y);
		glutBitmapCharacter(font, *c);
		x1 = x1 + glutBitmapWidth(font, *c) + spacing;
	}

	glPopMatrix();
}

void Renderer::renderString(int x, int y, RenderFont font, const std::string& str) { renderSpacedBitmapString((float)x, (float)y, 0, renderFonts[(int)font], str.c_str()); }

int getSpacedStringWidth(int spacing, void* font, const char* string)
{
	int width = 0;
	const char* c;

	for (c = string; *c != '\0'; c++)
	{
		width += glutBitmapWidth(font, *c) + spacing;
	}

	return width;
}

int Renderer::getStringWidth(RenderFont font, const std::string& string) { return getSpacedStringWidth(0, renderFonts[(int)font], string.c_str()); }