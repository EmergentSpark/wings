#pragma once

#include <string>

#include "Renderer.h"
#include "CombatEntity.h"

class ItemInfo
{
public:
	virtual std::string getName() = 0;
	virtual std::string getDescription() { return "No description."; }
	virtual void drawIcon() = 0;
	virtual void onUse(CombatEntity* user) {}
};

class Item_BasicRaycaster : public ItemInfo
{
public:
	virtual std::string getName() { return "Basic Raycaster"; }
	virtual void drawIcon()
	{
		Renderer::color3b(112, 112, 225);
		Renderer::drawQuad2D(11, 15, 8, 19);
		Renderer::drawQuad2D(19, 15, 19, 8);
		Renderer::drawQuad2D(19, 25, 3, 3);
	}
};

class Item_PowerRaycaster : public ItemInfo
{
public:
	virtual std::string getName() { return "Power Raycaster"; }
	virtual void drawIcon()
	{
		Renderer::color3b(25, 255, 201);
		Renderer::drawQuad2D(11, 15, 8, 19);
		Renderer::drawQuad2D(19, 15, 19, 8);
		Renderer::drawQuad2D(19, 25, 3, 3);
	}
};

class Item_BlastRaycaster : public ItemInfo
{
public:
	virtual std::string getName() { return "Blast Raycaster"; }
	virtual void drawIcon()
	{
		Renderer::color3b(255, 79, 56);
		Renderer::drawQuad2D(11, 15, 8, 19);
		Renderer::drawQuad2D(19, 15, 19, 8);
		Renderer::drawQuad2D(19, 25, 3, 3);
	}
};

class Item_BasicCharger : public ItemInfo
{
public:
	virtual std::string getName() { return "Basic Charger"; }
	virtual void drawIcon()
	{
		Renderer::color3b(112, 112, 225);
		Renderer::drawQuad2D(11, 15, 8, 19);
		Renderer::drawQuad2D(19, 15, 19, 8);
		Renderer::drawQuad2D(19, 25, 3, 3);
		Renderer::drawQuad2D(29, 23, 9, 11);
		Renderer::drawQuad2D(19, 31, 10, 3);
	}
};

class Item_PowerCharger : public ItemInfo
{
public:
	virtual std::string getName() { return "Power Charger"; }
	virtual void drawIcon()
	{
		Renderer::color3b(25, 255, 201);
		Renderer::drawQuad2D(11, 15, 8, 19);
		Renderer::drawQuad2D(19, 15, 19, 8);
		Renderer::drawQuad2D(19, 25, 3, 3);
		Renderer::drawQuad2D(29, 23, 9, 11);
		Renderer::drawQuad2D(19, 31, 10, 3);
	}
};

class Item_BlastCharger : public ItemInfo
{
public:
	virtual std::string getName() { return "Blast Charger"; }
	virtual void drawIcon()
	{
		Renderer::color3b(255, 79, 56);
		Renderer::drawQuad2D(11, 15, 8, 19);
		Renderer::drawQuad2D(19, 15, 19, 8);
		Renderer::drawQuad2D(19, 25, 3, 3);
		Renderer::drawQuad2D(29, 23, 9, 11);
		Renderer::drawQuad2D(19, 31, 10, 3);
	}
};

class Item_RedPotion : public ItemInfo
{
public:
	virtual std::string getName() { return "Red Potion"; }
	virtual std::string getDescription() { return "Restores 10 HP."; }
	virtual void drawIcon()
	{
		Renderer::color3b(131, 138, 142);
		Renderer::drawQuad2D(12, 12, 24, 24);
		Renderer::drawQuad2D(18, 10, 12, 2);
		Renderer::color3b(77, 81, 84);
		Renderer::drawQuad2D(18, 8, 12, 2);
		Renderer::color3b(173, 183, 188);
		Renderer::drawQuad2D(20, 12, 8, 2);
		Renderer::drawQuad2D(14, 14, 20, 2);
		Renderer::color3b(142, 42, 54);
		Renderer::drawQuad2D(14, 16, 20, 18);
	}
	virtual void onUse(CombatEntity* user)
	{
		user->setHP(user->getHP() + 10);
	}
};

