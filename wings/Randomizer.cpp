#include "Randomizer.h"

#include <random>

std::random_device rd;
std::mt19937 mt(rd());

int Randomizer::getRandomInt() { return std::uniform_int_distribution<int>()(mt); }
int Randomizer::getRandomInt(int min, int max) { return std::uniform_int_distribution<int>(min, max)(mt); }
double Randomizer::getRandomDouble() { return std::uniform_real_distribution<double>()(mt); }
float Randomizer::getRandomFloat() { return std::uniform_real_distribution<float>()(mt); }