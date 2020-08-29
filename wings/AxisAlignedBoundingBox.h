#pragma once

#include <glm/vec3.hpp>

class AxisAlignedBoundingBox
{
private:
	glm::vec3 mLowerCorner;
	glm::vec3 mUpperCorner;

public:
	AxisAlignedBoundingBox(const glm::vec3& lower, const glm::vec3& higher) : mLowerCorner(lower), mUpperCorner(higher) {}

	const glm::vec3& getLowerBound() const { return mLowerCorner; }
	const glm::vec3& getHigherBound() const { return mUpperCorner; }

	bool intersects(const AxisAlignedBoundingBox& b)
	{
		return (mLowerCorner.x <= b.mUpperCorner.x && mUpperCorner.x >= b.mLowerCorner.x) &&
			(mLowerCorner.y <= b.mUpperCorner.y && mUpperCorner.y >= b.mLowerCorner.y) &&
			(mLowerCorner.z <= b.mUpperCorner.z && mUpperCorner.z >= b.mLowerCorner.z);
	}

	const bool containsPoint(const glm::vec3& pos) const
	{
		return (pos.x <= mUpperCorner.x) // - boundary
			&& (pos.y <= mUpperCorner.y) // - boundary
			&& (pos.z <= mUpperCorner.z) // - boundary
			&& (pos.x >= mLowerCorner.x) // + boundary
			&& (pos.y >= mLowerCorner.y) // + boundary
			&& (pos.z >= mLowerCorner.z); // + boundary
	}
};