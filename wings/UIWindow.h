#pragma once

#include <string>

#include <glm/vec2.hpp>

#include "Renderer.h"

class UIWindow
{
private:
	glm::ivec2 mPosition;
	glm::ivec2 mSize;
	std::string mTitle;
	bool mVisible = false;
	glm::ivec2 mClientAreaOffset = glm::ivec2(5, 25);

protected:
	void quad(int x, int y, int width, int height) { Renderer::drawQuad2D(x, y, width, height); }
	void text(int x, int y, RenderFont font, const std::string& str) { Renderer::renderString(mPosition.x + mClientAreaOffset.x + x, mPosition.y + mClientAreaOffset.y + y, font, str); }

	void pushTransformMatrix()
	{
		Renderer::pushMatrix();
		Renderer::translatef((float)(mPosition.x + mClientAreaOffset.x), (float)(mPosition.y + mClientAreaOffset.y), 0.0f);
	}

	void popTransformMatrix()
	{
		Renderer::popMatrix();
	}

	virtual void draw() = 0;
	virtual void click(int x, int y) {}
	virtual void mouseMove(int x, int y) {}
	virtual void mouseDrag(int x, int y) {}

	glm::ivec2 getClientAreaMousePos();

public:
	UIWindow(const glm::ivec2& pos, const glm::ivec2& size, const std::string& title) : mPosition(pos), mSize(size), mTitle(title) {}

	const glm::ivec2& getPosition() const { return mPosition; }
	void setPosition(const glm::ivec2& pos) { mPosition = pos; }
	const glm::ivec2& getSize() const { return mSize; }
	const std::string& getTitle() const { return mTitle; }
	const bool& getVisible() const { return mVisible; }
	void setVisible(bool visible) { mVisible = visible; }

	void onDraw()
	{
		if (!mVisible) { return; }

		Renderer::color4f(0.0f, 0.0f, 0.0f, 0.75f);
		Renderer::drawQuad2D(mPosition.x, mPosition.y, mSize.x, mSize.y);
		Renderer::color4f(1.0f, 1.0f, 1.0f, 0.75f);
		Renderer::drawQuad2D(mPosition.x + 5, mPosition.y + 5, mSize.x - 10, mSize.y - 10);
		Renderer::drawQuad2D(mPosition.x + 10, mPosition.y + 7, mSize.x - 40, 20); // title bar bg
		Renderer::color4f(0.0f, 0.0f, 0.0f, 1.0f);
		Renderer::renderString(mPosition.x + 5 + (mSize.x / 2) - (Renderer::getStringWidth(RenderFont::BITMAP_HELVETICA_18, mTitle) / 2), mPosition.y + 5 + 20, RenderFont::BITMAP_HELVETICA_18, mTitle);

		// x button
		Renderer::color4f(0.0f, 0.0f, 0.0f, 0.75f);
		Renderer::drawQuad2D(mPosition.x + mSize.x - 26, mPosition.y + 7, 19, 20);
		Renderer::color4f(1.0f, 1.0f, 1.0f, 0.75f);
		Renderer::renderString(mPosition.x + mSize.x - 21, mPosition.y + 7 + 15, RenderFont::BITMAP_HELVETICA_18, "x");

		pushTransformMatrix();
		draw();
		popTransformMatrix();
	}

	bool onClick(int x, int y)
	{
		if (!mVisible) { return false; }

		if (x >= mPosition.x && x <= mPosition.x + mSize.x && y >= mPosition.y && y <= mPosition.y + mSize.y)
		{
			// toggle visiblity on x button press
			glm::ivec2 xbPos(mPosition.x + mSize.x - 26, mPosition.y + 7);
			if (x >= xbPos.x && y >= xbPos.y && x <= xbPos.x + 19 && y <= xbPos.y + 20) { mVisible = false; }
			else { click(x - mPosition.x - mClientAreaOffset.x, y - mPosition.y - mClientAreaOffset.y); }

			return true;
		}

		return false;
	}

	void onMouseMove();

	void onMouseDrag(int x, int y)
	{
		if (!mVisible) { return; }

		mouseDrag(x - mPosition.x - mClientAreaOffset.x, y - mPosition.y - mClientAreaOffset.y);
	}
};