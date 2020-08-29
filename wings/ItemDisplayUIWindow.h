#pragma once

#include "UIWindow.h"
#include "ItemInformationProvider.h"

class ItemDisplayUIWindow : public UIWindow
{
protected:
	void drawItemTooltip(const glm::ivec2& curPos, Item* item)
	{
		pushTransformMatrix();
		Renderer::color4f(0.25f, 0.25f, 0.25f, 0.9f);
		quad(curPos.x + 20, curPos.y + 20, 175, 100);
		Renderer::color3f(1.0f, 1.0f, 1.0f);
		ItemInfo* info = ItemInformationProvider::getItemInfo(item->getItemId());
		if (info)
		{
			text(curPos.x + 23, curPos.y + 23 + 18, RenderFont::BITMAP_HELVETICA_18, info->getName());
			text(curPos.x + 23, curPos.y + 23 + 18 + 3 + 12, RenderFont::BITMAP_HELVETICA_12, info->getDescription());

			if (item->getInventoryType() == InventoryType::EQUIP)
			{
				Equip* equip = (Equip*)item;
				int yOff = curPos.y + 23 + 18 + 3 + 12 + 3 + 10;
				if (equip->getHP() > 0) { text(curPos.x + 23, yOff, RenderFont::BITMAP_HELVETICA_10, "HP +" + std::to_string(equip->getHP())); yOff += 13; }
				if (equip->getMP() > 0) { text(curPos.x + 23, yOff, RenderFont::BITMAP_HELVETICA_10, "MP +" + std::to_string(equip->getMP())); yOff += 13; }
				if (equip->getAttackDamage() > 0) { text(curPos.x + 23, yOff, RenderFont::BITMAP_HELVETICA_10, "Attack +" + std::to_string(equip->getAttackDamage())); yOff += 13; }
				if (equip->getMoveSpeed() > 0) { text(curPos.x + 23, yOff, RenderFont::BITMAP_HELVETICA_10, "Move Speed +" + std::to_string(equip->getMoveSpeed())); yOff += 13; }
			}
		}
		else
		{
			text(curPos.x + 23, curPos.y + 23 + 12, RenderFont::BITMAP_HELVETICA_12, "Item information not loaded.");
		}
		popTransformMatrix();
	}

public:
	ItemDisplayUIWindow(const glm::ivec2& pos, const glm::ivec2& size, const std::string& title) : UIWindow(pos, size, title) {}
};