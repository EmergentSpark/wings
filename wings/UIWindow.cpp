#include "UIWindow.h"

#include "UIWindowManager.h"

glm::ivec2 UIWindow::getClientAreaMousePos() { return glm::ivec2(UIWindowManager::getMousePos().x - mPosition.x - mClientAreaOffset.x, UIWindowManager::getMousePos().y - mPosition.y - mClientAreaOffset.y); }

void UIWindow::onMouseMove()
{
	if (!mVisible) { return; }

	mouseMove(UIWindowManager::getMousePos().x - mPosition.x - mClientAreaOffset.x, UIWindowManager::getMousePos().y - mPosition.y - mClientAreaOffset.y);
}