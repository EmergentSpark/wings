#pragma once

#include "Item.h"

class Equip : public Item
{
private:
	int mHP = 0;
	int mMP = 0;
	int mAttackDamage = 0;
	float mMoveSpeed = 0.0f;

public:
	Equip(int id) : Item(id) {}

	int getHP() { return mHP; }
	int getMP() { return mMP; }
	int getAttackDamage() { return mAttackDamage; }
	float getMoveSpeed() { return mMoveSpeed; }

	void setHP(int hp) { mHP = hp; }
	void setMP(int mp) { mMP = mp; }
	void setAttackDamage(int attack) { mAttackDamage = attack; }
	void setMoveSpeed(float movespeed) { mMoveSpeed = movespeed; }
};