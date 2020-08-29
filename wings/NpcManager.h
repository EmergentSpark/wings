#pragma once

#include <vector>
#include <memory>

#include <glm/vec3.hpp>

struct Npc
{
	int id;
	glm::vec3 position;

	Npc(int _id, const glm::vec3& _pos) : id(_id), position(_pos) {}

	void draw();
};

class NpcManager
{
private:
	NpcManager() {}

public:
	static Npc* loadNpc(int id, const glm::vec3& pos);
	static void unloadNpc(int id);
	static void clearNpcs();
	static std::vector<std::unique_ptr<Npc>>& getNpcs();
};