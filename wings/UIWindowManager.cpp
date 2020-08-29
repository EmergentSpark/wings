#include "UIWindowManager.h"

glm::ivec2 currentMousePos;
std::vector<std::unique_ptr<UIWindow>> uiWindows;

void UIWindowManager::addWindow(UIWindow* window) { uiWindows.push_back(std::unique_ptr<UIWindow>(window)); }

UIWindow* UIWindowManager::getWindowByTitle(const std::string& title)
{
	UIWindow* ret = 0;
	for (auto& window : uiWindows)
	{
		if (window->getTitle() == title)
		{
			ret = window.get();
			break;
		}
	}
	return ret;
}

std::vector<std::unique_ptr<UIWindow>>& UIWindowManager::getWindows() { return uiWindows; }

void UIWindowManager::setMousePos(int x, int y) { currentMousePos.x = x; currentMousePos.y = y; }

const glm::ivec2& UIWindowManager::getMousePos() { return currentMousePos; }