#pragma once

#include <vector>
#include <algorithm>

#include "AxisAlignedBoundingBox.h"

class CombatEntity;

class ICombatEntityListener
{
public:
	virtual void onKilled(CombatEntity* entity) = 0;
};

class CombatEntity
{
private:
	int mHP;
	int mMP;

	int mMaxHP;
	int mMaxMP;
	int mAttackDamage;
	float mMoveSpeed;

	int mBonusMaxHP = 0;
	int mBonusMaxMP = 0;
	int mBonusAttackDamage = 0;
	float mBonusMoveSpeed = 0.0f;

	int mTrueMaxHP;
	int mTrueMaxMP;
	int mTrueAttackDamage;
	float mTrueMoveSpeed;

	void recalcMaxHP() { mTrueMaxHP = mMaxHP + mBonusMaxHP; }
	void recalcMaxMP() { mTrueMaxMP = mMaxMP + mBonusMaxMP; }
	void recalcAttackDamage() { mTrueAttackDamage = mAttackDamage + mBonusAttackDamage; }
	void recalcMoveSpeed() { mTrueMoveSpeed = mMoveSpeed + mBonusMoveSpeed; }

protected:
	glm::vec3 mPosition;

	std::vector<ICombatEntityListener*> mListeners;

public:
	CombatEntity()
	{
		setBaseMaxHP(5);
		setBaseMaxMP(5);
		setBaseAttackDamage(1);
		setBaseMoveSpeed(7.5f);

		setHP(getMaxHP());
		setMP(getMaxMP());
	}

	const glm::vec3& getPosition() const { return mPosition; }
	void setPosition(const glm::vec3& pos) { mPosition = pos; }

	AxisAlignedBoundingBox getAxisAlignedBoundingBox()
	{
		glm::vec3 lowerLeft(getPosition());
		glm::vec3 upperRight(lowerLeft);
		lowerLeft.x--;
		lowerLeft.z--;
		upperRight.x++;
		upperRight.z++;
		upperRight.y += 2;

		return AxisAlignedBoundingBox(lowerLeft, upperRight);
	}

	void addListener(ICombatEntityListener* listener) { mListeners.push_back(listener); }

	void onKilled() { for (auto& listener : mListeners) { listener->onKilled(this); } }

	// dependent stats

	int getHP() { return mHP; }
	void setHP(int hp) { mHP = std::min(hp, getMaxHP()); }
	int getMP() { return mMP; }
	void setMP(int mp) { mMP = std::min(mp, getMaxMP()); }

	// base stats

	int getBaseMaxHP() { return mMaxHP; }
	void setBaseMaxHP(int maxhp) { mMaxHP = maxhp; recalcMaxHP(); }
	int getBaseMaxMP() { return mMaxMP; }
	void setBaseMaxMP(int maxmp) { mMaxMP = maxmp; recalcMaxMP(); }
	int getBaseAttackDamage() { return mAttackDamage; }
	void setBaseAttackDamage(int attackDamage) { mAttackDamage = attackDamage; recalcAttackDamage(); }
	float getBaseMoveSpeed() { return mMoveSpeed; }
	void setBaseMoveSpeed(float movespeed) { mMoveSpeed = movespeed; recalcMoveSpeed(); }

	// bonus stats

	void setBonusMaxHP(int maxhp) { mBonusMaxHP = maxhp; recalcMaxHP(); }
	void setBonusMaxMP(int maxmp) { mBonusMaxMP = maxmp; recalcMaxMP(); }
	void setBonusAttackDamage(int attack) { mBonusAttackDamage = attack; recalcAttackDamage(); }
	void setBonusMoveSpeed(float movespeed) { mBonusMoveSpeed = movespeed; recalcMoveSpeed(); }

	// true stats

	int getMaxHP() { return mTrueMaxHP; }
	int getMaxMP() { return mTrueMaxMP; }
	int getAttackDamage() { return mTrueAttackDamage; }
	float getMoveSpeed() { return mTrueMoveSpeed; }
};