class Item_OrangePotion : public ItemInfo
{
public:
	virtual std::string getName() { return "Orange Potion"; }
	virtual std::string getDescription() { return "Restores 50 HP."; }
	virtual void drawIcon()
	{
		Renderer::color3b(131, 138, 142);
		Renderer::drawQuad2D(12, 12, 24, 24);
		Renderer::drawQuad2D(18, 10, 12, 2);
		Renderer::color3b(77, 81, 84);
		Renderer::drawQuad2D(18, 8, 12, 2);
		Renderer::color3b(173, 183, 188);
		Renderer::drawQuad2D(20, 12, 8, 2);
		Renderer::drawQuad2D(14, 14, 20, 2);
		Renderer::color3b(242, 137, 58);
		Renderer::drawQuad2D(14, 16, 20, 18);
	}
	virtual void onUse(CombatEntity* user)
	{
		user->setHP(user->getHP() + 50);
	}
};

class Item_WhitePotion : public ItemInfo
{
public:
	virtual std::string getName() { return "White Potion"; }
	virtual std::string getDescription() { return "Restores 100 HP."; }
	virtual void drawIcon()
	{
		Renderer::color3b(131, 138, 142);
		Renderer::drawQuad2D(12, 12, 24, 24);
		Renderer::drawQuad2D(18, 10, 12, 2);
		Renderer::color3b(77, 81, 84);
		Renderer::drawQuad2D(18, 8, 12, 2);
		Renderer::color3b(173, 183, 188);
		Renderer::drawQuad2D(20, 12, 8, 2);
		Renderer::drawQuad2D(14, 14, 20, 2);
		Renderer::color3b(239, 237, 227);
		Renderer::drawQuad2D(14, 16, 20, 18);
	}
	virtual void onUse(CombatEntity* user)
	{
		user->setHP(user->getHP() + 100);
	}
};

class Item_BluePotion : public ItemInfo
{
public:
	virtual std::string getName() { return "Blue Potion"; }
	virtual std::string getDescription() { return "Restores 10 MP."; }
	virtual void drawIcon()
	{
		Renderer::color3b(131, 138, 142);
		Renderer::drawQuad2D(12, 12, 24, 24);
		Renderer::drawQuad2D(18, 10, 12, 2);
		Renderer::color3b(77, 81, 84);
		Renderer::drawQuad2D(18, 8, 12, 2);
		Renderer::color3b(173, 183, 188);
		Renderer::drawQuad2D(20, 12, 8, 2);
		Renderer::drawQuad2D(14, 14, 20, 2);
		Renderer::color3b(50, 80, 140);
		Renderer::drawQuad2D(14, 16, 20, 18);
	}
	virtual void onUse(CombatEntity* user)
	{
		user->setMP(user->getMP() + 10);
	}
};

class Item_ManaElixir : public ItemInfo
{
public:
	virtual std::string getName() { return "Mana Elixir"; }
	virtual std::string getDescription() { return "Restores 100 MP."; }
	virtual void drawIcon()
	{
		Renderer::color3b(131, 138, 142);
		Renderer::drawQuad2D(12, 12, 24, 24);
		Renderer::drawQuad2D(18, 10, 12, 2);
		Renderer::color3b(77, 81, 84);
		Renderer::drawQuad2D(18, 8, 12, 2);
		Renderer::color3b(173, 183, 188);
		Renderer::drawQuad2D(20, 12, 8, 2);
		Renderer::drawQuad2D(14, 14, 20, 2);
		Renderer::color3b(11, 19, 237);
		Renderer::drawQuad2D(14, 16, 20, 18);
	}
	virtual void onUse(CombatEntity* user)
	{
		user->setMP(user->getMP() + 100);
	}
};