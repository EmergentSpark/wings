#pragma once

class Randomizer
{
private:
	Randomizer() {}

public:
	static int getRandomInt();
	static int getRandomInt(int min, int max);
	static double getRandomDouble();
	static float getRandomFloat();
};