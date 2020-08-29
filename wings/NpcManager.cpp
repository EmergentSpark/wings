#include "NpcManager.h"

#include "Renderer.h"

void Npc::draw()
{
	Renderer::color3f(0.0f, 0.0f, 1.0f);
	Renderer::pushMatrix();
	Renderer::translatef(position.x, position.y + 1, position.z);
	Renderer::wireCube(2.0);
	Renderer::popMatrix();
}

std::vector<std::unique_ptr<Npc>> loadedNPCs;

Npc* NpcManager::loadNpc(int id, const glm::vec3& pos)
{
	Npc* ret = new Npc(id, pos);
	loadedNPCs.push_back(std::unique_ptr<Npc>(ret));
	return ret;
}

void NpcManager::unloadNpc(int id)
{
	for (unsigned int i = 0; i < loadedNPCs.size(); i++)
	{
		if (loadedNPCs[i]->id == id)
		{
			loadedNPCs.erase(loadedNPCs.begin() + i);
			break;
		}
	}
}

void NpcManager::clearNpcs() { loadedNPCs.clear(); }

std::vector<std::unique_ptr<Npc>>& NpcManager::getNpcs() { return loadedNPCs; }