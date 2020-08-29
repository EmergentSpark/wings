#pragma once

#include <vector>
#include <memory>
#include <string>

#include "UIWindow.h"

class UIWindowManager
{
private:
	UIWindowManager() {}

public:
	static void addWindow(UIWindow* window);
	static UIWindow* getWindowByTitle(const std::string& title);
	static std::vector<std::unique_ptr<UIWindow>>& getWindows();
	static void setMousePos(int x, int y);
	static const glm::ivec2& getMousePos();
};