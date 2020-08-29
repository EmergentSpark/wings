#pragma once

#include <vector>
#include <functional>
#include <algorithm>
#include <unordered_map>
#include <memory>

#include "VecUtil.h"

namespace AStar
{
	std::vector<glm::ivec2> direction = {
			{ 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 },
			{ -1, -1 }, { 1, 1 }, { -1, 1 }, { 1, -1 }
	};

	using HeuristicFunction = std::function<unsigned int(const glm::ivec2&, const glm::ivec2&)>;

	class Heuristic
	{
	private:
		static glm::ivec2 getDelta(glm::ivec2 source_, glm::ivec2 target_)
		{
			return{ abs(source_.x - target_.x),  abs(source_.y - target_.y) };
		}

	public:
		static unsigned int manhattan(glm::ivec2 source_, glm::ivec2 target_)
		{
			auto delta = std::move(getDelta(source_, target_));
			return static_cast<unsigned int>(10 * (delta.x + delta.y));
		}

		static unsigned int euclidean(glm::ivec2 source_, glm::ivec2 target_)
		{
			auto delta = std::move(getDelta(source_, target_));
			return static_cast<unsigned int>(10 * sqrt(pow(delta.x, 2) + pow(delta.y, 2)));
		}

		static unsigned int octagonal(glm::ivec2 source_, glm::ivec2 target_)
		{
			auto delta = std::move(getDelta(source_, target_));
			return 10 * (delta.x + delta.y) + (-6) * std::min(delta.x, delta.y);
		}
	};

	struct Node
	{
		glm::ivec2 pos; // The position of the node on the field.
		Node* cameFrom; // For node n, cameFrom[n] is the node immediately preceding it on the cheapest path from start to n currently known.
		unsigned int gScore; // For node n, gScore[n] is the cost of the cheapest path from start to n currently known.
		unsigned int fScore; // For node n, fScore[n] := gScore[n] + h(n). fScore[n] represents our current best guess as to how short a path from start to finish can be if it goes through n.

		Node(const glm::ivec2& _pos) : pos(_pos), gScore(UINT_MAX), fScore(UINT_MAX), cameFrom(0) {}
		Node(const glm::ivec2& _pos, unsigned int _gScore, unsigned int _fScore) : pos(_pos), gScore(_gScore), fScore(_fScore), cameFrom(0) {}
	};

	class Pathfinder
	{
	private:
		HeuristicFunction heuristic;
		unsigned int directions;

		std::unordered_map<glm::ivec2, std::unique_ptr<Node>, KeyHash_GLMIVec2, KeyEqual_GLMIVec2> nodes;
		// The set of discovered nodes that may need to be (re-)expanded.
		// Initially, only the start node is known.
		// This is usually implemented as a min-heap or priority queue rather than a hash-set.
		std::vector<Node*> openSet;

	public:
		Pathfinder()
		{
			setDiagonalMovement(false);
			setHeuristic(&Heuristic::manhattan);
		}

		void setDiagonalMovement(bool enable_)
		{
			directions = (enable_ ? 8 : 4);
		}

		void setHeuristic(HeuristicFunction heuristic_)
		{
			heuristic = std::bind(heuristic_, std::placeholders::_1, std::placeholders::_2);
		}

		// A* finds a path from start to goal.
		// h is the heuristic function. h(n) estimates the cost to reach goal from node n.
		std::vector<glm::ivec2> findPath(const glm::ivec2& start, const glm::ivec2& goal, std::function<bool(const glm::ivec2&)> invalid)
		{
			openSet.push_back(new Node(start, 0, heuristic(start, goal)));
			nodes[start].reset(openSet[0]);

			while (!openSet.empty())
			{
				// This operation can occur in O(1) time if openSet is a min-heap or a priority queue
				//current := the node in openSet having the lowest fScore[] value
				Node* current = openSet[0];
				for (auto& node : openSet)
				{
					if (node->fScore <= current->fScore)
					{
						current = node;
					}
				}

				if (current->pos == goal)
				{
					std::vector<glm::ivec2> ret;
					while (current->cameFrom != 0)
					{
						ret.insert(ret.begin(), current->pos);
						current = current->cameFrom;
					}
					ret.insert(ret.begin(), current->pos);
					return ret;
				}

				openSet.erase(std::find(openSet.begin(), openSet.end(), current));

				// iterate through possible neighbors, skipping invalid ones
				for (unsigned int i = 0; i < directions; i++)
				{
					glm::ivec2 newPos(current->pos + direction[i]);
					if (invalid(newPos)) { continue; }
					Node* neighbor = nodes[newPos].get();
					if (neighbor == 0)
					{
						neighbor = new Node(newPos);
						nodes[newPos].reset(neighbor);
					}

					// d(current, neighbor) is the weight of the edge from current to neighbor
					// tentative_gScore is the distance from start to the neighbor through current
					unsigned int tentative_gScore = current->gScore + ((i < 4) ? 10 : 14);
					if (tentative_gScore < neighbor->gScore)
					{
						// This path to neighbor is better than any previous one. Record it!
						neighbor->cameFrom = current;
						neighbor->gScore = tentative_gScore;
						neighbor->fScore = neighbor->gScore + heuristic(neighbor->pos, goal);
						if (std::find(openSet.begin(), openSet.end(), neighbor) == openSet.end())
						{
							openSet.push_back(neighbor);
						}
					}
				}
			}

			// Open set is empty but goal was never reached
			return std::vector<glm::ivec2>();
		}
	};
};