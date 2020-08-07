#pragma region Includes

#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <fstream>
#include <sstream>
#include <utility>
#include <mutex>
#include <chrono>
#include <unordered_set>
#include <iterator>
#include <deque>
#include <atomic>
#include <random>

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/geometric.hpp>

#pragma endregion

#pragma region Graphics Engine

struct Vertex
{
	glm::vec3 mPosition;
	glm::vec3 mNormal;
	glm::vec3 mColor;

	Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec3& clr) :
		mPosition(pos), mNormal(norm), mColor(clr)
	{}
};

class Mesh
{
public:
	size_t addVertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec3& clr)
	{
		mVertices.push_back(Vertex(pos, norm, clr));
		return mVertices.size() - 1;
	}

	void addTriangle(size_t idx0, size_t idx1, size_t idx2)
	{
		assert(idx0 < mVertices.size());
		assert(idx1 < mVertices.size());
		assert(idx2 < mVertices.size());

		mIndices.push_back(idx0);
		mIndices.push_back(idx1);
		mIndices.push_back(idx2);
	}

	const size_t getNumIndices() const { return mIndices.size(); }
	const Vertex& getRenderVertex(const size_t& index) const { return mVertices[mIndices[index]]; }

private:
	std::vector<Vertex> mVertices;
	std::vector<size_t> mIndices;
};

class Ray
{
private:
	glm::vec3 mStart;
	glm::vec3 mEnd;
	glm::vec3 mDirAndLength;

public:
	void setStart(const glm::vec3& start) { mStart = start; }
	void setDirectionAndLength(const glm::vec3& dirAndLen) { mDirAndLength = dirAndLen; }
	void setEnd(const glm::vec3& end) { mEnd = end; }

	const glm::vec3& getStart() const { return mStart; }
	const glm::vec3& getDirectionAndLength() const { return mDirAndLength; }
	const glm::vec3& getEnd() const { return mEnd; }
};

class AxisAlignedBoundingBox
{
private:
	glm::vec3 mLower;
	glm::vec3 mHigher;

public:
	AxisAlignedBoundingBox(const glm::vec3& lower, const glm::vec3& higher) : mLower(lower), mHigher(higher) {}

	const glm::vec3& getLowerBound() const { return mLower; }
	const glm::vec3& getHigherBound() const { return mHigher; }

	bool intersects(const AxisAlignedBoundingBox& b)
	{
		return (mLower.x <= b.mHigher.x && mHigher.x >= b.mLower.x) &&
			(mLower.y <= b.mHigher.y && mHigher.y >= b.mLower.y) &&
			(mLower.z <= b.mHigher.z && mHigher.z >= b.mLower.z);
	}
};

#pragma endregion

#pragma region Voxel Engine

template<typename T>
constexpr auto BYTE_TO_FLOAT_COLOR(T b) { return b / 255.0f; }

struct VoxelType
{
public:
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;

	VoxelType() : r(0), g(0), b(0), a(0) {}
	VoxelType(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a) : r(_r), g(_g), b(_b), a(_a) {}

	bool operator > (int i) { return getTotal() > i; }
	bool operator == (int i) { return getTotal() == i; }

	glm::vec3 toVertexColor() { return glm::vec3(BYTE_TO_FLOAT_COLOR(r), BYTE_TO_FLOAT_COLOR(g), BYTE_TO_FLOAT_COLOR(b)); }

private:
	unsigned short getTotal() { return r + g + b + a; }
} EmptyVoxelType;

class VolumeRegion
{
public:
	VolumeRegion(int lowX, int lowY, int lowZ, int highX, int highY, int highZ) :
		mLowerCorner(lowX, lowY, lowZ), mUpperCorner(highX, highY, highZ)
	{
		assert(mUpperCorner.x >= mLowerCorner.x);
		assert(mUpperCorner.y >= mLowerCorner.y);
		assert(mUpperCorner.z >= mLowerCorner.z);

		mSize.x = (mUpperCorner.x - mLowerCorner.x) + 1;
		mSize.y = (mUpperCorner.y - mLowerCorner.y) + 1;
		mSize.z = (mUpperCorner.z - mLowerCorner.z) + 1;
	}

	const glm::ivec3& getLowerCorner() const { return mLowerCorner; }
	const glm::ivec3& getUpperCorner() const { return mUpperCorner; }

	const int getWidth() const { return mSize.x; }
	const int getHeight() const { return mSize.y; }
	const int getDepth() const { return mSize.z; }

	const bool containsPoint(const glm::ivec3& pos) const
	{
		return (pos.x <= mUpperCorner.x) // - boundary
			&& (pos.y <= mUpperCorner.y) // - boundary
			&& (pos.z <= mUpperCorner.z) // - boundary
			&& (pos.x >= mLowerCorner.x) // + boundary
			&& (pos.y >= mLowerCorner.y) // + boundary
			&& (pos.z >= mLowerCorner.z); // + boundary
	}

	void cropTo(const VolumeRegion& other)
	{
		mLowerCorner.x = ((std::max)(mLowerCorner.x, other.mLowerCorner.x));
		mLowerCorner.y = ((std::max)(mLowerCorner.y, other.mLowerCorner.y));
		mLowerCorner.z = ((std::max)(mLowerCorner.z, other.mLowerCorner.z));
		mUpperCorner.x = ((std::min)(mUpperCorner.x, other.mUpperCorner.x));
		mUpperCorner.y = ((std::min)(mUpperCorner.y, other.mUpperCorner.y));
		mUpperCorner.z = ((std::min)(mUpperCorner.z, other.mUpperCorner.z));
	}

private:
	glm::ivec3 mLowerCorner;
	glm::ivec3 mUpperCorner;
	glm::ivec3 mSize;
};

class VoxelVolume
{
public:
	VoxelVolume(int lowX, int lowY, int lowZ, int highX, int highY, int highZ) :
		mRegion(lowX, lowY, lowZ, highX, highY, highZ)
	{
		assert(mRegion.getWidth() > 0);
		assert(mRegion.getHeight() > 0);
		assert(mRegion.getDepth() > 0);

		mData = new VoxelType[mRegion.getWidth() * mRegion.getHeight() * mRegion.getDepth()];
	}

	const VolumeRegion& getEnclosingRegion() const { return mRegion; }

	const VoxelType& getVoxelAt(int x, int y, int z) const
	{
		if (mRegion.containsPoint(glm::ivec3(x, y, z)))
		{
			const glm::ivec3& lower = mRegion.getLowerCorner();
			int localX = x - lower.x;
			int localY = y - lower.y;
			int localZ = z - lower.z;

			return mData
			[
				localX +
				localY * mRegion.getWidth() +
				localZ * mRegion.getWidth() * mRegion.getHeight()
			];
		}
		else { return EmptyVoxelType; }
	}

	const bool setVoxelAt(int x, int y, int z, const VoxelType& val) const
	{
		if (mRegion.containsPoint(glm::ivec3(x, y, z)))
		{
			const glm::ivec3& lower = mRegion.getLowerCorner();
			int localX = x - lower.x;
			int localY = y - lower.y;
			int localZ = z - lower.z;

			mData
			[
				localX +
				localY * mRegion.getWidth() +
				localZ * mRegion.getWidth() * mRegion.getHeight()
			] = val;

			return true;
		}
		else { return false; }
	}

private:
	VolumeRegion mRegion;
	VoxelType* mData;
};

bool isQuadNeeded(VoxelType back, VoxelType front, glm::vec3& materialToUse)
{
	if ((back > 0) && (front == 0))
	{
		materialToUse = back.toVertexColor();
		return true;
	}
	else
	{
		return false;
	}
}

void extractVolumeSurface(VoxelVolume* volume, Mesh* mesh)
{
	const VolumeRegion& region = volume->getEnclosingRegion();

	for (int32_t z = region.getLowerCorner().z; z < region.getUpperCorner().z; z++)
	{
		for (int32_t y = region.getLowerCorner().y; y < region.getUpperCorner().y; y++)
		{
			for (int32_t x = region.getLowerCorner().x; x < region.getUpperCorner().x; x++)
			{
				// these are always positive anyway
				float regX = static_cast<float>(x - region.getLowerCorner().x);
				float regY = static_cast<float>(y - region.getLowerCorner().y);
				float regZ = static_cast<float>(z - region.getLowerCorner().z);

				glm::vec3 material;

				const VoxelType& curVoxel = volume->getVoxelAt(x, y, z);

				if (isQuadNeeded(curVoxel, volume->getVoxelAt(x + 1, y, z), material))
				{
					const glm::vec3 norm = glm::vec3(1.0f, 0.0f, 0.0f);

					uint32_t v0 = mesh->addVertex(glm::vec3(regX + 1.0f, regY       , regZ       ), norm, material);
					uint32_t v1 = mesh->addVertex(glm::vec3(regX + 1.0f, regY       , regZ + 1.0f), norm, material);
					uint32_t v2 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ       ), norm, material);
					uint32_t v3 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ + 1.0f), norm, material);

					mesh->addTriangle(v0, v2, v1);
					mesh->addTriangle(v1, v2, v3);
				}
				if (isQuadNeeded(volume->getVoxelAt(x + 1, y, z), curVoxel, material))
				{
					const glm::vec3 norm = glm::vec3(-1.0f, 0.0f, 0.0f);

					uint32_t v0 = mesh->addVertex(glm::vec3(regX + 1.0f, regY       , regZ       ), norm, material);
					uint32_t v1 = mesh->addVertex(glm::vec3(regX + 1.0f, regY       , regZ + 1.0f), norm, material);
					uint32_t v2 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ       ), norm, material);
					uint32_t v3 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ + 1.0f), norm, material);

					mesh->addTriangle(v0, v1, v2);
					mesh->addTriangle(v1, v3, v2);
				}

				if (isQuadNeeded(curVoxel, volume->getVoxelAt(x, y + 1, z), material))
				{
					const glm::vec3 norm = glm::vec3(0.0f, 1.0f, 0.0f);

					uint32_t v0 = mesh->addVertex(glm::vec3(regX       , regY + 1.0f, regZ       ), norm, material);
					uint32_t v1 = mesh->addVertex(glm::vec3(regX       , regY + 1.0f, regZ + 1.0f), norm, material);
					uint32_t v2 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ       ), norm, material);
					uint32_t v3 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ + 1.0f), norm, material);

					mesh->addTriangle(v0, v1, v2);
					mesh->addTriangle(v1, v3, v2);
				}
				if (isQuadNeeded(volume->getVoxelAt(x, y + 1, z), curVoxel, material))
				{
					const glm::vec3 norm = glm::vec3(0.0f, -1.0f, 0.0f);

					uint32_t v0 = mesh->addVertex(glm::vec3(regX       , regY + 1.0f, regZ       ), norm, material);
					uint32_t v1 = mesh->addVertex(glm::vec3(regX       , regY + 1.0f, regZ + 1.0f), norm, material);
					uint32_t v2 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ       ), norm, material);
					uint32_t v3 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ + 1.0f), norm, material);

					mesh->addTriangle(v0, v2, v1);
					mesh->addTriangle(v1, v2, v3);
				}

				if (isQuadNeeded(curVoxel, volume->getVoxelAt(x, y, z + 1), material))
				{
					const glm::vec3 norm = glm::vec3(0.0f, 0.0f, 1.0f);

					uint32_t v0 = mesh->addVertex(glm::vec3(regX       , regY       , regZ + 1.0f), norm, material);
					uint32_t v1 = mesh->addVertex(glm::vec3(regX       , regY + 1.0f, regZ + 1.0f), norm, material);
					uint32_t v2 = mesh->addVertex(glm::vec3(regX + 1.0f, regY       , regZ + 1.0f), norm, material);
					uint32_t v3 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ + 1.0f), norm, material);

					mesh->addTriangle(v0, v2, v1);
					mesh->addTriangle(v1, v2, v3);
				}
				if (isQuadNeeded(volume->getVoxelAt(x, y, z + 1), curVoxel, material))
				{
					const glm::vec3 norm = glm::vec3(0.0f, 0.0f, -1.0f);

					uint32_t v0 = mesh->addVertex(glm::vec3(regX       , regY       , regZ + 1.0f), norm, material);
					uint32_t v1 = mesh->addVertex(glm::vec3(regX       , regY + 1.0f, regZ + 1.0f), norm, material);
					uint32_t v2 = mesh->addVertex(glm::vec3(regX + 1.0f, regY       , regZ + 1.0f), norm, material);
					uint32_t v3 = mesh->addVertex(glm::vec3(regX + 1.0f, regY + 1.0f, regZ + 1.0f), norm, material);

					mesh->addTriangle(v0, v1, v2);
					mesh->addTriangle(v1, v3, v2);
				}
			}
		}
	}

	printf("Extracted (%d, %d, %d) with %d indices.\n", region.getLowerCorner().x, region.getLowerCorner().y, region.getLowerCorner().z, mesh->getNumIndices());
}

class VolumeSampler
{
public:
	VolumeSampler(VoxelVolume* volume) : mVolume(volume) {}

	const glm::ivec3& getPosition() const { return mPosition; }
	void setPosition(int x, int y, int z)
	{
		mPosition.x = x;
		mPosition.y = y;
		mPosition.z = z;
	}

	void movePositiveX() { mPosition.x++; }
	void moveNegativeX() { mPosition.x--; }
	void movePositiveY() { mPosition.y++; }
	void moveNegativeY() { mPosition.y--; }
	void movePositiveZ() { mPosition.z++; }
	void moveNegativeZ() { mPosition.z--; }

	const VoxelType& getVoxel() const { return mVolume->getVoxelAt(mPosition.x, mPosition.y, mPosition.z); }

private:
	VoxelVolume* mVolume;
	glm::ivec3 mPosition;
};

namespace RaycastResults
{
	/**
	 * The results of a raycast
	 */
	enum RaycastResult
	{
		Completed, ///< If the ray passed through the volume without being interupted
		Interupted ///< If the ray was interupted while travelling
	};
}
typedef RaycastResults::RaycastResult RaycastResult;

// This function is based on Christer Ericson's code and description of the 'Uniform Grid Intersection Test' in
// 'Real Time Collision Detection'. The following information from the errata on the book website is also relevent:
//
//	pages 326-327. In the function VisitCellsOverlapped() the two lines calculating tx and ty are incorrect.
//  The less-than sign in each line should be a greater-than sign. That is, the two lines should read:
//
//	float tx = ((x1 > x2) ? (x1 - minx) : (maxx - x1)) / Abs(x2 - x1);
//	float ty = ((y1 > y2) ? (y1 - miny) : (maxy - y1)) / Abs(y2 - y1);
//
//	Thanks to Jetro Lauha of Fathammer in Helsinki, Finland for reporting this error.
//
//	Jetro also points out that the computations of i, j, iend, and jend are incorrectly rounded if the line
//  coordinates are allowed to go negative. While that was not really the intent of the code -- that is, I
//  assumed grids to be numbered from (0, 0) to (m, n) -- I'm at fault for not making my assumption clear.
//  Where it is important to handle negative line coordinates the computation of these variables should be
//  changed to something like this:
//
//	// Determine start grid cell coordinates (i, j)
//	int i = (int)floorf(x1 / CELL_SIDE);
//	int j = (int)floorf(y1 / CELL_SIDE);
//
//	// Determine end grid cell coordinates (iend, jend)
//	int iend = (int)floorf(x2 / CELL_SIDE);
//	int jend = (int)floorf(y2 / CELL_SIDE);
//
//	page 328. The if-statement that reads "if (ty <= tx && ty <= tz)" has a superfluous condition.
//  It should simply read "if (ty <= tz)".
//
//	This error was reported by Joey Hammer (PixelActive).
RaycastResult raycastWithEndpoints(VoxelVolume* volData, const glm::vec3& v3dStart, const glm::vec3& v3dEnd, std::function<bool(VolumeSampler&)> callback)
{
	typename VolumeSampler sampler(volData);

	//The doRaycast function is assuming that it is iterating over the areas defined between
	//voxels. We actually want to define the areas as being centered on voxels (as this is
	//what the CubicSurfaceExtractor generates). We add 0.5 here to adjust for this.
	float x1 = v3dStart.x + 0.5f;
	float y1 = v3dStart.y + 0.5f;
	float z1 = v3dStart.z + 0.5f;
	float x2 = v3dEnd.x + 0.5f;
	float y2 = v3dEnd.y + 0.5f;
	float z2 = v3dEnd.z + 0.5f;

	int i = (int)floorf(x1);
	int j = (int)floorf(y1);
	int k = (int)floorf(z1);

	int iend = (int)floorf(x2);
	int jend = (int)floorf(y2);
	int kend = (int)floorf(z2);

	int di = ((x1 < x2) ? 1 : ((x1 > x2) ? -1 : 0));
	int dj = ((y1 < y2) ? 1 : ((y1 > y2) ? -1 : 0));
	int dk = ((z1 < z2) ? 1 : ((z1 > z2) ? -1 : 0));

	float deltatx = 1.0f / std::abs(x2 - x1);
	float deltaty = 1.0f / std::abs(y2 - y1);
	float deltatz = 1.0f / std::abs(z2 - z1);

	float minx = floorf(x1), maxx = minx + 1.0f;
	float tx = ((x1 > x2) ? (x1 - minx) : (maxx - x1)) * deltatx;
	float miny = floorf(y1), maxy = miny + 1.0f;
	float ty = ((y1 > y2) ? (y1 - miny) : (maxy - y1)) * deltaty;
	float minz = floorf(z1), maxz = minz + 1.0f;
	float tz = ((z1 > z2) ? (z1 - minz) : (maxz - z1)) * deltatz;

	sampler.setPosition(i, j, k);

	for (;;)
	{
		if (!callback(sampler))
		{
			return RaycastResults::Interupted;
		}

		if (tx <= ty && tx <= tz)
		{
			if (i == iend) break;
			tx += deltatx;
			i += di;

			if (di == 1) sampler.movePositiveX();
			if (di == -1) sampler.moveNegativeX();
		}
		else if (ty <= tz)
		{
			if (j == jend) break;
			ty += deltaty;
			j += dj;

			if (dj == 1) sampler.movePositiveY();
			if (dj == -1) sampler.moveNegativeY();
		}
		else
		{
			if (k == kend) break;
			tz += deltatz;
			k += dk;

			if (dk == 1) sampler.movePositiveZ();
			if (dk == -1) sampler.moveNegativeZ();
		}
	}

	return RaycastResults::Completed;
}

RaycastResult raycastWithDirection(VoxelVolume* volData, const glm::vec3& v3dStart, const glm::vec3& v3dDirectionAndLength, std::function<bool(VolumeSampler&)> callback)
{
	glm::vec3 v3dEnd = v3dStart + v3dDirectionAndLength;
	return raycastWithEndpoints(volData, v3dStart, v3dEnd, callback);
}

#pragma endregion

#pragma region XML Parser

class XMLElement
{
public:
	const std::string& getName() const { return mName; }
	const std::string& getValue() const { return mValue; }
	const bool hasAttribute(const std::string& name) const { return mAttributes.count(name); }
	const std::string& getAttributeValue(const std::string& name) { return mAttributes[name]; }
	XMLElement* getChildByIndex(unsigned int i) { return mChildren[i].get(); }
	XMLElement* getChildByName(const std::string& name)
	{
		for (auto& child : mChildren) { if (child->getName() == name) { return child.get(); } }
	}
	const bool hasChild(const std::string& name) { return getChildByName(name) != 0; }

	void setName(const std::string& name) { mName = name; }
	void addAttribute(const std::string& name, const std::string& value) { mAttributes[name] = value; }
	void addChild(XMLElement* element) { mChildren.push_back(std::unique_ptr<XMLElement>(element)); }

	unsigned int getNumChildren() { return mChildren.size(); }
	unsigned int getNumAttributes() { return mAttributes.size(); }

private:
	std::string mName;
	std::string mValue;
	std::unordered_map<std::string, std::string> mAttributes;
	std::vector<std::unique_ptr<XMLElement>> mChildren;
};

class XMLParser
{
public:
	static std::unique_ptr<XMLElement> load(const std::string& path) { return XMLParser().loadInternal(path); }

private:
	std::ifstream mFile;
	char mCurrentChar;
	std::vector<XMLElement*> mElementStack;

	bool readNextChar() { if (mFile.bad() || mFile.eof()) { return false; } else { mFile.read(&mCurrentChar, 1); return true; } }

	bool isSkippedWhitespaceChar() { return mCurrentChar == '\r' || mCurrentChar == '\n' || mCurrentChar == ' ' || mCurrentChar == '\t'; }

	bool readElementAttributes(XMLElement* elem)
	{
		bool done = false;
		bool readName = false;
		std::stringstream name;
		bool readingAttribs = false;
		bool readingAttribName = false;
		bool readingAttribValue = false;
		std::stringstream attribName;
		std::stringstream attribValue;

		while (!done && readNextChar())
		{
			if (!readName)
			{
				if (mCurrentChar == ' ' || mCurrentChar == '>')
				{
					elem->setName(name.str());
					readName = true;
					if (mCurrentChar == ' ') { readingAttribs = true; readingAttribName = true; }
					else if (mCurrentChar == '>') { mElementStack.push_back(elem); return true; }
				}
				else { name.put(mCurrentChar); }
			}
			else if (readingAttribs)
			{
				if (readingAttribName)
				{
					if (mCurrentChar == '/')
					{
						readNextChar(); // get the > out of the way
						return false;
					}
					
					if (mCurrentChar == '=')
					{
						readingAttribName = false;
						readingAttribValue = true;
						readNextChar();
						if (mCurrentChar != '\"') { printf("WTF INVALID XML!!\n"); }
					}
					else { attribName.put(mCurrentChar); }
				}
				else if (readingAttribValue)
				{
					if (mCurrentChar == '\"')
					{
						readingAttribValue = false;
						elem->addAttribute(attribName.str(), attribValue.str());
						//printf("%s :: %s -> %s\n", elem->getName().c_str(), attribName.str().c_str(), attribValue.str().c_str());
						attribName.str("");
						attribValue.str("");
					}
					else { attribValue.put(mCurrentChar); }
				}
				else
				{
					// only valid chars at this point are ' ', a char for a new attrib, '/', or '>'
					if (mCurrentChar == ' ') { readingAttribName = true; }
					else if (mCurrentChar == '>') { mElementStack.push_back(elem); return true; }
					else if (mCurrentChar == '/' || mCurrentChar == '?') { readNextChar(); return false; }
				}
			}
		}

		return false; // TODO: !!! wat!!
	}

	void readInnerElements(XMLElement* elem)
	{
		while (readNextChar())
		{
			if (isSkippedWhitespaceChar()) { continue; }

			if (mCurrentChar == '<')
			{
				XMLElement* newElem = new XMLElement();
				bool newScope = readElementAttributes(newElem);
				if (newElem->getName() == "?xml")
				{
					delete newElem;
					return;
				}
				if (newElem->getName()[0] == '/')
				{
					mElementStack.pop_back();
					delete newElem;
					return;
				}
				elem->addChild(newElem);
				if (newScope) { readInnerElements(newElem); }
			}
		}
	}

	std::unique_ptr<XMLElement> loadInternal(const std::string& path)
	{
		std::unique_ptr<XMLElement> document(new XMLElement());

		mFile.open(path);

		printf("Loading XML DOM for %s\n", path.c_str());

		while (mFile.good() && !mFile.eof()) { readInnerElements(document.get()); }

		mFile.close();

		printf("Loaded XML document with %d children.\n", document->getChildByIndex(0)->getNumChildren());

		return document;
	}
};

#pragma endregion

#pragma region General Tools

namespace Tools
{
	namespace StringUtil
	{
		const std::string toLowerCase(std::string in)
		{
			std::transform(in.begin(), in.end(), in.begin(), std::tolower);
			return in;
		}

		const bool equalsIgnoreCase(const std::string& s1, const std::string& s2) { return toLowerCase(s1) == toLowerCase(s2); }

		bool endsWith(const std::string& fullString, const std::string& ending)
		{
			if (fullString.length() >= ending.length()) { return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending)); }
			else { return false; }
		}

		std::vector<std::string> explode(std::string const& s, char delim)
		{
			std::vector<std::string> result;
			std::istringstream iss(s);

			for (std::string token; std::getline(iss, token, delim); )
			{
				result.push_back(std::move(token));
			}

			return result;
		}

		const std::string getLeftPaddedStr(const std::string& in, char padchar, int length)
		{
			std::stringstream builder;
			for (int x = in.length(); x < length; x++) { builder.put(padchar); }
			builder << in;
			return builder.str();
		}
	};

	namespace FileUtil
	{
		const bool isDirectory(const std::string& path) { return GetFileAttributes(path.c_str()) & FILE_ATTRIBUTE_DIRECTORY; }

		const bool exists(const std::string& path) { return GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES; }

		const std::string getParentFile(const std::string& path) { return ""; }
	};

	long long currentTimeMillis() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(); }
};

#pragma endregion

#pragma region Maple Data Processing Engine

namespace MapleDataTypes
{
	enum MapleDataType
	{
		NONE, IMG_0x00, SHORT, INT, FLOAT, DOUBLE, STRING, EXTENDED, PROPERTY, CANVAS, VECTOR, CONVEX, SOUND, UOL, UNKNOWN_TYPE, UNKNOWN_EXTENDED_TYPE
	};
};
typedef MapleDataTypes::MapleDataType MapleDataType;

class MapleDataEntity
{
public:
	virtual const std::string& getName() = 0;
	virtual MapleDataEntity* getParent() = 0;
};

class MapleDataEntry : public MapleDataEntity
{
public:
	virtual const std::string& getName() = 0;
	virtual int getSize() = 0;
	virtual int getChecksum() = 0;
	virtual int getOffset() = 0;
};

class MapleDataFileEntry : public MapleDataEntry
{
public:
	virtual void setOffset(int offset) = 0;
};

class MapleDataDirectoryEntry : public MapleDataEntry
{
public:
	virtual std::vector<MapleDataDirectoryEntry*> getSubdirectories() = 0;
	virtual std::vector<MapleDataFileEntry*> getFiles() = 0;
	virtual MapleDataEntry* getEntry(const std::string& name) = 0;
};

class MapleData : public MapleDataEntity
{
public:
	virtual const std::string& getName() = 0;
	virtual MapleDataType getType() = 0;
	virtual std::vector<MapleData*>& getChildren() = 0;
	virtual MapleData* getChildByPath(const std::string& path) = 0;
	virtual void* getData() = 0;

	std::vector<MapleData*>::iterator begin() { return getChildren().begin(); }
	std::vector<MapleData*>::iterator end() { return getChildren().end(); }
};

class MapleDataProvider
{
public:
	virtual MapleData* getData(const std::string& path) = 0;
	virtual MapleDataDirectoryEntry* getRoot() = 0;
};

class WZEntry : public MapleDataEntry
{
private:
	std::string name;
	int size;
	int checksum;
	int offset;
	MapleDataEntity* parent;

public:
	WZEntry(const std::string& name, int size, int checksum, MapleDataEntity* parent)
	{
		this->name = name;
		this->size = size;
		this->checksum = checksum;
		this->parent = parent;
	}

	const std::string& getName() { return name; }
	int getSize() { return size; }
	int getChecksum() { return checksum; }
	int getOffset() { return offset; }
	MapleDataEntity* getParent() { return parent; }
};

class WZDirectoryEntry : public WZEntry, public MapleDataDirectoryEntry
{
private:
	std::vector<MapleDataDirectoryEntry*> subdirs;
	std::vector<MapleDataFileEntry*> files;
	std::unordered_map<std::string, MapleDataEntry*> entries;

public:
	WZDirectoryEntry(const std::string& name, int size, int checksum, MapleDataEntity* parent) : WZEntry(name, size, checksum, parent) {}

	WZDirectoryEntry() : WZEntry("", 0, 0, 0) {}

	void addDirectory(MapleDataDirectoryEntry* dir)
	{
		subdirs.push_back(dir);
		entries[dir->getName()] = dir;
	}

	void addFile(MapleDataFileEntry* fileEntry)
	{
		files.push_back(fileEntry);
		entries[fileEntry->getName()] = fileEntry;
	}

	std::vector<MapleDataDirectoryEntry*> getSubdirectories() { return subdirs; }

	std::vector<MapleDataFileEntry*> getFiles() { return files; }

	MapleDataEntry* getEntry(const std::string& name) { return entries[name]; }
};

// TODO: need this for image reading!?
class WZFile : public MapleDataProvider
{
public:
	WZFile(const std::string& wzfile, bool provideImages) {}

	MapleData* getData(const std::string& path) { return 0; }
	MapleDataDirectoryEntry* getRoot() { return 0; }
};

class XMLDomMapleData : public MapleData
{
private:
	XMLElement* node;
	std::string imageDataDir;
	std::vector<MapleData*> mChildren;
	bool mChildrenLoaded = false;
	XMLDomMapleData* mParent;
	void* mData = 0;
	bool mDataLoaded = false;

	XMLDomMapleData(XMLElement* node, XMLDomMapleData* parent)
	{
		this->node = node;
		this->mParent = parent;
		//printf("Child node created\n");
	}

public:
	XMLDomMapleData(const std::string& filePath, const std::string& imageDataDir) : mParent(0)
	{
		node = XMLParser::load(filePath).release()->getChildByIndex(0);
		this->imageDataDir = imageDataDir;
	}

	// TODO: store the heirarchy properly! this xml traversal is ugly af!
	virtual MapleData* getChildByPath(const std::string& path) // the whole XML reading system seems susceptible to give nulls on strenuous read scenarios
	{
		std::vector<std::string> segments = Tools::StringUtil::explode(path, '/');
		if (segments[0] == "..") { return ((XMLDomMapleData*)getParent())->getChildByPath(path.substr(path.find("/") + 1)); }

		XMLElement* myNode = node;
		for (std::string& s : segments)
		{
			bool foundChild = false;
			for (unsigned int i = 0; i < myNode->getNumChildren(); i++)
			{
				XMLElement* childNode = myNode->getChildByIndex(i);
				if (childNode->getAttributeValue("name") == s)
				{
					myNode = childNode;
					foundChild = true;
					break;
				}
			}
			if (!foundChild) { return 0; }
		}

		XMLDomMapleData* ret = new XMLDomMapleData(myNode, this);
		ret->imageDataDir = Tools::FileUtil::getParentFile(imageDataDir + getName() + "/" + path);
		return ret;
	}

	virtual std::vector<MapleData*>& getChildren()
	{
		if (!mChildrenLoaded)
		{
			for (unsigned int i = 0; i < node->getNumChildren(); i++)
			{
				XMLDomMapleData* child = new XMLDomMapleData(node->getChildByIndex(i), this);
				child->imageDataDir = imageDataDir + getName();
				mChildren.push_back(child);
			}
			mChildrenLoaded = true;
		}
		return mChildren;
	}

	virtual void* getData()
	{
		if (!mDataLoaded)
		{
			if (node->getAttributeValue("value").length() == 0)
			{
				printf("%s has empty value!\n", getName().c_str());
				return 0;
			}

			MapleDataType type = getType();
			switch (type)
			{
			case MapleDataType::DOUBLE: mData = (void*)new double(std::stod(node->getAttributeValue("value"))); break;
			case MapleDataType::FLOAT: mData = (void*)new float(std::stof(node->getAttributeValue("value"))); break;
			case MapleDataType::INT: mData = (void*)new int(std::stoi(node->getAttributeValue("value"))); break;
			case MapleDataType::SHORT: mData = (void*)new short(std::stoi(node->getAttributeValue("value"))); break;
			case MapleDataType::STRING:
			case MapleDataType::UOL: mData = (void*)new std::string(node->getAttributeValue("value")); break;
			case MapleDataType::VECTOR: mData = (void*)new glm::ivec2(std::stoi(node->getAttributeValue("x")), std::stoi(node->getAttributeValue("y"))); break;
			/*
			case MapleDataType::CANVAS:
			{
				int width = attributes.getNamedItem("width").getNodeValue();
				int height = attributes.getNamedItem("height").getNodeValue();
				return new FileStoredPngMapleCanvas(Integer.parseInt(width), Integer.parseInt(height), new File(
					imageDataDir, getName() + ".png"));
			}
			*/
			default:
				return 0;
			}
		}
		return mData;
	}

	virtual MapleDataType getType()
	{
		const std::string& nodeName = node->getName();

		if (nodeName == "imgdir") { return MapleDataType::PROPERTY; }
		else if (nodeName == "canvas") { return MapleDataType::CANVAS; }
		else if (nodeName == "convex") { return MapleDataType::CONVEX; }
		else if (nodeName == "sound") { return MapleDataType::SOUND; }
		else if (nodeName == "uol") { return MapleDataType::UOL; }
		else if (nodeName == "double") { return MapleDataType::DOUBLE; }
		else if (nodeName == "float") { return MapleDataType::FLOAT; }
		else if (nodeName == "int") { return MapleDataType::INT; }
		else if (nodeName == "short") { return MapleDataType::SHORT; }
		else if (nodeName == "string") { return MapleDataType::STRING; }
		else if (nodeName == "vector") { return MapleDataType::VECTOR; }
		else if (nodeName == "null") { return MapleDataType::IMG_0x00; }
		else { return MapleDataType::IMG_0x00; } // TODO: its not really that but...
	}

	virtual MapleDataEntity* getParent() { return mParent; }

	virtual const std::string& getName() { return node->getAttributeValue("name"); }
};

class XMLWZFile : public MapleDataProvider
{
public:
	XMLWZFile(const std::string& fileIn)
	{
		root = fileIn;
		//rootForNavigation = new WZDirectoryEntry(fileIn.getName(), 0, 0, null);
		//fillMapleDataEntitys(root, rootForNavigation);
	}

	virtual MapleData* getData(const std::string& path)
	{
		std::string dataFile = root + "/" + path + ".xml";
		std::string imageDataDir = root + path;
		if (!Tools::FileUtil::exists(dataFile)) { printf("XML WZ file doesnt exist!: %s\n", dataFile.c_str()); return 0; }
		return new XMLDomMapleData(dataFile, Tools::FileUtil::getParentFile(imageDataDir));
	}

	virtual MapleDataDirectoryEntry* getRoot() { return rootForNavigation; }

private:
	std::string root;
	WZDirectoryEntry* rootForNavigation;

	/*
	void fillMapleDataEntitys(const std::string& lroot, WZDirectoryEntry* wzdir)
	{
		for (File file : lroot.listFiles()) {
			String fileName = file.getName();
			if (file.isDirectory() && !fileName.endsWith(".img")) {
				WZDirectoryEntry newDir = new WZDirectoryEntry(fileName, 0, 0, wzdir);
				wzdir.addDirectory(newDir);
				fillMapleDataEntitys(file, newDir);
			}
			else if (fileName.endsWith(".xml")) {
				wzdir.addFile(new WZFileEntry(fileName.substring(0, fileName.length() - 4), 0, 0, wzdir));
			}
		}
	}
	*/
};

namespace MapleDataProviderFactory
{
	MapleDataProvider* getWZ(const std::string& in, bool provideImages)
	{
		if (Tools::StringUtil::endsWith(Tools::StringUtil::toLowerCase(in), "wz") && !Tools::FileUtil::isDirectory(in)) { printf("wtf\n"); return new WZFile(in, provideImages); }
		else { printf("Loading %s\n", in.c_str()); return new XMLWZFile(in); }
	}

	//TODO: remove need for this??
	const std::string fileInWZPath(const std::string& filename) { return "wz/" + filename; }

	MapleDataProvider* getDataProvider(const std::string& in) { return getWZ(fileInWZPath(in), false); }
	MapleDataProvider* getImageProvidingDataProvider(const std::string& in) { return getWZ(fileInWZPath(in), true); }
};

namespace MapleDataTool
{
	std::string getString(MapleData* data, const std::string& def)
	{
		if (data == 0 || data->getData() == 0) { return def; }
		else { return *(std::string*)data->getData(); }
	}

	std::string getString(MapleData* data, const std::string& path, const std::string& def)
	{
		if (data == 0) { return def; }
		else { return getString(data->getChildByPath(path), def); }
	}

	float getFloat(MapleData* data, const std::string& path, float def)
	{
		if (data == 0) { return def; }
		else
		{
			MapleData* target = data->getChildByPath(path);
			if (target == 0) { return def; }
			else if (target->getData() == 0) { return def; }
			else { return *(float*)target->getData(); }
		}
	}

	int getInt(MapleData* data, const std::string& path, int def)
	{
		if (data == 0) { return def; }
		else
		{
			MapleData* target = data->getChildByPath(path);
			if (target == 0) { return def; }
			else if (target->getData() == 0) { return def; }
			else if (target->getType() == MapleDataType::STRING) { return std::stoi(getString(target, std::to_string(def))); }
			else if (target->getType() == MapleDataType::SHORT) { printf("USING getInt() ON SHORT VALUE \"%s\"!!\n", path.c_str()); return 0; }
			else { return *(int*)target->getData(); } // TODO: might be a short, we still want int in that case?? wtf?
		}
	}
};

#pragma endregion

#pragma region Maple Map Processing Engine

double distanceSq(const glm::ivec2& v, const glm::ivec2& v2) { return (v2.x - v.x) + (v2.y - v.y); }

class MapleCharacter
{
public:
	void addVisibleMapObject(class MapleMapObject* obj) {}

	const glm::ivec2 getPosition() { return glm::ivec2(); }
};
class MapleClient
{
public:
	MapleCharacter* getPlayer() { return 0; }
};

class MaplePortal
{
public:
	static const int SPAWNPOINT_PORTAL = 0;
	static const int TELEPORT_PORTAL = 1;
	static const int MAP_PORTAL = 2;
	static const int DOOR_PORTAL = 6;
	static const bool OPEN = true;
	static const bool CLOSED = false;
	virtual int getType() = 0;
	virtual int getId() = 0;
	virtual const glm::ivec2& getPosition() = 0;
	virtual const std::string& getName() = 0;
	virtual const std::string& getTarget() = 0;
	virtual const std::string& getScriptName() = 0;
	virtual void setScriptName(const std::string& newName) = 0;
	virtual void setPortalStatus(bool newStatus) = 0;
	virtual bool getPortalStatus() = 0;
	virtual int getTargetMapId() = 0;
	virtual void enterPortal(MapleClient* c) = 0;
	virtual void setPortalState(bool state) = 0;
	virtual bool getPortalState() = 0;
};

class PortalScriptManager
{
private:
	static PortalScriptManager* mInstance;

public:
	static PortalScriptManager& getInstance() { return *mInstance; }

	bool executePortalScript(MaplePortal* portal, MapleClient* c) { return false; }
};

PortalScriptManager* PortalScriptManager::mInstance = new PortalScriptManager();

class MapleGenericPortal : public MaplePortal
{
private:
	std::string name;
	std::string target;
	glm::ivec2 position;
	int targetmap;
	int type;
	bool status = true;
	int id;
	std::string scriptName;
	bool portalState;
	std::mutex scriptLock;

public:
	MapleGenericPortal(int type)
	{
		this->type = type;
	}

	virtual int getId() { return id; }
	void setId(int id) { this->id = id; }
	virtual const std::string& getName() { return name; }
	virtual const glm::ivec2& getPosition() { return position; }
	virtual const std::string& getTarget() { return target; }
	virtual void setPortalStatus(bool newStatus) { this->status = newStatus; }
	virtual bool getPortalStatus() { return status; }
	virtual int getTargetMapId() { return targetmap; }
	virtual int getType() { return type; }
	virtual const std::string& getScriptName() { return scriptName; }
	void setName(const std::string& name) { this->name = name; }
	void setPosition(const glm::ivec2& position) { this->position = position; }
	void setTarget(const std::string& target) { this->target = target; }
	void setTargetMapId(int targetmapid) { this->targetmap = targetmapid; }

	virtual void setScriptName(const std::string& scriptName) { this->scriptName = scriptName; }

	virtual void enterPortal(MapleClient* c)
	{
		bool changed = false;
		if (getScriptName() != "")
		{
			scriptLock.lock();
			changed = PortalScriptManager::getInstance().executePortalScript(this, c);
			scriptLock.unlock();
		}
		else if (getTargetMapId() != 999999999)
		{
			/* FIX THIS
			MapleCharacter* chr = c->getPlayer();
			if (!(chr.getChalkboard() != null && GameConstants.isFreeMarketRoom(getTargetMapId())))
			{
				MapleMap to = chr.getEventInstance() == null ? c.getChannelServer().getMapFactory().getMap(getTargetMapId()) : chr.getEventInstance().getMapInstance(getTargetMapId());
				MaplePortal pto = to.getPortal(getTarget());
				if (pto == null) {// fallback for missing portals - no real life case anymore - interesting for not implemented areas
					pto = to.getPortal(0);
				}
				chr.changeMap(to, pto); //late resolving makes this harder but prevents us from loading the whole world at once
				changed = true;
			}
			else { chr.dropMessage(5, "You cannot enter this map with the chalkboard opened."); }
			*/
		}
		//if (!changed) { c.announce(MaplePacketCreator.enableActions()); }
	}

	virtual void setPortalState(bool state) { this->portalState = state; }
	virtual bool getPortalState() { return portalState; }
};

class MapleMapPortal : public MapleGenericPortal
{
public:
	MapleMapPortal() : MapleGenericPortal(MaplePortal::MAP_PORTAL) {}
};

class MaplePortalFactory
{
public:
	MaplePortalFactory()
	{
		nextDoorPortal = 0x80;
	}

	MaplePortal* makePortal(int type, MapleData* portal)
	{
		MapleGenericPortal* ret = 0;
		if (type == MaplePortal::MAP_PORTAL) { ret = new MapleMapPortal(); }
		else { ret = new MapleGenericPortal(type); }
		loadPortal(ret, portal);
		return ret;
	}

private:
	int nextDoorPortal;

	void loadPortal(MapleGenericPortal* myPortal, MapleData* portal)
	{
		myPortal->setName(MapleDataTool::getString(portal, "pn", ""));
		myPortal->setTarget(MapleDataTool::getString(portal, "tn", ""));
		myPortal->setTargetMapId(MapleDataTool::getInt(portal, "tm", 0));
		int x = MapleDataTool::getInt(portal, "x", 0);
		int y = -MapleDataTool::getInt(portal, "y", 0);
		myPortal->setPosition(glm::ivec2(x, y));
		myPortal->setScriptName(MapleDataTool::getString(portal, "script", ""));
		if (myPortal->getType() == MaplePortal::DOOR_PORTAL)
		{
			myPortal->setId(nextDoorPortal);
			nextDoorPortal++;
		}
		else { myPortal->setId(std::stoi(portal->getName())); }
	}
};

class MapleFoothold
{
public:
	MapleFoothold(const glm::ivec2& p1, const glm::ivec2& p2, int id, int prev, int next)
	{
		this->p1 = p1;
		this->p2 = p2;
		this->id = id;
		this->prev = prev;
		this->next = next;
	}

	bool isWall() { return p1.x == p2.x; }

	int getX1() { return p1.x; }

	int getX2() { return p2.x; }

	int getY1() { return p1.y; }

	int getY2() { return p2.y; }

	// XXX may need more precision
	int calculateFooting(int x)
	{
		if (p1.y == p2.y) { return p2.y; } // y at both ends is the same
		int slope = (p1.y - p2.y) / (p1.x - p2.x);
		int intercept = p1.y - (slope * p1.x);
		return (slope * x) + intercept;
	}

	int compareTo(MapleFoothold o)
	{
		if (p2.y < o.getY1()) { return -1; }
		else if (p1.y > o.getY2()) { return 1; }
		else { return 0; }
	}

	int getId() { return id; }
	int getNext() { return next; }
	int getPrev() { return prev; }

private:
	glm::ivec2 p1;
	glm::ivec2 p2;
	int id;
	int next, prev;
};

class MapleFootholdTree
{
public:
	MapleFootholdTree(const glm::ivec2& p1, const glm::ivec2& p2) : nw(0), ne(0), sw(0), se(0)
	{
		this->p1 = p1;
		this->p2 = p2;
		center = glm::ivec2((p2.x - p1.x) / 2, (p2.y - p1.y) / 2);
	}

	MapleFootholdTree(const glm::ivec2& p1, const glm::ivec2& p2, int depth) : nw(0), ne(0), sw(0), se(0)
	{
		this->p1 = p1;
		this->p2 = p2;
		this->depth = depth;
		center = glm::ivec2((p2.x - p1.x) / 2, (p2.y - p1.y) / 2);
	}

	void insert(MapleFoothold* f)
	{
		if (depth == 0)
		{
			if (f->getX1() > maxDropX) { maxDropX = f->getX1(); }
			if (f->getX1() < minDropX) { minDropX = f->getX1(); }
			if (f->getX2() > maxDropX) { maxDropX = f->getX2(); }
			if (f->getX2() < minDropX) { minDropX = f->getX2(); }
		}
		if (depth == maxDepth ||
			(f->getX1() >= p1.x && f->getX2() <= p2.x &&
				f->getY1() >= p1.y && f->getY2() <= p2.y))
		{
			footholds.push_back(f);
		}
		else
		{
			if (nw == 0)
			{
				nw = new MapleFootholdTree(p1, center, depth + 1);
				ne = new MapleFootholdTree(glm::ivec2(center.x, p1.y), glm::ivec2(p2.x, center.y), depth + 1);
				sw = new MapleFootholdTree(glm::ivec2(p1.x, center.y), glm::ivec2(center.x, p2.y), depth + 1);
				se = new MapleFootholdTree(center, p2, depth + 1);
			}
			if (f->getX2() <= center.x && f->getY2() <= center.y) { nw->insert(f); }
			else if (f->getX1() > center.x && f->getY2() <= center.y) { ne->insert(f); }
			else if (f->getX2() <= center.x && f->getY1() > center.y) { sw->insert(f); }
			else { se->insert(f); }
		}
	}
	/*
	private List<MapleFoothold> getRelevants(Point p) {
		return getRelevants(p, new LinkedList<MapleFoothold>());
	}

	private List<MapleFoothold> getRelevants(Point p, List<MapleFoothold> list) {
		list.addAll(footholds);
		if (nw != null) {
			if (p.x <= center.x && p.y <= center.y) {
				nw.getRelevants(p, list);
			}
			else if (p.x > center.x && p.y <= center.y) {
				ne.getRelevants(p, list);
			}
			else if (p.x <= center.x && p.y > center.y) {
				sw.getRelevants(p, list);
			}
			else {
				se.getRelevants(p, list);
			}
		}
		return list;
	}

	private MapleFoothold findWallR(Point p1, Point p2) {
		MapleFoothold ret;
		for (MapleFoothold f : footholds) {
			if (f.isWall() && f.getX1() >= p1.x && f.getX1() <= p2.x &&
				f.getY1() >= p1.y && f.getY2() <= p1.y) {
				return f;
			}
		}
		if (nw != null) {
			if (p1.x <= center.x && p1.y <= center.y) {
				ret = nw.findWallR(p1, p2);
				if (ret != null) {
					return ret;
				}
			}
			if ((p1.x > center.x || p2.x > center.x) && p1.y <= center.y) {
				ret = ne.findWallR(p1, p2);
				if (ret != null) {
					return ret;
				}
			}
			if (p1.x <= center.x && p1.y > center.y) {
				ret = sw.findWallR(p1, p2);
				if (ret != null) {
					return ret;
				}
			}
			if ((p1.x > center.x || p2.x > center.x) && p1.y > center.y) {
				ret = se.findWallR(p1, p2);
				if (ret != null) {
					return ret;
				}
			}
		}
		return null;
	}

	MapleFoothold findWall(Point p1, Point p2) {
		if (p1.y != p2.y) {
			throw new IllegalArgumentException();
		}
		return findWallR(p1, p2);
	}

	MapleFoothold findBelow(Point p) {
		List<MapleFoothold> relevants = getRelevants(p);
		List<MapleFoothold> xMatches = new LinkedList<MapleFoothold>();
		for (MapleFoothold fh : relevants) {
			if (fh.getX1() <= p.x && fh.getX2() >= p.x) {
				xMatches.add(fh);
			}
		}
		Collections.sort(xMatches);
		for (MapleFoothold fh : xMatches) {
			if (!fh.isWall()) {
				if (fh.getY1() != fh.getY2()) {
					int calcY;
					double s1 = Math.abs(fh.getY2() - fh.getY1());
					double s2 = Math.abs(fh.getX2() - fh.getX1());
					double s4 = Math.abs(p.x - fh.getX1());
					double alpha = Math.atan(s2 / s1);
					double beta = Math.atan(s1 / s2);
					double s5 = Math.cos(alpha) * (s4 / Math.cos(beta));
					if (fh.getY2() < fh.getY1()) {
						calcY = fh.getY1() - (int)s5;
					}
					else {
						calcY = fh.getY1() + (int)s5;
					}
					if (calcY >= p.y) {
						return fh;
					}
				}
				else {
					if (fh.getY1() >= p.y) {
						return fh;
					}
				}
			}
		}
		return null;
	}
	*/
	int getX1() { return p1.x; }

	int getX2() { return p2.x; }

	int getY1() { return p1.y; }

	int getY2() { return p2.y; }

	int getMaxDropX() { return maxDropX; }

	int getMinDropX() { return minDropX; }

	std::vector<MapleFoothold*>& getFootholds() { return footholds; }

private:
	MapleFootholdTree* nw;
	MapleFootholdTree* ne;
	MapleFootholdTree* sw;
	MapleFootholdTree* se;
	std::vector<MapleFoothold*> footholds;
	glm::ivec2 p1;
	glm::ivec2 p2;
	glm::ivec2 center;
	int depth = 0;
	const int maxDepth = 8;
	int maxDropX;
	int minDropX;
};

class MapleLadder
{
public:
	MapleLadder(int l, int x, int y1, int y2)
	{
		this->l = l;
		this->x = x;
		this->y1 = y1;
		this->y2 = y2;
	}

	const int& getL() const { return l; }
	const int& getX() const { return x; }
	const int& getY1() const { return y1; }
	const int& getY2() const { return y2; }

private:
	int l;
	int x;
	int y1;
	int y2;
};

#pragma region Life Processing Engine

namespace MapleMapObjectTypes
{
	enum MapleMapObjectType
	{
		NPC, MONSTER, ITEM, PLAYER, DOOR, SUMMON, SHOP, MINI_GAME, MIST, REACTOR, HIRED_MERCHANT, PLAYER_NPC, DRAGON, KITE
	};
};
typedef MapleMapObjectTypes::MapleMapObjectType MapleMapObjectType;

class MapleMapObject
{
public:
	virtual int getObjectId() = 0;
	virtual void setObjectId(int id) = 0;
	virtual MapleMapObjectType getType() = 0;
	virtual const glm::ivec2& getPosition() = 0;
	virtual void setPosition(const glm::ivec2& position) = 0;
	virtual void sendSpawnData(MapleClient* client) = 0;
	virtual void sendDestroyData(MapleClient* client) = 0;
	virtual void nullifyPosition() = 0;
};

class AbstractMapleMapObject : public MapleMapObject
{
private:
	glm::ivec2 position;
	int objectId;

public:
	virtual const glm::ivec2& getPosition() { return position; }
	virtual void setPosition(const glm::ivec2& position) { this->position = position; }
	virtual int getObjectId() { return objectId; }
	virtual void setObjectId(int id) { this->objectId = id; }
	virtual void nullifyPosition() { this->position = glm::ivec2(); }
};

// TODO: this is actually a MapleMapObject! never use it directly!
class AnimatedMapleMapObject
{
public:
	virtual int getStance() = 0;
	virtual void setStance(int stance) = 0;
	virtual bool isFacingLeft() = 0;
};

class AbstractAnimatedMapleMapObject : public AbstractMapleMapObject, public AnimatedMapleMapObject
{
private:
	int stance;

public:
	virtual int getStance() { return stance; }
	virtual void setStance(int stance) { this->stance = stance; }
	virtual bool isFacingLeft() { return std::abs(stance) % 2 == 1; }
};

class AbstractLoadedMapleLife : public AbstractAnimatedMapleMapObject
{
private:
	int id;
	int f;
	bool hide;
	int fh;
	int start_fh;
	int cy;
	int rx0;
	int rx1;

public:
	AbstractLoadedMapleLife(int id) : id(id) {}

	AbstractLoadedMapleLife(AbstractLoadedMapleLife* life)
	{
		this->id = life->id;
		this->f = life->f;
		this->hide = life->hide;
		this->fh = life->fh;
		this->start_fh = life->fh;
		this->cy = life->cy;
		this->rx0 = life->rx0;
		this->rx1 = life->rx1;
	}

	int getF() { return f; }
	void setF(int f) { this->f = f; }
	bool isHidden() { return hide; }
	void setHide(bool hide) { this->hide = hide; }
	int getFh() { return fh; }
	void setFh(int fh) { this->fh = fh; }
	int getStartFh() { return start_fh; }
	int getCy() { return cy; }
	void setCy(int cy) { this->cy = cy; }
	int getRx0() { return rx0; }
	void setRx0(int rx0) { this->rx0 = rx0; }
	int getRx1() { return rx1; }
	void setRx1(int rx1) { this->rx1 = rx1; }
	int getId() { return id; }
};

class MonsterListener
{
public:
	virtual void monsterKilled(int aniTime) = 0;
	virtual void monsterDamaged(MapleCharacter* from, int trueDmg) = 0;
	virtual void monsterHealed(int trueHeal) = 0;
};

class MapleMonster : public AbstractLoadedMapleLife
{
public:
	MapleMonster(int id) : AbstractLoadedMapleLife(id) {}

	virtual MapleMapObjectType getType() { return MapleMapObjectType::MONSTER; }

	void setTeam(int team) {}
	void addListener(MonsterListener* listener) {}
	bool isMobile() { return true; }

	virtual void sendSpawnData(MapleClient* client) {}
	virtual void sendDestroyData(MapleClient* client) {}
};

class MapleMonsterStats;

class MapleNPC : public AbstractLoadedMapleLife
{
private:
	std::string mName;

public:
	MapleNPC(int id, const std::string& name) : AbstractLoadedMapleLife(id)
	{
		mName = name;
	}

	//bool hasShop() { return MapleShopFactory.getInstance().getShopForNPC(getId()) != null; }

	//void sendShop(MapleClient c) { MapleShopFactory.getInstance().getShopForNPC(getId()).sendShop(c); }

	virtual void sendSpawnData(MapleClient* client)
	{
		//client.announce(MaplePacketCreator.spawnNPC(this));
		//client.announce(MaplePacketCreator.spawnNPCRequestController(this, true));
	}

	virtual void sendDestroyData(MapleClient* client)
	{
		//client.announce(MaplePacketCreator.removeNPCController(getObjectId()));
		//client.announce(MaplePacketCreator.removeNPC(getObjectId()));
	}

	virtual MapleMapObjectType getType() { return MapleMapObjectType::NPC; }

	const std::string& getName() { return mName; }
};

class MapleLifeFactory
{
private:
	static MapleLifeFactory* mInstance;

	MapleDataProvider* data;
	MapleDataProvider* stringDataWZ;
	MapleData* mobStringData;
	MapleData* npcStringData;
	std::unordered_map<int, MapleMonsterStats*> monsterStats;
	//std::unordered_set<int> hpbarBosses;

	MapleLifeFactory()
	{
		data = MapleDataProviderFactory::getDataProvider("Mob.wz");
		stringDataWZ = MapleDataProviderFactory::getDataProvider("String.wz");
		mobStringData = stringDataWZ->getData("Mob.img");
		npcStringData = stringDataWZ->getData("Npc.img");
		//hpbarBosses = getHpBarBosses();
	}

	/*
	static std::unordered_set<int> getHpBarBosses() {
		Set<Integer> ret = new HashSet<>();

		MapleDataProvider uiDataWZ = MapleDataProviderFactory.getDataProvider(new File(System.getProperty("wzpath") + "/UI.wz"));
		for (MapleData bossData : uiDataWZ.getData("UIWindow.img").getChildByPath("MobGage/Mob").getChildren()) {
			ret.add(Integer.valueOf(bossData.getName()));
		}

		return ret;
	}

	static class MobAttackInfoHolder {
		protected int attackPos;
		protected int mpCon;
		protected int coolTime;
		protected int animationTime;

		protected MobAttackInfoHolder(int attackPos, int mpCon, int coolTime, int animationTime) {
			this.attackPos = attackPos;
			this.mpCon = mpCon;
			this.coolTime = coolTime;
			this.animationTime = animationTime;
		}
	}

	static void setMonsterAttackInfo(int mid, List<MobAttackInfoHolder> attackInfos) {
		if (!attackInfos.isEmpty()) {
			MapleMonsterInformationProvider mi = MapleMonsterInformationProvider.getInstance();

			for (MobAttackInfoHolder attackInfo : attackInfos) {
				mi.setMobAttackInfo(mid, attackInfo.attackPos, attackInfo.mpCon, attackInfo.coolTime);
				mi.setMobAttackAnimationTime(mid, attackInfo.attackPos, attackInfo.animationTime);
			}
		}
	}

	static Pair<MapleMonsterStats, List<MobAttackInfoHolder>> getMonsterStats(int mid) {
		MapleData monsterData = data.getData(StringUtil.getLeftPaddedStr(Integer.toString(mid) + ".img", '0', 11));
		if (monsterData == null) {
			return null;
		}
		MapleData monsterInfoData = monsterData.getChildByPath("info");

		List<MobAttackInfoHolder> attackInfos = new LinkedList<>();
		MapleMonsterStats stats = new MapleMonsterStats();

		int linkMid = MapleDataTool.getIntConvert("link", monsterInfoData, 0);
		if (linkMid != 0) {
			Pair<MapleMonsterStats, List<MobAttackInfoHolder>> linkStats = getMonsterStats(linkMid);
			if (linkStats == null) {
				return null;
			}

			// thanks resinate for noticing non-propagable infos such as revives getting retrieved
			attackInfos.addAll(linkStats.getRight());
		}

		stats.setHp(MapleDataTool.getIntConvert("maxHP", monsterInfoData));
		stats.setFriendly(MapleDataTool.getIntConvert("damagedByMob", monsterInfoData, stats.isFriendly() ? 1 : 0) == 1);
		stats.setPADamage(MapleDataTool.getIntConvert("PADamage", monsterInfoData));
		stats.setPDDamage(MapleDataTool.getIntConvert("PDDamage", monsterInfoData));
		stats.setMADamage(MapleDataTool.getIntConvert("MADamage", monsterInfoData));
		stats.setMDDamage(MapleDataTool.getIntConvert("MDDamage", monsterInfoData));
		stats.setMp(MapleDataTool.getIntConvert("maxMP", monsterInfoData, stats.getMp()));
		stats.setExp(MapleDataTool.getIntConvert("exp", monsterInfoData, stats.getExp()));
		stats.setLevel(MapleDataTool.getIntConvert("level", monsterInfoData));
		stats.setRemoveAfter(MapleDataTool.getIntConvert("removeAfter", monsterInfoData, stats.removeAfter()));
		stats.setBoss(MapleDataTool.getIntConvert("boss", monsterInfoData, stats.isBoss() ? 1 : 0) > 0);
		stats.setExplosiveReward(MapleDataTool.getIntConvert("explosiveReward", monsterInfoData, stats.isExplosiveReward() ? 1 : 0) > 0);
		stats.setFfaLoot(MapleDataTool.getIntConvert("publicReward", monsterInfoData, stats.isFfaLoot() ? 1 : 0) > 0);
		stats.setUndead(MapleDataTool.getIntConvert("undead", monsterInfoData, stats.isUndead() ? 1 : 0) > 0);
		stats.setName(MapleDataTool.getString(mid + "/name", mobStringData, "MISSINGNO"));
		stats.setBuffToGive(MapleDataTool.getIntConvert("buff", monsterInfoData, stats.getBuffToGive()));
		stats.setCP(MapleDataTool.getIntConvert("getCP", monsterInfoData, stats.getCP()));
		stats.setRemoveOnMiss(MapleDataTool.getIntConvert("removeOnMiss", monsterInfoData, stats.removeOnMiss() ? 1 : 0) > 0);

		MapleData special = monsterInfoData.getChildByPath("coolDamage");
		if (special != null) {
			int coolDmg = MapleDataTool.getIntConvert("coolDamage", monsterInfoData);
			int coolProb = MapleDataTool.getIntConvert("coolDamageProb", monsterInfoData, 0);
			stats.setCool(new Pair<>(coolDmg, coolProb));
		}
		special = monsterInfoData.getChildByPath("loseItem");
		if (special != null) {
			for (MapleData liData : special.getChildren()) {
				stats.addLoseItem(new loseItem(MapleDataTool.getInt(liData.getChildByPath("id")), (byte)MapleDataTool.getInt(liData.getChildByPath("prop")), (byte)MapleDataTool.getInt(liData.getChildByPath("x"))));
			}
		}
		special = monsterInfoData.getChildByPath("selfDestruction");
		if (special != null) {
			stats.setSelfDestruction(new selfDestruction((byte)MapleDataTool.getInt(special.getChildByPath("action")), MapleDataTool.getIntConvert("removeAfter", special, -1), MapleDataTool.getIntConvert("hp", special, -1)));
		}
		MapleData firstAttackData = monsterInfoData.getChildByPath("firstAttack");
		int firstAttack = 0;
		if (firstAttackData != null) {
			if (firstAttackData.getType() == MapleDataType.FLOAT) {
				firstAttack = Math.round(MapleDataTool.getFloat(firstAttackData));
			}
			else {
				firstAttack = MapleDataTool.getInt(firstAttackData);
			}
		}
		stats.setFirstAttack(firstAttack > 0);
		stats.setDropPeriod(MapleDataTool.getIntConvert("dropItemPeriod", monsterInfoData, stats.getDropPeriod() / 10000) * 10000);

		// thanks yuxaij, Riizade, Z1peR, Anesthetic for noticing some bosses crashing players due to missing requirements
		boolean hpbarBoss = stats.isBoss() && hpbarBosses.contains(mid);
		stats.setTagColor(hpbarBoss ? MapleDataTool.getIntConvert("hpTagColor", monsterInfoData, 0) : 0);
		stats.setTagBgColor(hpbarBoss ? MapleDataTool.getIntConvert("hpTagBgcolor", monsterInfoData, 0) : 0);

		for (MapleData idata : monsterData) {
			if (!idata.getName().equals("info")) {
				int delay = 0;
				for (MapleData pic : idata.getChildren()) {
					delay += MapleDataTool.getIntConvert("delay", pic, 0);
				}
				stats.setAnimationTime(idata.getName(), delay);
			}
		}
		MapleData reviveInfo = monsterInfoData.getChildByPath("revive");
		if (reviveInfo != null) {
			List<Integer> revives = new LinkedList<>();
			for (MapleData data_ : reviveInfo) {
				revives.add(MapleDataTool.getInt(data_));
			}
			stats.setRevives(revives);
		}
		decodeElementalString(stats, MapleDataTool.getString("elemAttr", monsterInfoData, ""));

		MapleMonsterInformationProvider mi = MapleMonsterInformationProvider.getInstance();
		MapleData monsterSkillInfoData = monsterInfoData.getChildByPath("skill");
		if (monsterSkillInfoData != null) {
			int i = 0;
			List<Pair<Integer, Integer>> skills = new ArrayList<>();
			while (monsterSkillInfoData.getChildByPath(Integer.toString(i)) != null) {
				int skillId = MapleDataTool.getInt(i + "/skill", monsterSkillInfoData, 0);
				int skillLv = MapleDataTool.getInt(i + "/level", monsterSkillInfoData, 0);
				skills.add(new Pair<>(skillId, skillLv));

				MapleData monsterSkillData = monsterData.getChildByPath("skill" + (i + 1));
				if (monsterSkillData != null) {
					int animationTime = 0;
					for (MapleData effectEntry : monsterSkillData.getChildren()) {
						animationTime += MapleDataTool.getIntConvert("delay", effectEntry, 0);
					}

					MobSkill skill = MobSkillFactory.getMobSkill(skillId, skillLv);
					mi.setMobSkillAnimationTime(skill, animationTime);
				}

				i++;
			}
			stats.setSkills(skills);
		}

		int i = 0;
		MapleData monsterAttackData;
		while ((monsterAttackData = monsterData.getChildByPath("attack" + (i + 1))) != null) {
			int animationTime = 0;
			for (MapleData effectEntry : monsterAttackData.getChildren()) {
				animationTime += MapleDataTool.getIntConvert("delay", effectEntry, 0);
			}

			int mpCon = MapleDataTool.getIntConvert("info/conMP", monsterAttackData, 0);
			int coolTime = MapleDataTool.getIntConvert("info/attackAfter", monsterAttackData, 0);
			attackInfos.add(new MobAttackInfoHolder(i, mpCon, coolTime, animationTime));
			i++;
		}

		MapleData banishData = monsterInfoData.getChildByPath("ban");
		if (banishData != null) {
			stats.setBanishInfo(new BanishInfo(MapleDataTool.getString("banMsg", banishData), MapleDataTool.getInt("banMap/0/field", banishData, -1), MapleDataTool.getString("banMap/0/portal", banishData, "sp")));
		}

		int noFlip = MapleDataTool.getInt("noFlip", monsterInfoData, 0);
		if (noFlip > 0) {
			Point origin = MapleDataTool.getPoint("stand/0/origin", monsterData, null);
			if (origin != null) {
				stats.setFixedStance(origin.getX() < 1 ? 5 : 4);    // fixed left/right
			}
		}

		return new Pair<>(stats, attackInfos);
	}

	static void decodeElementalString(MapleMonsterStats* stats, const std::string& elemAttr) {
		for (int i = 0; i < elemAttr.length(); i += 2) {
			stats.setEffectiveness(Element.getFromChar(elemAttr.charAt(i)), ElementalEffectiveness.getByNumber(Integer.valueOf(String.valueOf(elemAttr.charAt(i + 1)))));
		}
	}

	public static int getMonsterLevel(int mid) {
		try {
			MapleMonsterStats stats = monsterStats.get(Integer.valueOf(mid));
			if (stats == null) {
				MapleData monsterData = data.getData(StringUtil.getLeftPaddedStr(Integer.toString(mid) + ".img", '0', 11));
				if (monsterData == null) {
					return -1;
				}
				MapleData monsterInfoData = monsterData.getChildByPath("info");
				return MapleDataTool.getIntConvert("level", monsterInfoData);
			}
			else {
				return stats.getLevel();
			}
		}
		catch (NullPointerException npe) {
			System.out.println("[SEVERE] MOB " + mid + " failed to load. Issue: " + npe.getMessage() + "\n\n");
			npe.printStackTrace();
		}

		return -1;
	}

	public static String getNPCDefaultTalk(int nid) {
		return MapleDataTool.getString(nid + "/d0", npcStringData, "(...)");
	}

	public static class BanishInfo {

		private int map;
		private String portal, msg;

		public BanishInfo(String msg, int map, String portal) {
			this.msg = msg;
			this.map = map;
			this.portal = portal;
		}

		public int getMap() {
			return map;
		}

		public String getPortal() {
			return portal;
		}

		public String getMsg() {
			return msg;
		}
	}

	public static class loseItem {

		private int id;
		private byte chance, x;

		private loseItem(int id, byte chance, byte x) {
			this.id = id;
			this.chance = chance;
			this.x = x;
		}

		public int getId() {
			return id;
		}

		public byte getChance() {
			return chance;
		}

		public byte getX() {
			return x;
		}
	}

	public static class selfDestruction {

		private byte action;
		private int removeAfter;
		private int hp;

		private selfDestruction(byte action, int removeAfter, int hp) {
			this.action = action;
			this.removeAfter = removeAfter;
			this.hp = hp;
		}

		public int getHp() {
			return hp;
		}

		public byte getAction() {
			return action;
		}

		public int removeAfter() {
			return removeAfter;
		}
	}

	public static MapleMonster* getMonster(int mid) {
		try {
			MapleMonsterStats stats = monsterStats.get(Integer.valueOf(mid));
			if (stats == null) {
				Pair<MapleMonsterStats, List<MobAttackInfoHolder>> mobStats = getMonsterStats(mid);
				stats = mobStats.getLeft();
				setMonsterAttackInfo(mid, mobStats.getRight());

				monsterStats.put(Integer.valueOf(mid), stats);
			}
			MapleMonster ret = new MapleMonster(mid, stats);
			return ret;
		}
		catch (NullPointerException npe) {
			System.out.println("[SEVERE] MOB " + mid + " failed to load. Issue: " + npe.getMessage() + "\n\n");
			npe.printStackTrace();

			return null;
		}
	}
	*/

public:
	static MapleLifeFactory& getInstance() { return *mInstance; }

	MapleMonster* getMonster(int mid) { return new MapleMonster(mid); }

	MapleNPC* getNPC(int nid) { return new MapleNPC(nid, MapleDataTool::getString(npcStringData, std::to_string(nid) + "/name", "MISSINGNO")); }

	AbstractLoadedMapleLife* getLife(int id, const std::string& type)
	{
		printf("Creating %s life %d\n", type.c_str(), id);

		if (Tools::StringUtil::equalsIgnoreCase(type, "n")) { return getNPC(id); }
		else if (Tools::StringUtil::equalsIgnoreCase(type, "m")) { return getMonster(id); }
		else
		{
			printf("Unknown Life type: %s\n", type.c_str());
			return 0;
		}
	}
};

MapleLifeFactory* MapleLifeFactory::mInstance = 0;// new MapleLifeFactory();

#pragma endregion

class SpawnPoint
{
private:
	int monster, mobTime, team, fh, f;
	glm::ivec2 pos;
	long long nextPossibleSpawn;
	int mobInterval = 5000;
	int spawnedMonsters = 0;
	bool immobile, denySpawn = false;

	class MonsterSpawnerListener : public MonsterListener
	{
	private:
		SpawnPoint* mSpawnPoint;

	public:
		MonsterSpawnerListener(SpawnPoint* sp) : mSpawnPoint(sp) {}

		virtual void monsterKilled(int aniTime)
		{
			mSpawnPoint->resetNextPossibleSpawn(mSpawnPoint->getMobTime() > 0 ? mSpawnPoint->getMobTime() * 1000 : aniTime);
			mSpawnPoint->decreaseSpawnedMonsters();
		}

		virtual void monsterDamaged(MapleCharacter* from, int trueDmg) {}

		virtual void monsterHealed(int trueHeal) {}
	};

	MonsterSpawnerListener* mListener;

public:
	SpawnPoint(MapleMonster* monster, const glm::ivec2& pos, bool immobile, int mobTime, int mobInterval, int team)
	{
		this->monster = monster->getId();
		this->pos = pos;
		this->mobTime = mobTime;
		this->team = team;
		this->fh = monster->getFh();
		this->f = monster->getF();
		this->immobile = immobile;
		this->mobInterval = mobInterval;
		this->nextPossibleSpawn = Tools::currentTimeMillis();
		this->mListener = new MonsterSpawnerListener(this);
	}

	int getSpawned() { return spawnedMonsters; }
	void setDenySpawn(bool val) { denySpawn = val; }
	bool getDenySpawn() { return denySpawn; }

	bool shouldSpawn()
	{
		if (denySpawn || mobTime < 0 || spawnedMonsters > 0) { return false; }
		return nextPossibleSpawn <= Tools::currentTimeMillis();
	}

	bool shouldForceSpawn()
	{
		if (mobTime < 0 || spawnedMonsters > 0) { return false; }
		return true;
	}

	MapleMonster* getMonster()
	{
		MapleMonster* mob = MapleLifeFactory::getInstance().getMonster(monster);
		mob->setPosition(pos);
		mob->setTeam(team);
		mob->setFh(fh);
		mob->setF(f);
		spawnedMonsters++;
		mob->addListener(mListener);
		if (mobTime == 0) { resetNextPossibleSpawn(mobInterval); }
		return mob;
	}

	int getMonsterId() { return monster; }
	const glm::ivec2& getPosition() const { return pos; }
	int getF() { return f; }
	int getFh() { return fh; }
	int getMobTime() { return mobTime; }
	int getTeam() { return team; }

	void resetNextPossibleSpawn(int delay) { nextPossibleSpawn = Tools::currentTimeMillis() + delay; }
	void decreaseSpawnedMonsters() { spawnedMonsters--; }
};

const bool CONFIG_USE_MAP_MAXRANGE = false;

class MapleMap
{
public:
	MapleMap(int mapId, int returnMapId, float monsterRate) : mRunningOID(1000000001) {}

	void setTimeMob(int id, const std::string& msg) { mTimeMob = std::pair<int, std::string>(id, msg); }
	void setFieldLimit(int fieldLimit) {}
	void setMobInterval(int mobInterval) {}
	void addPortal(MaplePortal* portal) { mPortals[portal->getId()] = portal; }
	void setFootholds(MapleFootholdTree* fTree) { mFootholds = fTree; }

	void setMapPointBoundings(int px, int py, int h, int w) { mMapArea = { px, py, w, h }; }

	void setMapLineBoundings(int vrTop, int vrBottom, int vrLeft, int vrRight) { mMapArea = { vrLeft, vrTop, vrRight - vrLeft, vrBottom - vrTop }; }

	void setMapName(const std::string& mapName) { mMapName = mapName; }
	void setStreetName(const std::string& streetName) { mStreetName = streetName; }

	void setClock(bool hasClock) { mHasClock = hasClock; }
	void setEverlast(bool everlast) { mEverlast = everlast; }
	void setTown(bool isTown) { mIsTown = isTown; }
	void setForcedReturnMap(int map) { mForcedReturnMap = map; }
	void setMobCapacity(int capacity) { mMobCapacity = capacity; }

	void addMapObject(MapleMapObject* mapobject)
	{
		int curOID = getUsableOID();
		mapobject->setObjectId(curOID);
		mMapObjects[curOID] = mapobject;
	}

	glm::ivec2 calcPointBelow(const glm::ivec2& pt) { return pt; }

	static double getRangedDistance() { return CONFIG_USE_MAP_MAXRANGE ? std::numeric_limits<double>::max() : 722500; }

	void spawnAndAddRangedMapObject(MapleMapObject* mapobject/*, DelayedPacketCreation packetbakery, SpawnCondition condition*/)
	{
		std::vector<MapleCharacter*> inRangeCharacters;
		int curOID = getUsableOID();

		mapobject->setObjectId(curOID);
		mMapObjects[curOID] = mapobject;
		for (MapleCharacter* chr : mCharacters)
		{
			if (/*condition == null || condition.canSpawn(chr)*/true)
			{
				if (distanceSq(chr->getPosition(), mapobject->getPosition()) <= getRangedDistance())
				{
					inRangeCharacters.push_back(chr);
					chr->addVisibleMapObject(mapobject);
				}
			}
		}

		//for (MapleCharacter chr : inRangeCharacters) { packetbakery.sendPackets(chr.getClient()); }
	}

	void spawnMonster(MapleMonster* monster)
	{
		spawnAndAddRangedMapObject(monster);
	}

	void addMonsterSpawn(MapleMonster* monster, int mobTime, int team)
	{
		glm::ivec2 newpos = calcPointBelow(monster->getPosition());
		newpos.y -= 1;
		SpawnPoint* sp = new SpawnPoint(monster, newpos, !monster->isMobile(), mobTime, mMobInterval, team);
		mMonsterSpawnPoints.push_back(std::unique_ptr<SpawnPoint>(sp));
		if (sp->shouldSpawn() || mobTime == -1) { spawnMonster(sp->getMonster()); } // -1 does not respawn and should not either but force ONE spawn
	}

	// RAW RENDERING FUNCS
	void addRawFoothold(MapleFoothold* fh) { allFootholds.push_back(fh); }
	std::vector<MapleFoothold*>& getRawFootholds() { return allFootholds; }
	std::unordered_map<int, MaplePortal*>& getPortals() { return mPortals; }
	std::vector<std::unique_ptr<SpawnPoint>>& getSpawnPoints() { return mMonsterSpawnPoints; }
	std::unordered_map<int, MapleMapObject*>& getMapObjects() { return mMapObjects; }
	const glm::ivec4& getMapArea() const { return mMapArea; }
	const std::vector<MapleLadder*>& getLadders() { return mLadders; }
	void addLadder(MapleLadder* ld) { mLadders.push_back(ld); }

	const std::string& getMapName() { return mMapName; }
	const std::string& getStreetName() { return mStreetName; }

private:
	std::pair<int, std::string> mTimeMob;
	int mMobInterval;
	std::unordered_map<int, MaplePortal*> mPortals;
	glm::ivec4 mMapArea;
	MapleFootholdTree* mFootholds;
	std::vector<MapleFoothold*> allFootholds;
	std::vector<MapleLadder*> mLadders;

	std::string mMapName;
	std::string mStreetName;

	bool mHasClock;
	bool mEverlast;
	bool mIsTown;
	int mForcedReturnMap;
	int mMobCapacity;

	int mRunningOID;
	std::unordered_map<int, MapleMapObject*> mMapObjects;
	std::vector<std::unique_ptr<SpawnPoint>> mMonsterSpawnPoints;
	std::vector<MapleCharacter*> mCharacters;

	int getUsableOID()
	{
		do
		{
			mRunningOID++;
			if (mRunningOID >= 2147000000) { mRunningOID = 1000000001; }
		} while (mMapObjects.count(mRunningOID) != 0);

		return mRunningOID;
	}
};

class MapleMapFactory
{
private:
	static MapleMapFactory* mInstance;

	MapleData* nameData;
	MapleDataProvider* mapSource;

	MapleMapFactory()
	{
		nameData = MapleDataProviderFactory::getDataProvider("String.wz")->getData("Map.img");
		mapSource = MapleDataProviderFactory::getDataProvider("Map.wz");
	}

public:
	static MapleMapFactory& getInstance() { return *mInstance; }

	AbstractLoadedMapleLife* loadLife(int id, const std::string& type, int cy, int f, int fh, int rx0, int rx1, int x, int y, int hide)
	{
		AbstractLoadedMapleLife* myLife = MapleLifeFactory::getInstance().getLife(id, type);
		myLife->setCy(cy);
		myLife->setF(f);
		myLife->setFh(fh);
		myLife->setRx0(rx0);
		myLife->setRx1(rx1);
		myLife->setPosition(glm::ivec2(x, y));
		if (hide == 1) { myLife->setHide(true); }
		return myLife;
	}

	void loadLifeRaw(MapleMap* map, int id, const std::string& type, int cy, int f, int fh, int rx0, int rx1, int x, int y, int hide, int mobTime, int team)
	{
		AbstractLoadedMapleLife* myLife = loadLife(id, type, cy, f, fh, rx0, rx1, x, y, hide);
		if (myLife->getType() == MapleMapObjectType::MONSTER) { map->addMonsterSpawn((MapleMonster*)myLife, mobTime, team); }
		else { map->addMapObject(myLife); }
	}

	void loadLifeFromWz(MapleMap* map, MapleData* mapData)
	{
		for (MapleData* life : *mapData->getChildByPath("life"))
		{
			printf("Loading life from WZ: %s\n", life->getName().c_str());

			//life.getName();
			int id = MapleDataTool::getInt(life, "id", -1); // stored as a string? wtf nexon?
			std::string type = MapleDataTool::getString(life, "type", "");
			int team = MapleDataTool::getInt(life, "team", -1);
			/*
			if (map.isCPQMap2() && type.equals("m")) {
				if ((Integer.parseInt(life.getName()) % 2) == 0) {
					team = 0;
				}
				else {
					team = 1;
				}
			}
			*/
			int cy = MapleDataTool::getInt(life, "cy", 0);
			int f = MapleDataTool::getInt(life, "f", 0);
			int fh = MapleDataTool::getInt(life, "fh", 0);
			int rx0 = MapleDataTool::getInt(life, "rx0", 0);
			int rx1 = MapleDataTool::getInt(life, "rx1", 0);
			int x = MapleDataTool::getInt(life, "x", 0);
			int y = -MapleDataTool::getInt(life, "y", 0);
			int hide = MapleDataTool::getInt(life, "hide", 0);
			int mobTime = MapleDataTool::getInt(life, "mobTime", 0);

			loadLifeRaw(map, id, type, cy, f, fh, rx0, rx1, x, y, hide, mobTime, team);
		}
	}

	/*
	MapleReactor loadReactor(MapleData reactor, String id, final byte FacingDirection) {
		MapleReactor myReactor = new MapleReactor(MapleReactorFactory.getReactor(Integer.parseInt(id)), Integer.parseInt(id));
		int x = MapleDataTool.getInt(reactor.getChildByPath("x"));
		int y = MapleDataTool.getInt(reactor.getChildByPath("y"));
		myReactor.setFacingDirection(FacingDirection);
		myReactor.setPosition(new Point(x, y));
		myReactor.setDelay(MapleDataTool.getInt(reactor.getChildByPath("reactorTime")) * 1000);
		myReactor.setName(MapleDataTool.getString(reactor.getChildByPath("name"), ""));
		myReactor.resetReactorActions(0);
		return myReactor;
	}
	
	void loadReactors(MapleMap* map, MapleData* mapData)
	{
		if (mapData->getChildByPath("reactor") != 0)
		{
			for (MapleData* reactor : mapData->getChildByPath("reactor"))
			{
				std::string id = MapleDataTool.getString(reactor.getChildByPath("id"));
				if (id != null)
				{
					MapleReactor newReactor = loadReactor(reactor, id, (byte)MapleDataTool.getInt(reactor.getChildByPath("f"), 0));
					map.spawnReactor(newReactor);
				}
			}
		}
	}
	*/

	const std::string getMapStringName(int mapid)
	{
		std::stringstream builder;
		if (mapid < 100000000) { builder << "maple"; }
		else if (mapid >= 100000000 && mapid < 200000000) { builder << "victoria"; }
		else if (mapid >= 200000000 && mapid < 300000000) { builder << "ossyria"; }
		else if (mapid >= 300000000 && mapid < 400000000) { builder << "elin"; }
		else if (mapid >= 540000000 && mapid < 560000000) { builder << "singapore"; }
		else if (mapid >= 600000000 && mapid < 620000000) { builder << "MasteriaGL"; }
		else if (mapid >= 677000000 && mapid < 677100000) { builder << "Episode1GL"; }
		else if (mapid >= 670000000 && mapid < 682000000)
		{
			if ((mapid >= 674030000 && mapid < 674040000) || (mapid >= 680100000 && mapid < 680200000)) { builder << "etc"; }
			else { builder << "weddingGL"; }
		}
		else if (mapid >= 682000000 && mapid < 683000000) { builder << "HalloweenGL"; }
		else if (mapid >= 683000000 && mapid < 684000000) { builder << "event"; }
		else if (mapid >= 800000000 && mapid < 900000000)
		{
			if ((mapid >= 889100000 && mapid < 889200000)) { builder << "etc"; }
			else { builder << "jp"; }
		}
		else { builder << "etc"; }
		builder << "/" << mapid;
		return builder.str();
	}

	const std::string loadPlaceName(int mapid) { return MapleDataTool::getString(nameData->getChildByPath(getMapStringName(mapid)), "mapName", ""); }

	const std::string loadStreetName(int mapid) { return MapleDataTool::getString(nameData->getChildByPath(getMapStringName(mapid)), "streetName", ""); }

	const std::string getMapName(int mapid)
	{
		std::stringstream builder;
		builder << "Map/Map";
		int area = mapid / 100000000;
		builder << area;
		builder << "/";
		builder << Tools::StringUtil::getLeftPaddedStr(std::to_string(mapid), '0', 9);
		builder << ".img";
		return builder.str();
	}

	void loadBounds(MapleMap* map, MapleData* mapData, MapleData* infoData)
	{
		int bounds[4];
		bounds[0] = MapleDataTool::getInt(infoData, "VRTop", 0);
		bounds[1] = MapleDataTool::getInt(infoData, "VRBottom", 0);

		if (bounds[0] == bounds[1]) // old-style baked map
		{
			MapleData* minimapData = mapData->getChildByPath("miniMap");
			if (minimapData != 0)
			{
				bounds[0] = MapleDataTool::getInt(minimapData, "centerX", 0) * -1;
				bounds[1] = MapleDataTool::getInt(minimapData, "centerY", 0) * -1;
				bounds[2] = MapleDataTool::getInt(minimapData, "height", 0);
				bounds[3] = MapleDataTool::getInt(minimapData, "width", 0);

				map->setMapPointBoundings(bounds[0], bounds[1], bounds[2], bounds[3]);
			}
			else
			{
				int dist = (1 << 18); // TODO: might be fucked cuz of java bit ordering...
				map->setMapPointBoundings(-dist / 2, -dist / 2, dist, dist);
			}
		}
		else
		{
			bounds[2] = MapleDataTool::getInt(infoData, "VRLeft", 0);
			bounds[3] = MapleDataTool::getInt(infoData, "VRRight", 0);

			map->setMapLineBoundings(bounds[0], bounds[1], bounds[2], bounds[3]);
		}
	}

	void loadFootholds(MapleMap* map, MapleData* mapData)
	{
		std::vector<MapleFoothold*> allFootholds;
		glm::ivec2 lBound{ 0,0 };
		glm::ivec2 uBound{ 0,0 };
		for (MapleData* footRoot : *mapData->getChildByPath("foothold"))
		{
			for (MapleData* footCat : *footRoot)
			{
				for (MapleData* footHold : *footCat)
				{
					int x1 = MapleDataTool::getInt(footHold, "x1", 0);
					int y1 = -MapleDataTool::getInt(footHold, "y1", 0);
					int x2 = MapleDataTool::getInt(footHold, "x2", 0);
					int y2 = -MapleDataTool::getInt(footHold, "y2", 0);
					int id = std::stoi(footHold->getName());
					int prev = MapleDataTool::getInt(footHold, "prev", 0);
					int next = MapleDataTool::getInt(footHold, "next", 0);
					MapleFoothold* fh = new MapleFoothold(glm::ivec2(x1, y1), glm::ivec2(x2, y2), id, prev, next);
					if (fh->getX1() < lBound.x) { lBound.x = fh->getX1(); }
					if (fh->getX2() > uBound.x) { uBound.x = fh->getX2(); }
					if (fh->getY1() < lBound.y) { lBound.y = fh->getY1(); }
					if (fh->getY2() > uBound.y) { uBound.y = fh->getY2(); }
					allFootholds.push_back(fh);
				}
			}
		}
		MapleFootholdTree* fTree = new MapleFootholdTree(lBound, uBound);
		for (MapleFoothold* fh : allFootholds) { fTree->insert(fh); map->addRawFoothold(fh); }
		map->setFootholds(fTree);
	}

	void loadLadders(MapleMap* map, MapleData* mapData)
	{
		for (MapleData* ladderRope : *mapData->getChildByPath("ladderRope"))
		{
			int l = MapleDataTool::getInt(ladderRope, "l", 0);
			//int uf = MapleDataTool::getInt(ladderRope, "uf", 0);
			int x = MapleDataTool::getInt(ladderRope, "x", 0);
			int y1 = -MapleDataTool::getInt(ladderRope, "y1", 0);
			int y2 = -MapleDataTool::getInt(ladderRope, "y2", 0);
			//int page = MapleDataTool::getInt(ladderRope, "page", 0);
			MapleLadder* ld = new MapleLadder(l, x, y1, y2);
			map->addLadder(ld);
		}
	}

	MapleMap* loadMapFromWz(int mapid)
	{
		std::string mapName = getMapName(mapid);
		MapleData* mapData = mapSource->getData(mapName);
		MapleData* infoData = mapData->getChildByPath("info");

		// nexon made hundreds of dojo maps so to reduce the size they added links.
		std::string link = MapleDataTool::getString(infoData->getChildByPath("link"), "");
		if (link != "")
		{
			mapName = getMapName(std::stoi(link));
			mapData = mapSource->getData(mapName);
		}

		MapleMap* map = new MapleMap(mapid, MapleDataTool::getInt(infoData, "returnMap", 0), MapleDataTool::getFloat(infoData, "mobRate", 0));

		map->setFieldLimit(MapleDataTool::getInt(infoData, "fieldLimit", 0));
		map->setMobInterval(MapleDataTool::getInt(infoData, "createMobInterval", 5000));
		MaplePortalFactory* portalFactory = new MaplePortalFactory(); // TODO: loose pointer!
		for (MapleData* portal : *mapData->getChildByPath("portal"))
		{
			map->addPortal(portalFactory->makePortal(MapleDataTool::getInt(portal, "pt", 0), portal));
		}
		MapleData* timeMob = infoData->getChildByPath("timeMob");
		if (timeMob != 0) { map->setTimeMob(MapleDataTool::getInt(timeMob, "id", 0), MapleDataTool::getString(timeMob, "message", "")); }

		loadBounds(map, mapData, infoData);
		loadFootholds(map, mapData);
		loadLifeFromWz(map, mapData);
		//loadReactors(map, mapData);
		loadLadders(map, mapData);

		map->setMapName(loadPlaceName(mapid));
		map->setStreetName(loadStreetName(mapid));

		map->setClock(mapData->getChildByPath("clock") != 0);
		map->setEverlast(MapleDataTool::getInt(infoData, "everlast", 0) != 0); // thanks davidlafriniere for noticing value 0 accounting as true
		map->setTown(MapleDataTool::getInt(infoData, "town", 0) != 0);
		//map->setHPDec(MapleDataTool.getIntConvert("decHP", infoData, 0));
		//map->setHPDecProtect(MapleDataTool.getIntConvert("protectItem", infoData, 0));
		map->setForcedReturnMap(MapleDataTool::getInt(infoData, "forcedReturn", 999999999));
		//map->setBoat(mapData->getChildByPath("shipObj") != 0);
		//map->setTimeLimit(MapleDataTool.getIntConvert("timeLimit", infoData, -1));
		//map->setFieldType(MapleDataTool.getIntConvert("fieldType", infoData, 0));
		map->setMobCapacity(MapleDataTool::getInt(infoData, "fixedMobCapacity", 500));//Is there a map that contains more than 500 mobs?
		//map->setRecovery(MapleDataTool::getFloat(infoData, "recovery", 0));

		return map;
	}
};

MapleMapFactory* MapleMapFactory::mInstance = 0;// new MapleMapFactory();

#pragma endregion

#pragma region Core 3D Drawing Functionality

// window size
int windowWidth = 0, windowHeight = 0;

// view matrices
GLdouble projMatrix[16];
GLdouble modelMatrix[16];
GLint viewport[4];

glm::vec3 project(const glm::vec3& pos)
{
	GLdouble winX, winY, winZ;
	gluProject(pos.x, pos.y, pos.z, modelMatrix, projMatrix, viewport, &winX, &winY, &winZ);
	return glm::vec3(winX, winY, winZ);
}

Ray unproject(const glm::ivec2& pos)
{
	GLdouble startX, startY, startZ, endX, endY, endZ;
	gluUnProject(pos.x, pos.y, 0.1f, modelMatrix, projMatrix, viewport, &startX, &startY, &startZ);
	gluUnProject(pos.x, pos.y, 1000000000.0f, modelMatrix, projMatrix, viewport, &endX, &endY, &endZ);

	Ray ret;
	ret.setStart(glm::vec3(startX, startY, startZ));
	ret.setEnd(glm::vec3(endX, endY, endZ));
	return ret;
}

/*
GLUT_BITMAP_8_BY_13
GLUT_BITMAP_9_BY_15
GLUT_BITMAP_TIMES_ROMAN_10
GLUT_BITMAP_TIMES_ROMAN_24
GLUT_BITMAP_HELVETICA_10
GLUT_BITMAP_HELVETICA_12
GLUT_BITMAP_HELVETICA_18
*/
void renderSpacedBitmapString(float x, float y, int spacing, void* font, const char* string)
{
	glPushMatrix();
	glLoadIdentity();

	const char* c;
	float x1 = x;

	for (c = string; *c != '\0'; c++)
	{
		glRasterPos2f(x1, y);
		glutBitmapCharacter(font, *c);
		x1 = x1 + glutBitmapWidth(font, *c) + spacing;
	}

	glPopMatrix();
}

void renderString(int x, int y, void* font, const std::string& str) { renderSpacedBitmapString((float)x, (float)y, 0, font, str.c_str()); }

int getSpacedStringWidth(int spacing, void* font, const char* string)
{
	int width = 0;
	const char* c;

	for (c = string; *c != '\0'; c++)
	{
		width += glutBitmapWidth(font, *c) + spacing;
	}

	return width;
}

int getStringWidth(void* font, const std::string& string) { return getSpacedStringWidth(0, font, string.c_str()); }

void setOrthographicProjection() {

	// switch to projection mode
	glMatrixMode(GL_PROJECTION);

	// save previous matrix which contains the
	//settings for the perspective projection
	glPushMatrix();

	// reset matrix
	glLoadIdentity();

	// set a 2D orthographic projection
	gluOrtho2D(0, windowWidth, 0, windowHeight);

	// invert the y axis, down is positive
	glScalef(1, -1, 1);

	// mover the origin from the bottom left corner
	// to the upper left corner
	glTranslatef(0, -(float)windowHeight, 0);

	// switch back to modelview mode
	glMatrixMode(GL_MODELVIEW);

	// disable depth testing
	glDisable(GL_DEPTH_TEST);
}

void restorePerspectiveProjection() {

	glMatrixMode(GL_PROJECTION);
	// restore previous projection matrix
	glPopMatrix();

	// get back to modelview mode
	glMatrixMode(GL_MODELVIEW);

	// enable depth testing
	glEnable(GL_DEPTH_TEST);
}

/*
GLUT_STROKE_ROMAN
GLUT_STROKE_MONO_ROMAN (fixed width font: 104.76 units wide).
*/
void renderStrokeFontString(float x, float y, float z, void* font, const char* string) {

	const char* c;
	glPushMatrix();
	glTranslatef(x, y, z);
	glScalef(0.1f, 0.1f, 0.1f);

	for (c = string; *c != '\0'; c++) {
		glutStrokeCharacter(font, *c);
	}

	glPopMatrix();
}

void drawQuad2D(int x, int y, int width, int height)
{
	glBegin(GL_QUADS);
	glVertex2i(x, y);
	glVertex2i(x, y + height);
	glVertex2i(x + width, y + height);
	glVertex2i(x + width, y);
	glEnd();
}

#pragma endregion

#pragma region Camera Management

// angle of rotation for the camera direction
float angle = 0.0;
// actual vector representing the camera's direction
float lx = 0.0f, lz = -1.0f, ly = 0.0f;
// position of the camera
float cx = 0.0f, cz = 5.0f, cy = 0.0f;
// mouse camera movement
float deltaAngle = 0.0f;
float deltaAngleY = 0.0f;
int xOrigin = -1;
// desired move directions based on input
bool moveCamForward = false;
bool moveCamBackward = false;
bool moveCamLeft = false;
bool moveCamRight = false;
bool camMouseLocked = false;
POINT lastCursorPos;

void moveCamera(float direction, float speed)
{
	cx += lx * speed * direction;
	cz += lz * speed * direction;
}

void updateCamera(float elapsed)
{
	if (camMouseLocked)
	{
		// update current cursor position info
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		SetCursorPos(1920 / 2, 1080 / 2);

		// update deltaAngle
		deltaAngle += (cursorPos.x - 960) * 0.001f;
		deltaAngleY -= (cursorPos.y - 540) * 0.001f;

		// update camera's direction
		lx = sinf(deltaAngle);
		lz = -cosf(deltaAngle);
		ly = sinf(deltaAngleY);

		// update last cursor position
		lastCursorPos = cursorPos;
	}

	float moveCamZ = 0.0f;
	if (moveCamForward) { moveCamZ++; }
	if (moveCamBackward) { moveCamZ--; }
	moveCamera(moveCamZ, elapsed * 10.0f);
}

void toggleCameraMouseLock()
{
	if (camMouseLocked)
	{
		camMouseLocked = false;
		return;
	}

	camMouseLocked = true;
	SetCursorPos(1920 / 2, 1080 / 2);
	GetCursorPos(&lastCursorPos);
}

#pragma endregion

#pragma region Volume Scene Manager

struct VolumeChunk
{
	std::unique_ptr<VoxelVolume> mVolume;
	std::unique_ptr<Mesh> mMesh;
	bool mMeshNeedsUpdate = false;
	std::unique_ptr<Mesh> mUpdatedMesh;
	bool mUpdatingMesh = false;
	bool mUpdatedMeshReady = false;

	void render()
	{
		glPushMatrix();
		glTranslatef((float)mVolume->getEnclosingRegion().getLowerCorner().x, (float)mVolume->getEnclosingRegion().getLowerCorner().y, (float)mVolume->getEnclosingRegion().getLowerCorner().z);

		if (mMesh.get() == 0)
		{
			glTranslatef(8.0f, 8.0f, 8.0f);
			glColor4f(0.2f, 0.2f, 0.2f, 0.3f);
			glutSolidCube(16.0f);
		}
		else
		{
			glBegin(GL_TRIANGLES);
			for (size_t i = 0; i < mMesh->getNumIndices(); i++)
			{
				const Vertex& vert = mMesh->getRenderVertex(i);
				glColor3f(vert.mColor.r, vert.mColor.g, vert.mColor.b);
				glNormal3f(vert.mNormal.x, vert.mNormal.y, vert.mNormal.z);
				glVertex3f(vert.mPosition.x, vert.mPosition.y, vert.mPosition.z);
			}
			glEnd();
		}

		glPopMatrix();
	}
};

struct KeyHash_GLMIVec3
{
	size_t operator()(const glm::ivec3& k) const
	{
		return std::hash<int>()(k.x) ^ std::hash<int>()(k.y) ^ std::hash<int>()(k.z);
	}
};
struct KeyEqual_GLMIVec3
{
	bool operator()(const glm::ivec3& a, const glm::ivec3& b) const
	{
		return a.x == b.x && a.y == b.y && a.z == b.z;
	}
};

std::unordered_map<glm::ivec3, std::unique_ptr<VolumeChunk>, KeyHash_GLMIVec3, KeyEqual_GLMIVec3> mChunks;

VolumeChunk* initChunk(int x, int y, int z)
{
	glm::ivec3 chunkStart(x * 16, y * 16, z * 16);
	VolumeChunk* chunk = new VolumeChunk();
	chunk->mVolume.reset(new VoxelVolume(chunkStart.x, chunkStart.y, chunkStart.z, chunkStart.x + 16, chunkStart.y + 16, chunkStart.z + 16));
	chunk->mMesh.reset(new Mesh());
	mChunks[glm::ivec3(x, y, z)].reset(chunk);

	printf("Inited chunk [%d, %d, %d]\n", x, y, z);

	return chunk;
}

void setVoxel(int x, int y, int z, unsigned char r, unsigned char g, unsigned char b)
{
	glm::ivec3 chunkPos(std::floorf((float)x / 16.0f), std::floorf((float)y / 16.0f), std::floorf((float)z / 16.0f));
	VolumeChunk* chunk;

	if (mChunks.count(chunkPos) == 0) { chunk = initChunk(chunkPos.x, chunkPos.y, chunkPos.z); }
	else { chunk = mChunks[chunkPos].get(); }

	if (!chunk->mVolume->setVoxelAt(x, y, z, VoxelType(r, g, b, 255))) { printf("Failed to set voxel! (%d, %d, %d)\n", x, y, z); return; }
	chunk->mMeshNeedsUpdate = true;
}

const VoxelType& getVoxel(int x, int y, int z)
{
	glm::ivec3 chunkPos(std::floorf((float)x / 16.0f), std::floorf((float)y / 16.0f), std::floorf((float)z / 16.0f));
	VolumeChunk* chunk;

	if (mChunks.count(chunkPos) == 0) { chunk = initChunk(chunkPos.x, chunkPos.y, chunkPos.z); }
	else { chunk = mChunks[chunkPos].get(); }

	return chunk->mVolume->getVoxelAt(x, y, z);
}

const VoxelType& getVoxel(const glm::ivec3& pos) { return getVoxel(pos.x, pos.y, pos.z); }

int volumeRenderDistance = 3;
int volumeMaxSurfaceExtractionThreads = 4;
std::atomic<int> activeSurfaceExtractionThreads = 0;

void chunkSurfaceExtractProc(VolumeChunk* chunk)
{
	chunk->mUpdatedMesh.reset(new Mesh());
	extractVolumeSurface(chunk->mVolume.get(), chunk->mUpdatedMesh.get());
	chunk->mUpdatedMeshReady = true;
	chunk->mMeshNeedsUpdate = false;
	chunk->mUpdatingMesh = false;
	activeSurfaceExtractionThreads--;
}

void renderChunks()
{
	glm::vec3 camPos(cx, 0, cz);

	for (auto it = mChunks.begin(); it != mChunks.end(); it++)
	{
		VolumeChunk* chunk = it->second.get();

		// apply max render distance
		const glm::ivec3& corner = chunk->mVolume.get()->getEnclosingRegion().getLowerCorner();
		glm::vec3 volumeCenterWorldPos(corner.x + 8, corner.y + 8, corner.z + 8);
		if (glm::distance(camPos, volumeCenterWorldPos) >= (volumeRenderDistance * 16)) { continue; }

		// update and render chunk geometry
		if (!chunk->mUpdatingMesh)
		{
			if (chunk->mMeshNeedsUpdate && activeSurfaceExtractionThreads < volumeMaxSurfaceExtractionThreads)
			{
				activeSurfaceExtractionThreads++;
				chunk->mUpdatingMesh = true;
				chunk->mMesh.reset();
				std::thread t(chunkSurfaceExtractProc, chunk);
				t.detach();
			}
			// with modern buffered rendering, gpu pushes must happen on the main thread, so this is where it'd be done
			else if (chunk->mUpdatedMeshReady)
			{
				chunk->mMesh = std::move(chunk->mUpdatedMesh);
				chunk->mUpdatedMeshReady = false;
			}
		}

		chunk->render();
	}
}

glm::ivec3 getVoxelChunkPos(int x, int y, int z) { return glm::ivec3(std::floorf((float)x / 16.0f), std::floorf((float)y / 16.0f), std::floorf((float)z / 16.0f)); }

glm::ivec3 getVoxelChunkPos(const glm::ivec3& pos) { return getVoxelChunkPos(pos.x, pos.y, pos.z); }

#pragma endregion

#pragma region Maple Map 3D Processing

std::random_device rd;
std::mt19937 mt(rd());

int getRandomInt() { return std::uniform_int_distribution<int>()(mt); }

int getRandomInt(int min, int max) { return std::uniform_int_distribution<int>(min, max)(mt); }

glm::vec3 charPos;

int randomNumber(int LO, int HI) { return LO + (rand()) / ((RAND_MAX / (HI - LO))); }

float randf(float LO, float HI) { return LO + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (HI - LO))); }

bool fhTraceCb(VolumeSampler& s)
{
	for (int z = 0; z < 10; z++)
	{
		setVoxel(s.getPosition().x, s.getPosition().y, s.getPosition().z + z, rand() % 255, rand() % 255, rand() % 255); // base raytrace
		//setVoxel(s.getPosition().x + randomNumber(-1, 1), s.getPosition().y + randomNumber(-1, 1), s.getPosition().z + z + randomNumber(-1, 1), rand() % 255, rand() % 255, rand() % 255); // level 1 noise
	}
	return true;
}

MapleMap* loadedMapleMap = 0;

float mapleMapScaling = 10.0f;

void switchMapleMap(int mapId)
{
	mChunks.clear();

	loadedMapleMap = MapleMapFactory::getInstance().loadMapFromWz(mapId);

	for (unsigned int i = 0; i < loadedMapleMap->getRawFootholds().size(); i++)
	{
		MapleFoothold* fh = loadedMapleMap->getRawFootholds()[i];
		raycastWithEndpoints(0, glm::vec3(fh->getX1() / mapleMapScaling, fh->getY1() / mapleMapScaling, 0), glm::vec3(fh->getX2() / mapleMapScaling, fh->getY2() / mapleMapScaling, 0), fhTraceCb);
	}

	const glm::ivec4& mapArea = loadedMapleMap->getMapArea();
	glm::ivec2 top(mapArea[0], mapArea[1]);
	top /= mapleMapScaling;
	top.y = -top.y;
	glm::ivec2 bottom(mapArea[0] + mapArea[2], mapArea[1] + mapArea[3]);
	bottom /= mapleMapScaling;
	bottom.y = -bottom.y;
	raycastWithEndpoints(0, glm::vec3(top.x, top.y, 0), glm::vec3(bottom.x, top.y, 0), fhTraceCb); // top
	raycastWithEndpoints(0, glm::vec3(top.x, bottom.y, 0), glm::vec3(bottom.x, bottom.y, 0), fhTraceCb); // bottom
	raycastWithEndpoints(0, glm::vec3(top.x, top.y, 0), glm::vec3(top.x, bottom.y, 0), fhTraceCb); // left
	raycastWithEndpoints(0, glm::vec3(bottom.x, top.y, 0), glm::vec3(bottom.x, bottom.y, 0), fhTraceCb); // right

	/*
	for (int x = 0; x < 64; x++)
	{
		for (int y = 0; y < 3; y++)
		{
			for (int z = 0; z < 64; z++)
			{
				if (rand() % 100 < 50)
					setVoxel(x, y, z, rand() % 255, rand() % 255, rand() % 255);
			}
		}
	}
	*/
}

glm::vec3 mapleMapPosToWorldPos(const glm::ivec2& pos) { return glm::vec3((float)pos.x / mapleMapScaling, (float)pos.y / mapleMapScaling, 0.0f); }

void charAttemptEnterPortal()
{
	bool switched = false;
	std::string targetPortalName;

	for (auto& portal : loadedMapleMap->getPortals())
	{
		if (portal.second->getType() == MaplePortal::MAP_PORTAL)
		{
			if (glm::distance(mapleMapPosToWorldPos(portal.second->getPosition()), charPos) < 5.0f)
			{
				int targetMapId = portal.second->getTargetMapId();
				if (targetMapId == 0) { printf("Portal has no target map id specified!\n"); return; }
				targetPortalName = portal.second->getTarget();
				printf("Attempting map switch to id %d, pn %s\n", targetMapId, targetPortalName.c_str());
				switchMapleMap(targetMapId);
				switched = true;
				break;
			}
		}
	}

	bool found = false;

	if (switched)
	{
		for (auto& portal : loadedMapleMap->getPortals())
		{
			if (portal.second->getType() == MaplePortal::MAP_PORTAL)
			{
				if (portal.second->getName() == targetPortalName)
				{
					charPos = mapleMapPosToWorldPos(portal.second->getPosition());
					found = true;
					break;
				}
			}
		}
	}

	if (!found) { printf("pn %s not found\n", targetPortalName.c_str()); }
}

void drawLoadedMapleMap()
{
	renderChunks();

	// render portals
	int numMapPortals = 0;
	for (auto& portal : loadedMapleMap->getPortals())
	{
		if (portal.second->getType() == MaplePortal::MAP_PORTAL)
		{
			glPushMatrix();
			glColor3f(0.2f, 0.2f, 1.0f);
			const glm::ivec2& pos = portal.second->getPosition();
			glTranslatef((float)pos.x / mapleMapScaling, (float)pos.y / mapleMapScaling, 0.0f);
			glutSolidSphere(2.0f, 20, 20);
			glPopMatrix();

			numMapPortals++;
		}
	}

	// render spawn points
	for (auto& sp : loadedMapleMap->getSpawnPoints())
	{
		glPushMatrix();
		glColor3f(1.0f, 0.2f, 0.2f);
		const glm::ivec2& pos = sp->getPosition();
		glTranslatef((float)pos.x / mapleMapScaling, (float)pos.y / mapleMapScaling, 0.0f);
		glutSolidSphere(2.0f, 20, 20);
		glPopMatrix();
	}

	// render npcs
	int numNPCs = 0;
	for (auto& mo : loadedMapleMap->getMapObjects())
	{
		if (mo.second->getType() == MapleMapObjectType::NPC)
		{
			glPushMatrix();
			glColor3f(0.2f, 1.0f, 0.2f);
			const glm::ivec2& pos = mo.second->getPosition();
			glTranslatef((float)pos.x / mapleMapScaling, (float)pos.y / mapleMapScaling, 0.0f);
			glutSolidSphere(2.0f, 20, 20);
			glPopMatrix();

			numNPCs++;
		}
	}

	// render ladders
	glLineWidth(5.0f);
	for (auto& ladder : loadedMapleMap->getLadders())
	{
		//glColor3f(1.0f, 0.2f, 1.0f);
		glBegin(GL_LINE_STRIP);
		glVertex3f((float)ladder->getX() / mapleMapScaling, (float)ladder->getY1() / mapleMapScaling, 0.0f);
		glVertex3f((float)ladder->getX() / mapleMapScaling, (float)ladder->getY2() / mapleMapScaling, 0.0f);
		glVertex3f((float)ladder->getX() / mapleMapScaling, (float)ladder->getY2() / mapleMapScaling, 0.0f);
		glEnd();
	}

	// render player character
	glPushMatrix();
	glColor3f(0.46f, 0.29f, 0.587f);
	glTranslatef(charPos.x, charPos.y, charPos.z);
	glutSolidSphere(2.0f, 20, 20);
	glPopMatrix();
}

void drawLoadedMapleMapStatistics()
{
	// TEMP :: FROM DRAW LOADED MAP
	int numMapPortals = 0;
	int numNPCs = 0;

	std::string fhStr = "Footholds: " + std::to_string(loadedMapleMap->getRawFootholds().size());
	std::string ptStr = "Portals: " + std::to_string(loadedMapleMap->getPortals().size()) + " | Map: " + std::to_string(numMapPortals);
	std::string spStr = "SpawnPoints: " + std::to_string(loadedMapleMap->getSpawnPoints().size());
	std::string npStr = "NPCs: " + std::to_string(numNPCs);
	std::string ldStr = "Ladders: " + std::to_string(loadedMapleMap->getLadders().size());
	renderSpacedBitmapString(5, 50, 0, GLUT_BITMAP_HELVETICA_18, fhStr.c_str());
	renderSpacedBitmapString(5, 70, 0, GLUT_BITMAP_HELVETICA_18, ptStr.c_str());
	renderSpacedBitmapString(5, 90, 0, GLUT_BITMAP_HELVETICA_18, spStr.c_str());
	renderSpacedBitmapString(5, 110, 0, GLUT_BITMAP_HELVETICA_18, npStr.c_str());
	renderSpacedBitmapString(5, 130, 0, GLUT_BITMAP_HELVETICA_18, ldStr.c_str());

	std::string mnStr = loadedMapleMap->getStreetName() + ": " + loadedMapleMap->getMapName();
	renderSpacedBitmapString(5, 170, 0, GLUT_BITMAP_HELVETICA_18, mnStr.c_str());
}

#pragma endregion

#pragma region Collision Detection

int inline GetIntersection(float fDst1, float fDst2, glm::vec3 P1, glm::vec3 P2, glm::vec3& Hit) {
	if ((fDst1 * fDst2) >= 0.0f) return 0;
	if (fDst1 == fDst2) return 0;
	Hit = P1 + (P2 - P1) * (-fDst1 / (fDst2 - fDst1));
	return 1;
}

int inline InBox(glm::vec3 Hit, glm::vec3 B1, glm::vec3 B2, const int Axis) {
	if (Axis == 1 && Hit.z > B1.z && Hit.z < B2.z && Hit.y > B1.y && Hit.y < B2.y) return 1;
	if (Axis == 2 && Hit.z > B1.z && Hit.z < B2.z && Hit.x > B1.x && Hit.x < B2.x) return 1;
	if (Axis == 3 && Hit.x > B1.x && Hit.x < B2.x && Hit.y > B1.y && Hit.y < B2.y) return 1;
	return 0;
}

// returns true if line (L1, L2) intersects with the box (B1, B2)
// returns intersection point in Hit
int CheckLineBox(glm::vec3 B1, glm::vec3 B2, glm::vec3 L1, glm::vec3 L2, glm::vec3& Hit)
{
	if (L2.x < B1.x && L1.x < B1.x) return false;
	if (L2.x > B2.x && L1.x > B2.x) return false;
	if (L2.y < B1.y && L1.y < B1.y) return false;
	if (L2.y > B2.y && L1.y > B2.y) return false;
	if (L2.z < B1.z && L1.z < B1.z) return false;
	if (L2.z > B2.z && L1.z > B2.z) return false;
	if (L1.x > B1.x && L1.x < B2.x &&
		L1.y > B1.y && L1.y < B2.y &&
		L1.z > B1.z && L1.z < B2.z)
	{
		Hit = L1;
		return true;
	}
	if ((GetIntersection(L1.x - B1.x, L2.x - B1.x, L1, L2, Hit) && InBox(Hit, B1, B2, 1))
		|| (GetIntersection(L1.y - B1.y, L2.y - B1.y, L1, L2, Hit) && InBox(Hit, B1, B2, 2))
		|| (GetIntersection(L1.z - B1.z, L2.z - B1.z, L1, L2, Hit) && InBox(Hit, B1, B2, 3))
		|| (GetIntersection(L1.x - B2.x, L2.x - B2.x, L1, L2, Hit) && InBox(Hit, B1, B2, 1))
		|| (GetIntersection(L1.y - B2.y, L2.y - B2.y, L1, L2, Hit) && InBox(Hit, B1, B2, 2))
		|| (GetIntersection(L1.z - B2.z, L2.z - B2.z, L1, L2, Hit) && InBox(Hit, B1, B2, 3)))
		return true;

	return false;
}

#pragma endregion

#pragma region Pathfinding

struct KeyHash_GLMIVec2
{
	size_t operator()(const glm::ivec2& k) const
	{
		return std::hash<int>()(k.x) ^ std::hash<int>()(k.y);
	}
};
struct KeyEqual_GLMIVec2
{
	bool operator()(const glm::ivec2& a, const glm::ivec2& b) const
	{
		return a.x == b.x && a.y == b.y;
	}
};

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

#pragma endregion

bool invalidPathfindNode(const glm::ivec2& node)
{
	return false; // temp

	//if (node.x < 0 || node.y < 0 || node.x >= 57 || node.y >= 57) { return true; }
	//if (mazeWalls[node.x][node.y][0] && mazeWalls[node.x][node.y][1] && mazeWalls[node.x][node.y][2] && mazeWalls[node.x][node.y][3]) { return true; }
	//return false;
}

glm::ivec2 worldToMazePos(const glm::vec3& worldPos) { return glm::ivec2((int)std::floorf((worldPos.x + 200.0f) / 7.0f), (int)std::floorf((worldPos.z + 200.0f) / 7.0f)); }

glm::vec3 mazeToWorldPos(const glm::ivec2& pos) { return glm::vec3(-200 + (pos.x * 7) + 3, 0.0f, -200 + (pos.y * 7) + 3); }

#pragma region Combat Engine

class ICombatSkill
{
private:
	std::string mName;

public:
	ICombatSkill(const std::string& name) : mName(name) {}

	const std::string& getName() const { return mName; }

	virtual void attemptCast() = 0;
};

std::unordered_map<int, std::unique_ptr<ICombatSkill>> loadedSkills;

void registerSkill(int id, ICombatSkill* skill) { loadedSkills[id].reset(skill); }

class CombatEntity;

class ICombatEntityListener
{
public:
	virtual void onKilled(CombatEntity* entity) = 0;
};

class CombatEntity
{
protected:
	glm::vec3 mPosition;

	std::vector<ICombatEntityListener*> mListeners;

public:
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
};

#pragma endregion

#pragma region Player Stats

int playerHP = 10;
int playerMaxHP = 10;
int playerMP = 5;
int playerMaxMP = 5;
int playerEXP = 0;
int playerLevel = 1;

int getEXPNeeded(int level) { return 7 + (level * 9) + (((level - 1) * 3) * level); }

void playerGainEXP(int exp)
{
	playerEXP += exp;
	if (playerEXP >= getEXPNeeded(playerLevel))
	{
		playerLevel++;
		playerEXP = 0;

		playerMaxHP += randomNumber(2, 6);
		playerMaxMP += randomNumber(1, 3);
	}
}

long long playerLastMpRegen = 0;
glm::vec3 playerLastPos;
float playerSamePosTime = 0;
float playerTotalTravelDistance = 0;

bool playerJumpRequested = false;
bool playerJumpProcessing = false;
float playerJumpTime = 0;
bool playerOnGround = true;
float playerFallTime = 0;
float playerVerticalVelocity = 0.0f;

glm::ivec3 getPlayerPositionVoxelPos() { return glm::ivec3((int)(cx < 0 ? std::ceilf(cx) - 1 : std::floorf(cx)), (int)(cy < 0 ? std::ceilf(cy) - 1 : std::floorf(cy)), (int)(cz < 0 ? std::ceilf(cz) - 1 : std::floorf(cz))); }

// apply physics based on voxel terrain
class PhysicalEnvironment
{
private:
	glm::ivec3 curVoxelPos;

public:
	void preMove(float elapsed)
	{
		curVoxelPos = getPlayerPositionVoxelPos();
		const VoxelType& curVoxel = getVoxel(curVoxelPos);

		// process jumping and gravity
		glm::ivec3 curVoxelUnderPos(curVoxelPos.x, curVoxelPos.y - 1, curVoxelPos.z);
		const VoxelType& curVoxelUnder = getVoxel(curVoxelUnderPos);

		// check if player is standing on solid ground currently. STRANGE BEHAVIOR FOR COLLISIONS AT cy < 0. NEEDS FIXING.
		bool playerCurrentlyOnGround = curVoxelUnder.a != 0 && (cy < 0 ? std::floorf(cy) : cy) == curVoxelPos.y;

		if (playerCurrentlyOnGround != playerOnGround)
		{
			playerFallTime = 0;
			playerOnGround = playerCurrentlyOnGround;
			if (playerOnGround) { playerVerticalVelocity = 0; }
		}

		// initiate jumps if on solid ground
		if (playerJumpRequested)
		{
			playerJumpRequested = false;
			if (playerOnGround)
			{
				playerJumpTime = 0;
				playerJumpProcessing = true;
			}
		}

		// process jump acceleration changes
		if (playerJumpProcessing)
		{
			playerJumpTime = std::min(playerJumpTime + (elapsed * 2), 1.0f);
			float jumpAcceleration = glm::mix(3.5f, 0.0f, playerJumpTime);
			float frameAcceleration = jumpAcceleration * (elapsed * 2);
			playerVerticalVelocity = frameAcceleration;

			if (playerJumpTime == 1.0f) { playerJumpProcessing = false; }
		}

		// apply gravity
		if (!playerOnGround && !playerJumpProcessing)
		{
			playerFallTime += elapsed;
			float fallAcceleration = std::max(-3.130495f * playerFallTime, -7.280109f); // use real world acceleration and terminal velocity, 9.8m2 and 53m2
			playerVerticalVelocity += fallAcceleration * elapsed;
		}

		// apply vertical velocity
		cy += playerVerticalVelocity;

		if (curVoxelUnder.a != 0 && cy < curVoxelPos.y) { cy = (float)curVoxelPos.y; }
	}

	void postMove()
	{
		glm::ivec3 newVoxelPos = getPlayerPositionVoxelPos();
		const VoxelType& newVoxel = getVoxel(newVoxelPos);

		// DEBUG: draw player position voxel
		glPushMatrix();
		glTranslatef((float)newVoxelPos.x, (float)newVoxelPos.y, (float)newVoxelPos.z);
		glTranslatef(0.5f, 0.5f, 0.5f);
		glColor3f(0.5f, 0.5f, 0.5f);
		glutWireCube(1.0f);
		glPopMatrix();

		if (newVoxelPos != curVoxelPos)
		{
			//printf("=== CHANGED VOXELS!! ===\n");
			//printf("old: %d, %d, %d | new: %d, %d, %d\n", curVoxelPos.x, curVoxelPos.y, curVoxelPos.z, newVoxelPos.x, newVoxelPos.y, newVoxelPos.z);
			if (newVoxel.a != 0) // isn't air voxel
			{
				glm::vec3 oldLower((float)curVoxelPos.x + 0.0001f, (float)curVoxelPos.y + 0.0001f, (float)curVoxelPos.z + 0.0001f);
				glm::vec3 oldHigher(oldLower.x + 0.9998f, oldLower.y + 0.9998f, oldLower.z + 0.9998f);

				//printf("=== VOXEL COLLISION!! ===\n");
				//printf("old bounds: %f, %f, %f to %f, %f, %f\n", oldLower.x, oldLower.y, oldLower.z, oldHigher.x, oldHigher.y, oldHigher.z);

				if (cx < oldLower.x && getVoxel(newVoxelPos.x, 0, curVoxelPos.z).a != 0) { cx = oldLower.x; }
				if (cx > oldHigher.x && getVoxel(newVoxelPos.x, 0, curVoxelPos.z).a != 0) { cx = oldHigher.x; }
				if (cz < oldLower.z && getVoxel(curVoxelPos.x, 0, newVoxelPos.z).a != 0) { cz = oldLower.z; }
				if (cz > oldHigher.z && getVoxel(curVoxelPos.x, 0, newVoxelPos.z).a != 0) { cz = oldHigher.z; }
			}
		}
	}
};

void updatePlayer(float elapsed)
{
	// apply mp regen
	if (Tools::currentTimeMillis() - playerLastMpRegen > 10000)
	{
		if (playerMP < playerMaxMP) { playerMP = std::min(playerMaxMP, playerMP + 2); }
		playerLastMpRegen = Tools::currentTimeMillis();
	}

	glm::vec3 playerCurrentPos(cx, 0.0f, cz);

	// apply hp regen if standing in the same spot for 5 sec at a time
	if (playerCurrentPos != playerLastPos)
	{
		playerLastPos = playerCurrentPos;
		playerSamePosTime = 0;
	}
	else if (playerSamePosTime >= 5.0f)
	{
		if (playerHP < playerMaxHP) { playerHP = std::min(playerMaxHP, playerHP + 2); }
		playerSamePosTime = 0;
	}
	else { playerSamePosTime += elapsed; }

	// apply camera movement with physics
	PhysicalEnvironment penv;
	penv.preMove(elapsed);
	updateCamera(elapsed); // temporarily do hard camera movement here, so we can apply physics & stats easily
	penv.postMove();

	// update statistics
	playerTotalTravelDistance += glm::distance(glm::vec3(cx, 0.0f, cz), playerCurrentPos);
}

#pragma region Items & Inventory

enum class InventoryType
{
	UNDEFINED,
	EQUIP,
	USE,
	SETUP,
	ETC,
	CASH,
	EQUIPPED = -1
};

InventoryType getItemInventoryType(int itemId)
{
	InventoryType ret = InventoryType::UNDEFINED;

	int type = itemId / 1000000;
	if (type >= 1 && type <= 5) { ret = (InventoryType)type; }

	return ret;
}

class Item
{
private:
	int mId;
	short mPosition;
	short mQuantity;

public:
	Item(int id) : mId(id), mPosition(0), mQuantity(1) {}
	Item(int id, short quantity) : mId(id), mPosition(0), mQuantity(quantity) {}
	Item(int id, short position, short quantity) : mId(id), mPosition(position), mQuantity(quantity) {}

	const int& getItemId() { return mId; }
	const short& getPosition() { return mPosition; }
	const short& getQuantity() { return mQuantity; }

	void setPosition(short position) { mPosition = position; }
	void setQuantity(short quantity) { mQuantity = quantity; }

	InventoryType getInventoryType() { return getItemInventoryType(mId); }
};

class Inventory
{
private:
	std::unordered_map<short, std::unique_ptr<Item>> mItems;
	short mSlotLimit;
	InventoryType mType;

	short getNextFreeSlot()
	{
		if (isFull()) { return -1; }

		for (short i = 1; i <= mSlotLimit; i++)
		{
			if (mItems.count(i) == 0) { return i; }
		}

		return -1;
	}

	short addSlot(Item* item)
	{
		if (item == 0) { return -1; }

		short slotId = getNextFreeSlot();
		if (slotId < 0) { return -1; }

		mItems[slotId].reset(item);

		return slotId;
	}

	void removeSlot(short slot) { mItems.erase(slot); }

public:
	Inventory(InventoryType type, short slotLimit) : mType(type), mSlotLimit(slotLimit) {}

	const short& getSlotLimit() const { return mSlotLimit; }
	const InventoryType& getType() const { return mType; }

	Item* getItem(short slot) { return mItems.count(slot) != 0 ? mItems[slot].get() : 0; }

	bool isFull() { return (short)mItems.size() >= mSlotLimit; }

	short addItem(Item* item)
	{
		short slotId = addSlot(item);
		if (slotId == -1) { return -1; }
		item->setPosition(slotId);
		return slotId;
	}

	void removeItem(short slot, short quantity, bool allowZero)
	{
		Item* item = getItem(slot);
		if (item == 0) { return; }
		item->setQuantity(item->getQuantity() - quantity);
		if (item->getQuantity() < 0) { item->setQuantity(0); }
		if (item->getQuantity() == 0 && !allowZero) { removeSlot(slot); }
	}

	void removeItem(short slot) { removeItem(slot, 1, false); }

	int countById(int itemId)
	{
		int qty = 0;
		for (auto& item : mItems)
		{
			if (item.second->getItemId() == itemId) { qty += item.second->getQuantity(); }
		}
		return qty;
	}

	short getNumFreeSlot()
	{
		if (isFull()) { return 0; }

		short free = 0;
		for (short i = 1; i <= mSlotLimit; i++)
		{
			if (mItems.count(i) == 0) { free++; }
		}
		return free;
	}

	Item* findById(int itemId)
	{
		for (auto& item : mItems)
		{
			if (item.second->getItemId() == itemId) { return item.second.get(); }
		}
		return 0;
	}
};

Inventory playerInventoryItems(InventoryType::EQUIP, 36);
std::vector<int> playerEquipmentItems;

#pragma endregion

#pragma region Skills

class LearnedSkill
{
private:
	int mId;
	int mLevel;
	int mEXP;

	static int getNextEXP(int level) { return 11 + (level * 37) + (((level - 1) * 3) * level); }

public:
	LearnedSkill(int id, int level, int exp) : mId(id), mLevel(level), mEXP(exp) {}

	const int& getId() const { return mId; }
	const int& getLevel() const { return mLevel; }
	const int& getExp() const { return mEXP; }
	int getLevelUpExp() { return getNextEXP(mLevel); }

	void gainExp(int exp)
	{
		mEXP += exp;
		if (mEXP >= getLevelUpExp())
		{
			mLevel++;
			mEXP = 0;
		}
	}
};

std::vector<std::unique_ptr<LearnedSkill>> playerSkills;

void playerSkillGainExp(int id, int exp)
{
	for (unsigned int i = 0; i < playerSkills.size(); i++)
	{
		if (playerSkills[i]->getId() == id)
		{
			playerSkills[i]->gainExp(exp);
			break;
		}
	}
}

#pragma endregion

CombatEntity* playerEntity = new CombatEntity();

#pragma endregion

#pragma region Enemies

void drawSnowMan() {

	glColor3f(1.0f, 1.0f, 1.0f);

	// Draw Body
	glTranslatef(0.0f, 0.75f, 0.0f);
	glutSolidSphere(0.75f, 20, 20);

	// Draw Head
	glTranslatef(0.0f, 1.0f, 0.0f);
	glutSolidSphere(0.25f, 20, 20);

	// Draw Eyes
	glPushMatrix();
	glColor3f(0.0f, 0.0f, 0.0f);
	glTranslatef(0.05f, 0.10f, 0.18f);
	glutSolidSphere(0.05f, 10, 10);
	glTranslatef(-0.1f, 0.0f, 0.0f);
	glutSolidSphere(0.05f, 10, 10);
	glPopMatrix();

	// Draw Nose
	glColor3f(1.0f, 0.5f, 0.5f);
	glutSolidCone(0.08f, 0.5f, 10, 2);
}

class Enemy : public CombatEntity
{
private:
	glm::vec3 mMoveDirection;
	std::vector<glm::ivec2> mPathfindPath;
	long long mLastPathfindCalcTime = 0;
	std::vector<glm::ivec2> mLatestPathfindPath;
	bool mNewerPathfindAvailable = false;
	bool mCurrentlyPathfinding = false;

	int mHP = 5;
	int mMaxHP = 5;
	int mAttackDamage = 1;
	float mMoveSpeed = 7.5f;

	long long mLastAttackTime = 0;

	static void calculateMovementPath(Enemy* enemy, const glm::ivec2& startPos, const glm::ivec2& targetPos)
	{
		enemy->mCurrentlyPathfinding = true;
		AStar::Pathfinder pathfinder;
		enemy->mLatestPathfindPath = pathfinder.findPath(startPos, targetPos, invalidPathfindNode);
		enemy->mNewerPathfindAvailable = true;
		enemy->mCurrentlyPathfinding = false;
	}

public:
	Enemy(const glm::vec3& pos) { setPosition(pos); }

	void update(float elapsed)
	{
		glm::vec3 playerPos(cx, 0.0f, cz);
		float distToPlayer = glm::distance(mPosition, playerPos);

		// update pathfinding
		if (distToPlayer > 7.0f)
		{
			// swap latest calculated pathfind path in
			if (mNewerPathfindAvailable)
			{
				mPathfindPath = mLatestPathfindPath;
				mNewerPathfindAvailable = false;
			}

			// determine if path should be recalculated
			if (!mCurrentlyPathfinding && Tools::currentTimeMillis() - mLastPathfindCalcTime > 3000)
			{
				mLastPathfindCalcTime = Tools::currentTimeMillis();

				glm::ivec2 startPos(worldToMazePos(mPosition));
				glm::ivec2 targetPos(worldToMazePos(playerPos));
				if (!invalidPathfindNode(startPos) && !invalidPathfindNode(targetPos)) // invalid endpoints crash
				{
					std::thread t(std::bind(calculateMovementPath, this, startPos, targetPos));
					t.detach();
				}
			}
		}
		else
		{
			mPathfindPath.clear();
		}

		// recalculate movement direction
		if (mPathfindPath.size() > 1)
		{
			if (glm::distance(mPosition, mazeToWorldPos(mPathfindPath[0])) < 0.5f) { mPathfindPath.erase(mPathfindPath.begin()); }

			mMoveDirection = mazeToWorldPos(mPathfindPath[0]) - mPosition;
			mMoveDirection = glm::normalize(mMoveDirection);
		}
		else if (distToPlayer <= 7.0f)
		{
			mMoveDirection = playerPos - mPosition;
			mMoveDirection = glm::normalize(mMoveDirection);
		}
		else
		{
			mMoveDirection.x = 0;
			mMoveDirection.z = 0;
		}

		// move towards player
		if (distToPlayer >= 2.5f) { mPosition += mMoveDirection * elapsed * mMoveSpeed; }

		// attack if close enough to player
		if (Tools::currentTimeMillis() - mLastAttackTime > 1500 && distToPlayer <= 3.5f)
		{
			mLastAttackTime = Tools::currentTimeMillis();
			playerHP -= mAttackDamage;
			if (playerHP <= 0) { playerEntity->onKilled(); }
		}
	}

	void draw()
	{
		// draw mesh
		glPushMatrix();
		glTranslatef(mPosition.x, mPosition.y, mPosition.z);
		drawSnowMan();
		glPopMatrix();

		// draw aabb
		if (Tools::currentTimeMillis() - mLastAttackTime <= 1500) { glColor3f(1.0f, 0.0f, 0.0f); }
		else { glColor3f(0.0f, 0.5f, 0.5f); }
		glPushMatrix();
		glTranslatef(mPosition.x, mPosition.y + 1, mPosition.z);
		glScalef(2, 2, 2);
		glutWireCube(1.0f);
		glPopMatrix();
	}

	int getHP() { return mHP; }
	void setHP(int hp) { mHP = hp; }
	int getMaxHP() { return mMaxHP; }
	void setMaxHP(int maxhp) { mMaxHP = maxhp; }
	int getAttackDamage() { return mAttackDamage; }
	void setAttackDamage(int attackDamage) { mAttackDamage = attackDamage; }
	float getMoveSpeed() { return mMoveSpeed; }
	void setMoveSpeed(float movespeed) { mMoveSpeed = movespeed; }
};

std::vector<std::unique_ptr<Enemy>> enemies;

class EnemySpawnPoint
{
private:
	glm::vec3 mPosition;
	long long mNextSpawnTime;
	int mSpawnedEnemies;

	class MobListener : public ICombatEntityListener
	{
	private:
		EnemySpawnPoint* mSpawnPoint;

	public:
		MobListener(EnemySpawnPoint* sp) : mSpawnPoint(sp) {}

		virtual void onKilled(CombatEntity* entity)
		{
			mSpawnPoint->mSpawnedEnemies--;
		}
	};

	std::unique_ptr<MobListener> mMobListener;

public:
	EnemySpawnPoint(const glm::vec3& pos) : mPosition(pos), mNextSpawnTime(0), mSpawnedEnemies(0) { mMobListener.reset(new MobListener(this)); }

	const glm::vec3& getPosition() const { return mPosition; }

	bool shouldSpawn()
	{
		if (mSpawnedEnemies > 0) { return false; }
		return mNextSpawnTime <= Tools::currentTimeMillis();
	}

	Enemy* getEnemy()
	{
		Enemy* newEnemy = new Enemy(mPosition);
		newEnemy->addListener(mMobListener.get());
		
		mNextSpawnTime = Tools::currentTimeMillis() + 10000;
		mSpawnedEnemies++;

		return newEnemy;
	}

	void reset()
	{
		mSpawnedEnemies = 0;
		mNextSpawnTime = 0;
	}
};

std::vector<std::unique_ptr<EnemySpawnPoint>> spawnPoints;

class EnemyGiveExpListener : public ICombatEntityListener
{
public:
	virtual void onKilled(CombatEntity* entity)
	{
		playerGainEXP(2);
		// TODO: properly should know which skill killed and give exp accordingly
		for (unsigned int i = 0; i < playerEquipmentItems.size(); i++)
		{
			playerSkillGainExp(playerEquipmentItems[i], 2);
		}
	}
};

std::unique_ptr<ICombatEntityListener> enemyGiveExpListener;

#pragma endregion

#pragma region Item Information

class ItemInfo
{
public:
	virtual std::string getName() = 0;
	virtual std::string getDescription() { return "No description."; }
	virtual void drawIcon() = 0;
	virtual void onUse() {}
};

class Item_BasicRaycaster : public ItemInfo
{
public:
	virtual std::string getName() { return "Basic Raycaster"; }
	virtual void drawIcon()
	{
		glColor3f(BYTE_TO_FLOAT_COLOR(112), BYTE_TO_FLOAT_COLOR(112), BYTE_TO_FLOAT_COLOR(225));
		drawQuad2D(11, 15, 8, 19);
		drawQuad2D(19, 15, 19, 8);
		drawQuad2D(19, 25, 3, 3);
	}
};

class Item_PowerRaycaster : public ItemInfo
{
public:
	virtual std::string getName() { return "Power Raycaster"; }
	virtual void drawIcon()
	{
		glColor3f(BYTE_TO_FLOAT_COLOR(25), BYTE_TO_FLOAT_COLOR(255), BYTE_TO_FLOAT_COLOR(201));
		drawQuad2D(11, 15, 8, 19);
		drawQuad2D(19, 15, 19, 8);
		drawQuad2D(19, 25, 3, 3);
	}
};

class Item_BlastRaycaster : public ItemInfo
{
public:
	virtual std::string getName() { return "Blast Raycaster"; }
	virtual void drawIcon()
	{
		glColor3f(BYTE_TO_FLOAT_COLOR(255), BYTE_TO_FLOAT_COLOR(79), BYTE_TO_FLOAT_COLOR(56));
		drawQuad2D(11, 15, 8, 19);
		drawQuad2D(19, 15, 19, 8);
		drawQuad2D(19, 25, 3, 3);
	}
};

class Item_BasicCharger : public ItemInfo
{
public:
	virtual std::string getName() { return "Basic Charger"; }
	virtual void drawIcon()
	{
		glColor3f(BYTE_TO_FLOAT_COLOR(112), BYTE_TO_FLOAT_COLOR(112), BYTE_TO_FLOAT_COLOR(225));
		drawQuad2D(11, 15, 8, 19);
		drawQuad2D(19, 15, 19, 8);
		drawQuad2D(19, 25, 3, 3);
		drawQuad2D(29, 23, 9, 11);
		drawQuad2D(19, 31, 10, 3);
	}
};

class Item_PowerCharger : public ItemInfo
{
public:
	virtual std::string getName() { return "Power Charger"; }
	virtual void drawIcon()
	{
		glColor3f(BYTE_TO_FLOAT_COLOR(25), BYTE_TO_FLOAT_COLOR(255), BYTE_TO_FLOAT_COLOR(201));
		drawQuad2D(11, 15, 8, 19);
		drawQuad2D(19, 15, 19, 8);
		drawQuad2D(19, 25, 3, 3);
		drawQuad2D(29, 23, 9, 11);
		drawQuad2D(19, 31, 10, 3);
	}
};

class Item_BlastCharger : public ItemInfo
{
public:
	virtual std::string getName() { return "Blast Charger"; }
	virtual void drawIcon()
	{
		glColor3f(BYTE_TO_FLOAT_COLOR(255), BYTE_TO_FLOAT_COLOR(79), BYTE_TO_FLOAT_COLOR(56));
		drawQuad2D(11, 15, 8, 19);
		drawQuad2D(19, 15, 19, 8);
		drawQuad2D(19, 25, 3, 3);
		drawQuad2D(29, 23, 9, 11);
		drawQuad2D(19, 31, 10, 3);
	}
};

class Item_RedPotion : public ItemInfo
{
public:
	virtual std::string getName() { return "Red Potion"; }
	virtual std::string getDescription() { return "Restores 10 HP."; }
	virtual void drawIcon()
	{
		glColor3f(BYTE_TO_FLOAT_COLOR(131), BYTE_TO_FLOAT_COLOR(138), BYTE_TO_FLOAT_COLOR(142));
		drawQuad2D(12, 12, 24, 24);
		drawQuad2D(18, 10, 12, 2);
		glColor3f(BYTE_TO_FLOAT_COLOR(77), BYTE_TO_FLOAT_COLOR(81), BYTE_TO_FLOAT_COLOR(84));
		drawQuad2D(18, 8, 12, 2);
		glColor3f(BYTE_TO_FLOAT_COLOR(173), BYTE_TO_FLOAT_COLOR(183), BYTE_TO_FLOAT_COLOR(188));
		drawQuad2D(20, 12, 8, 2);
		drawQuad2D(14, 14, 20, 2);
		glColor3f(BYTE_TO_FLOAT_COLOR(142), BYTE_TO_FLOAT_COLOR(42), BYTE_TO_FLOAT_COLOR(54));
		drawQuad2D(14, 16, 20, 18);
	}
	virtual void onUse()
	{
		playerHP = std::min(playerHP + 10, playerMaxHP);
	}
};

class Item_BluePotion : public ItemInfo
{
public:
	virtual std::string getName() { return "Blue Potion"; }
	virtual std::string getDescription() { return "Restores 10 MP."; }
	virtual void drawIcon()
	{
		glColor3f(BYTE_TO_FLOAT_COLOR(131), BYTE_TO_FLOAT_COLOR(138), BYTE_TO_FLOAT_COLOR(142));
		drawQuad2D(12, 12, 24, 24);
		drawQuad2D(18, 10, 12, 2);
		glColor3f(BYTE_TO_FLOAT_COLOR(77), BYTE_TO_FLOAT_COLOR(81), BYTE_TO_FLOAT_COLOR(84));
		drawQuad2D(18, 8, 12, 2);
		glColor3f(BYTE_TO_FLOAT_COLOR(173), BYTE_TO_FLOAT_COLOR(183), BYTE_TO_FLOAT_COLOR(188));
		drawQuad2D(20, 12, 8, 2);
		drawQuad2D(14, 14, 20, 2);
		glColor3f(BYTE_TO_FLOAT_COLOR(50), BYTE_TO_FLOAT_COLOR(80), BYTE_TO_FLOAT_COLOR(140));
		drawQuad2D(14, 16, 20, 18);
	}
	virtual void onUse()
	{
		playerMP = std::min(playerMP + 10, playerMaxMP);
	}
};

std::unordered_map<int, std::unique_ptr<ItemInfo>> registeredItems;

void registerItem(int id, ItemInfo* info) { registeredItems[id].reset(info); }
ItemInfo* getItemInfo(int id) { return registeredItems.count(id) > 0 ? registeredItems[id].get() : 0; }

#pragma endregion

#pragma region User Interface

glm::ivec2 currentMousePos;

std::string to_string(const glm::vec3& v) { return std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z); }
std::string to_string(const glm::ivec3& v) { return std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z); }

int calcProgressWidth(int val, int valMax, int fullSize) { return (int)(((float)val / (float)valMax) * (float)fullSize); }

#pragma region Information History Area

class InformationHistoryEntry
{
private:
	long long mExpireTime;
	std::string mMessage;

public:
	InformationHistoryEntry(const std::string& msg) : mExpireTime(Tools::currentTimeMillis() + 5000), mMessage(msg) {}

	const std::string& getMessage() const { return mMessage; }

	bool expired() { return Tools::currentTimeMillis() > mExpireTime; }

	int getRemainingTime() { return (int)(mExpireTime - Tools::currentTimeMillis()); }
};

std::deque<std::unique_ptr<InformationHistoryEntry>> informationHistory;

void addInformationHistory(const std::string& msg) { informationHistory.push_back(std::unique_ptr<InformationHistoryEntry>(new InformationHistoryEntry(msg))); }
void updateInformationHistory()
{
	if (informationHistory.empty()) { return; }

	if (informationHistory.front()->expired()) { informationHistory.pop_front(); }

	for (unsigned int i = 0; i < informationHistory.size(); i++)
	{
		InformationHistoryEntry* entry = informationHistory.at(i).get();
		int remainTime = entry->getRemainingTime();
		glColor4f(0.0f, 0.0f, 0.0f, remainTime > 1000 ? 1.0f : glm::mix(0.1f, 1.0f, remainTime / 1000.0f));
		renderString(windowWidth - 300, 700 - (i * 20), GLUT_BITMAP_HELVETICA_18, entry->getMessage());
	}
}

#pragma endregion

#pragma region UI Window Core

class UIWindow
{
private:
	glm::ivec2 mPosition;
	glm::ivec2 mSize;
	std::string mTitle;
	bool mVisible = false;
	glm::ivec2 mClientAreaOffset = glm::ivec2(5, 25);

protected:
	void quad(int x, int y, int width, int height) { drawQuad2D(x, y, width, height); }
	void text(int x, int y, void* font, const std::string& str) { renderString(mPosition.x + mClientAreaOffset.x + x, mPosition.y + mClientAreaOffset.y + y, font, str); }

	void pushTransformMatrix()
	{
		glPushMatrix();
		glTranslatef((float)(mPosition.x + mClientAreaOffset.x), (float)(mPosition.y + mClientAreaOffset.y), 0.0f);
	}

	void popTransformMatrix()
	{
		glPopMatrix();
	}

	virtual void draw() = 0;
	virtual void click(int x, int y) {}
	virtual void mouseMove(int x, int y) {}

public:
	UIWindow(const glm::ivec2& pos, const glm::ivec2& size, const std::string& title) : mPosition(pos), mSize(size), mTitle(title) {}

	const glm::ivec2& getPosition() const { return mPosition; }
	void setPosition(const glm::ivec2& pos) { mPosition = pos; }
	const glm::ivec2& getSize() const { return mSize; }
	const std::string& getTitle() const { return mTitle; }
	const bool& getVisible() const { return mVisible; }
	void setVisible(bool visible) { mVisible = visible; }

	void onDraw()
	{
		if (!mVisible) { return; }

		glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
		drawQuad2D(mPosition.x, mPosition.y, mSize.x, mSize.y);
		glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
		drawQuad2D(mPosition.x + 5, mPosition.y + 5, mSize.x - 10, mSize.y - 10);
		drawQuad2D(mPosition.x + 10, mPosition.y + 7, mSize.x - 40, 20); // title bar bg
		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		renderString(mPosition.x + 5 + (mSize.x / 2) - (getStringWidth(GLUT_BITMAP_HELVETICA_18, mTitle) / 2), mPosition.y + 5 + 20, GLUT_BITMAP_HELVETICA_18, mTitle);

		// x button
		glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
		drawQuad2D(mPosition.x + mSize.x - 26, mPosition.y + 7, 19, 20);
		glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
		renderString(mPosition.x + mSize.x - 21, mPosition.y + 7 + 15, GLUT_BITMAP_HELVETICA_18, "x");

		pushTransformMatrix();
		draw();
		popTransformMatrix();
	}

	bool onClick(int x, int y)
	{
		if (!mVisible) { return false; }

		if (x >= mPosition.x && x <= mPosition.x + mSize.x && y >= mPosition.y && y <= mPosition.y + mSize.y)
		{
			// toggle visiblity on x button press
			glm::ivec2 xbPos(mPosition.x + mSize.x - 26, mPosition.y + 7);
			if (x >= xbPos.x && y >= xbPos.y && x <= xbPos.x + 19 && y <= xbPos.y + 20) { mVisible = false; }
			else { click(x - mPosition.x - mClientAreaOffset.x, y - mPosition.y - mClientAreaOffset.y); }

			return true;
		}

		return false;
	}

	void onMouseMove()
	{
		if (!mVisible) { return; }

		mouseMove(currentMousePos.x - mPosition.x - mClientAreaOffset.x, currentMousePos.y - mPosition.y - mClientAreaOffset.y);
	}
};

std::vector<std::unique_ptr<UIWindow>> uiWindows;

void addUIWindow(UIWindow* window) { uiWindows.push_back(std::unique_ptr<UIWindow>(window)); }

UIWindow* getUIWindowByTitle(const std::string& title)
{
	UIWindow* ret = 0;
	for (auto& window : uiWindows)
	{
		if (window->getTitle() == title)
		{
			ret = window.get();
			break;
		}
	}
	return ret;
}

#pragma endregion

class InventoryWindow : public UIWindow
{
protected:
	virtual void draw()
	{
		for (short i = 0; i < playerInventoryItems.getSlotLimit(); i++)
		{
			int row = i / 6;
			int col = i % 6;

			short slot = i + 1;
			Item* item = playerInventoryItems.getItem(slot);

			glColor4f(0.0f, 0.0f, 0.0f, item ? 0.75f : 0.25f);
			drawQuad2D(5 + (col * 52), 5 + (row * 52), 48, 48);
			if (item)
			{
				glColor4f(1.0f, 1.0f, 1.0f, 0.25f);
				text(5 + (col * 52), 5 + (row * 52) + 13, GLUT_BITMAP_8_BY_13, std::to_string(item->getItemId() / 1000000) + " - " + std::to_string(item->getItemId() % 1000000)); // debug

				// show quantity for non-equips
				if (item->getInventoryType() != InventoryType::EQUIP)
				{
					glColor3f(1.0f, 1.0f, 1.0f);
					std::string quantityStr = std::to_string(item->getQuantity());
					text(5 + (col * 52) + 45 - getStringWidth(GLUT_BITMAP_HELVETICA_12, quantityStr), 5 + (row * 52) + 45, GLUT_BITMAP_HELVETICA_12, quantityStr);
				}

				// draw icon if loaded
				ItemInfo* info = getItemInfo(item->getItemId());
				if (info)
				{
					glPushMatrix();
					glTranslatef((float)(5 + (col * 52)), (float)(5 + (row * 52)), 0.0f);
					info->drawIcon();
					glPopMatrix();
				}
			}
		}
	}

	virtual void click(int x, int y)
	{
		glm::ivec2 curPos(x, y);

		for (short i = 0; i < playerInventoryItems.getSlotLimit(); i++)
		{
			int row = i / 6;
			int col = i % 6;
			glm::ivec2 low(5 + (col * 52), 5 + (row * 52));
			glm::ivec2 high(low.x + 48, low.y + 48);

			if (curPos.x >= low.x && curPos.y >= low.y && curPos.x <= high.x && curPos.y <= high.y)
			{
				short slot = i + 1;

				addInformationHistory("Click on inventory slot " + std::to_string(slot));

				Item* item = playerInventoryItems.getItem(slot);
				if (item)
				{
					InventoryType type = item->getInventoryType();

					if (type == InventoryType::EQUIP)
					{
						playerEquipmentItems.push_back(item->getItemId());

						// TODO: dont add skill if already known!
						// TODO: should be skill id taught by item with given item id
						playerSkills.push_back(std::unique_ptr<LearnedSkill>(new LearnedSkill(item->getItemId(), 1, 0)));
						addInformationHistory("Learned skill (" + loadedSkills[item->getItemId()]->getName() + ")");

						playerInventoryItems.removeItem(slot);
					}
					else if (type == InventoryType::USE)
					{
						getItemInfo(item->getItemId())->onUse();
						playerInventoryItems.removeItem(slot);
					}
				}
				break;
			}
		}
	}

	virtual void mouseMove(int x, int y)
	{
		glm::ivec2 curPos(x, y);

		for (short i = 0; i < playerInventoryItems.getSlotLimit(); i++)
		{
			int row = i / 6;
			int col = i % 6;
			glm::ivec2 low(5 + (col * 52), 5 + (row * 52));
			glm::ivec2 high(low.x + 48, low.y + 48);

			if (curPos.x >= low.x && curPos.y >= low.y && curPos.x <= high.x && curPos.y <= high.y)
			{
				short slot = i + 1;
				Item* item = playerInventoryItems.getItem(slot);
				if (item)
				{
					pushTransformMatrix();
					glColor4f(0.25f, 0.25f, 0.25f, 0.9f);
					quad(curPos.x + 20, curPos.y + 20, 150, 80);
					glColor3f(1.0f, 1.0f, 1.0f);
					ItemInfo* info = getItemInfo(item->getItemId());
					if (info)
					{
						text(curPos.x + 23, curPos.y + 23 + 18, GLUT_BITMAP_HELVETICA_18, info->getName());
						text(curPos.x + 23, curPos.y + 23 + 18 + 3 + 12, GLUT_BITMAP_HELVETICA_12, info->getDescription());
					}
					else
					{
						text(curPos.x + 23, curPos.y + 23 + 12, GLUT_BITMAP_HELVETICA_12, "Item information not loaded.");
					}
					popTransformMatrix();
				}
				break;
			}
		}
	}

public:
	InventoryWindow() : UIWindow(glm::ivec2(95, 95), glm::ivec2(330, 345), "Inventory") {}
};

class EquipmentWindow : public UIWindow
{
protected:
	virtual void draw()
	{
		for (unsigned int i = 0; i < playerEquipmentItems.size(); i++)
		{
			int row = i / 6;
			int col = i % 6;
			glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
			quad(5 + (col * 29), 5 + (row * 29), 25, 25);
			glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
			text(5 + (col * 29), 25 + (row * 29), GLUT_BITMAP_HELVETICA_18, std::to_string(playerEquipmentItems[i]));
		}
	}

	virtual void click(int x, int y)
	{
		glm::vec2 curPos(x, y);

		for (unsigned int i = 0; i < playerEquipmentItems.size(); i++)
		{
			int row = i / 6;
			int col = i % 6;
			glm::vec2 low(5 + (col * 29), 5 + (row * 29));
			glm::vec2 high(low.x + 25, low.y + 25);

			if (curPos.x >= low.x && curPos.y >= low.y && curPos.x <= high.x && curPos.y <= high.y)
			{
				addInformationHistory("Click on equipment item " + std::to_string(i));
				playerInventoryItems.addItem(new Item(playerEquipmentItems[i]));
				playerEquipmentItems.erase(playerEquipmentItems.begin() + i);
				break;
			}
		}
	}

public:
	EquipmentWindow() : UIWindow(glm::ivec2(495, 95), glm::ivec2(195, 340), "Equipment") {}
};

class SkillsWindow : public UIWindow
{
protected:
	virtual void draw()
	{
		for (unsigned int i = 0; i < playerSkills.size(); i++)
		{
			LearnedSkill* sInfo = playerSkills[i].get();

			int y = 5 + (i * 29);

			glColor4f(0.0f, 1.0f, 0.0f, 0.2f);
			quad(5, y, 375, 25);
			glColor4f(0.0f, 1.0f, 0.0f, 0.8f);
			quad(5, y, calcProgressWidth(sInfo->getExp(), sInfo->getLevelUpExp(), 375), 25);
			glColor3f(0.0f, 0.0f, 0.0f);
			text(5, y + 20, GLUT_BITMAP_HELVETICA_18, loadedSkills[sInfo->getId()]->getName());
			text(225, y + 20, GLUT_BITMAP_HELVETICA_18, "Lv. " + std::to_string(sInfo->getLevel()) + " (" + std::to_string(sInfo->getExp()) + " / " + std::to_string(sInfo->getLevelUpExp()) + ")");
		}
	}

public:
	SkillsWindow() : UIWindow(glm::ivec2(795, 95), glm::ivec2(395, 340), "Skills") {}
};

#pragma region Shops

std::vector<int> shopItems;

class ShopWindow : public UIWindow
{
protected:
	virtual void draw()
	{
		for (unsigned int i = 0; i < shopItems.size(); i++)
		{
			glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
			quad(5, 5 + (i * 29), 175, 25);
			glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
			text(5, 25 + (i * 29), GLUT_BITMAP_HELVETICA_18, "item id " + std::to_string(shopItems[i]));
		}
	}

	virtual void click(int x, int y)
	{
		glm::vec2 curPos(x, y);

		for (unsigned int i = 0; i < shopItems.size(); i++)
		{
			glm::vec2 low(5, 5 + (i * 29));
			glm::vec2 high(low.x + 175, low.y + 25);

			if (curPos.x >= low.x && curPos.y >= low.y && curPos.x <= high.x && curPos.y <= high.y)
			{
				addInformationHistory("Click on shop item " + std::to_string(i));
				playerInventoryItems.addItem(new Item(shopItems[i]));
				break;
			}
		}
	}

public:
	ShopWindow() : UIWindow(glm::ivec2(395, 95), glm::ivec2(195, 340), "Shop") {}
};

void addShopItem(int id) { shopItems.push_back(id); }

void clearShopItems() { shopItems.clear(); }

#pragma endregion

#pragma region Dialogues

class IDialogueWindow
{
public:
	virtual std::string getMessage() = 0;

	virtual void onOk() {}
	virtual void onCancel() {}
};

bool dialogueVisible = false;
std::unique_ptr<IDialogueWindow> activeDialogueWindow;

void showDialogueWindow(IDialogueWindow* dlg)
{
	activeDialogueWindow.reset(dlg);
	dialogueVisible = true;
}

void drawDialogueWindow()
{
	if (!dialogueVisible) { return; }

	glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
	drawQuad2D(395, 95, 410, 340);
	glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
	drawQuad2D(400, 100, 400, 330);
	glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
	renderString(475, 120, GLUT_BITMAP_HELVETICA_18, "Dialogue");

	renderString(405, 150, GLUT_BITMAP_HELVETICA_18, activeDialogueWindow->getMessage());

	// buttons
	glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
	drawQuad2D(500, 405, 100, 20);
	glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
	renderString(540, 425, GLUT_BITMAP_HELVETICA_18, "OK");

	glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
	drawQuad2D(610, 405, 100, 20);
	glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
	renderString(625, 425, GLUT_BITMAP_HELVETICA_18, "Cancel");
}

void onDialogueWindowClick(int x, int y)
{
	if (!dialogueVisible) { return; }

	glm::vec2 curPos(x, y);

	// buttons
	if (curPos.x >= 500 && curPos.y >= 405 && curPos.x <= 600 && curPos.y <= 425)
	{
		dialogueVisible = false;
		activeDialogueWindow->onOk();
		return;
	}
	else if (curPos.x >= 610 && curPos.y >= 405 && curPos.x <= 710 && curPos.y <= 425)
	{
		dialogueVisible = false;
		activeDialogueWindow->onCancel();
		return;
	}
}

#pragma endregion

#pragma region Keybinding

struct Keybinding
{
	int keycode;

	glm::ivec2 position;
	glm::ivec2 size;
	std::string text;

	enum class Type
	{
		INTERNAL,
		SKILL,
		ITEM
	};

	Type type;
	LearnedSkill* skill;
	Item* item;
};
std::unordered_map<int, Keybinding> keybinds;

class KeybindingWindow : public UIWindow
{
protected:
	virtual void draw()
	{
		for (auto& keybind : keybinds)
		{
			glColor4f(0.0f, 0.0f, 0.0f, 0.75f);
			quad(keybind.second.position.x, keybind.second.position.y, keybind.second.size.x, keybind.second.size.y);
			glColor4f(1.0f, 1.0f, 1.0f, 0.75f);
			text(5 + keybind.second.position.x, 25 + keybind.second.position.y, GLUT_BITMAP_9_BY_15, std::to_string(keybind.second.keycode));
		}
	}

	virtual void click(int x, int y)
	{
		glm::vec2 curPos(x, y);

		for (auto& keybind : keybinds)
		{
			glm::vec2 low(keybind.second.position.x, keybind.second.position.y);
			glm::vec2 high(low.x + keybind.second.size.x, low.y + keybind.second.size.y);

			if (curPos.x >= low.x && curPos.y >= low.y && curPos.x <= high.x && curPos.y <= high.y)
			{
				addInformationHistory("Click on keybind " + std::to_string(keybind.second.keycode));
				break;
			}
		}
	}

public:
	KeybindingWindow() : UIWindow(glm::ivec2(200, 400), glm::ivec2(700, 300), "Keybinding") {}
};

void initKeybindings()
{
	keybinds[0].keycode = 0;
	keybinds[0].position = glm::ivec2(10, 10);
	keybinds[0].size = glm::ivec2(32, 32);
	keybinds[0].text = "test";
}

void registerKeybinding(int keycode, Keybinding::Type actionType, LearnedSkill* skill, Item* item) {}

#pragma endregion

#pragma endregion

#pragma region Attacking

void damageEnemy(Enemy* hit)
{
	int damage = randomNumber(1, 3);
	if (damage >= hit->getHP())
	{
		hit->onKilled();
		for (auto it = enemies.begin(); it != enemies.end(); it++) { if (it->get() == hit) { enemies.erase(it); break; } }
	}
	else { hit->setHP(hit->getHP() - damage); }
}

class ISkillEffect
{
public:
	virtual void update(float elapsed) = 0;
	virtual void draw() = 0;
	virtual bool completed() = 0;
};

std::vector<std::unique_ptr<ISkillEffect>> skillEffects;

class RaycasterFire : public ISkillEffect
{
private:
	glm::vec3 mStartPoint;
	glm::vec3 mEndPoint;
	float mTimeDisplayed = 0.0f;

public:
	RaycasterFire(const glm::vec3& start, const glm::vec3& end) : mStartPoint(start), mEndPoint(end) {}

	virtual void update(float elapsed)
	{
		mTimeDisplayed += elapsed;
	}

	virtual void draw()
	{
		// calculate ray alpha based on elapsed time
		float alpha = mTimeDisplayed * 2;
		if (mTimeDisplayed > 0.5f && mTimeDisplayed <= 1.0f) { alpha = 1; }
		else if (mTimeDisplayed > 1.0f) { alpha = glm::mix(1.0f, 0.0f, mTimeDisplayed - 1.0f); }
		alpha = std::max(alpha, 0.0f);

		// draw ray
		glLineWidth(10.0f);
		glColor4f(BYTE_TO_FLOAT_COLOR(225), BYTE_TO_FLOAT_COLOR(144), BYTE_TO_FLOAT_COLOR(255), alpha);
		glBegin(GL_LINES);
		glVertex3f(mStartPoint.x, mStartPoint.y, mStartPoint.z);
		glVertex3f(mEndPoint.x, mEndPoint.y, mEndPoint.z);
		glEnd();
		glLineWidth(1.0f);
	}

	virtual bool completed() { return mTimeDisplayed >= 2.0f; }
};

struct RaycasterShotHit
{
	Enemy* hit;
	glm::vec3 collisionPos;

	RaycasterShotHit(Enemy* _hit, const glm::vec3& pos) : hit(_hit), collisionPos(pos) {}
};

void shootRaycaster(int mode)
{
	// validate
	if (playerMP < mode) { printf("Not enough MP to shoot Raycaster at mode %d!\n", mode); return; }

	printf("Raycaster shot (mode %d)!\n", mode);
	playerMP -= mode;

	// calculate weapon ray points
	glm::vec3 rayStart(cx, 1.0f, cz);
	glm::vec3 rayEnd(lx, 0.0f, lz);
	rayEnd *= mode == 1 ? 150.0f : 50.0f;
	rayEnd += rayStart;

	// add visual
	skillEffects.push_back(std::unique_ptr<ISkillEffect>(new RaycasterFire(rayStart, rayEnd)));

	// check all enemies for hits
	std::vector<std::unique_ptr<RaycasterShotHit>> hits;

	for (auto& enemy : enemies)
	{
		// gen aabb
		AxisAlignedBoundingBox aabb = enemy->getAxisAlignedBoundingBox();

		// check collision
		glm::vec3 hitPos;

		if (CheckLineBox(aabb.getLowerBound(), aabb.getHigherBound(), rayStart, rayEnd, hitPos))
		{
			if (hits.empty() || mode == 2) { hits.push_back(std::unique_ptr<RaycasterShotHit>(new RaycasterShotHit(enemy.get(), hitPos))); }
			else if (glm::distance(rayStart, hitPos) < glm::distance(rayStart, hits[0]->collisionPos))
			{
				hits[0]->hit = enemy.get();
				hits[0]->collisionPos = hitPos;
			}
		}
	}

	// apply damage to hit enemies
	for (auto& hit : hits)
	{
		printf("Raycaster hit enemy! (%s)\n", to_string(hit->hit->getPosition()).c_str());
		damageEnemy(hit->hit);
	}

	// apply recoil
	deltaAngle += randomNumber(-10, 10) * 0.001f;
	deltaAngleY -= randomNumber(-10, 2) * 0.001f;
}

bool shootingRaycaster = false;
long long lastRaycasterShotTime = 0;

void updateRaycasterAutomaticFire()
{
	if (shootingRaycaster)
	{
		if (Tools::currentTimeMillis() - lastRaycasterShotTime > 200)
		{
			shootRaycaster(0);
			lastRaycasterShotTime = Tools::currentTimeMillis();
		}
	}
}

class RaycasterBasicAttackSkill : public ICombatSkill
{
public:
	RaycasterBasicAttackSkill() : ICombatSkill("Raycast Mastery") {}

	virtual void attemptCast() { shootRaycaster(0); }
};

class RaycasterPowerShotSkill : public ICombatSkill
{
public:
	RaycasterPowerShotSkill() : ICombatSkill("Raycast Power Shot") {}

	virtual void attemptCast() { shootRaycaster(1); }
};

class RaycasterBlastShotSkill : public ICombatSkill
{
public:
	RaycasterBlastShotSkill() : ICombatSkill("Raycast Blast Shot") {}

	virtual void attemptCast() { shootRaycaster(2); }
};

class ChargeDash : public ISkillEffect
{
private:
	glm::vec3 mStartPoint;
	glm::vec3 mEndPoint;
	float mTimeDisplayed = 0.0f;
	int mMode;
	std::unordered_set<Enemy*> mHitEnemies;

public:
	ChargeDash(int mode) : mMode(mode) {}

	virtual void update(float elapsed)
	{
		mTimeDisplayed += elapsed;

		mStartPoint.x = cx;
		mStartPoint.z = cz;

		PhysicalEnvironment penv;
		penv.preMove(elapsed);
		moveCamera(1, elapsed * 45.0f);
		penv.postMove();

		mEndPoint.x = cx;
		mEndPoint.z = cz;

		if (mMode > 1)
		{
			playerEntity->setPosition(glm::vec3(cx, cy, cz));
			AxisAlignedBoundingBox playerAabb = playerEntity->getAxisAlignedBoundingBox();

			// check all enemies for hits
			std::vector<Enemy*> hits;

			for (auto& enemy : enemies)
			{
				if (mHitEnemies.count(enemy.get()) == 0 && playerAabb.intersects(enemy->getAxisAlignedBoundingBox()))
				{
					hits.push_back(enemy.get());
				}
			}

			// apply damage to hit enemies
			for (auto& hit : hits)
			{
				printf("Charge dash hit enemy! (%s)\n", to_string(hit->getPosition()).c_str());
				damageEnemy(hit);
			}
		}
	}

	virtual void draw()
	{
		// calculate ray alpha based on elapsed time
		float alpha = mTimeDisplayed * 2;
		if (mTimeDisplayed > 0.5f && mTimeDisplayed <= 1.0f) { alpha = 1; }
		else if (mTimeDisplayed > 1.0f) { alpha = glm::mix(1.0f, 0.0f, mTimeDisplayed - 1.0f); }
		alpha = std::max(alpha, 0.0f);

		// draw ray
		glLineWidth(10.0f);
		glColor4f(BYTE_TO_FLOAT_COLOR(146), BYTE_TO_FLOAT_COLOR(144), BYTE_TO_FLOAT_COLOR(56), alpha);
		glBegin(GL_LINES);
		glVertex3f(mStartPoint.x, mStartPoint.y, mStartPoint.z);
		glVertex3f(mEndPoint.x, mEndPoint.y, mEndPoint.z);
		glEnd();
		glLineWidth(1.0f);
	}

	virtual bool completed() { return mTimeDisplayed >= 0.5f; }
};

void startChargeDash(int mode)
{
	// validate
	if (playerMP < mode) { printf("Not enough MP to charge dash at mode %d!\n", mode); return; }

	printf("Charge dash started at (%s) (mode %d)\n", to_string(glm::vec3(cx, cy, cz)).c_str(), mode);
	playerMP -= mode;

	skillEffects.push_back(std::unique_ptr<ISkillEffect>(new ChargeDash(mode)));
}

class ChargeDashBasicSkill : public ICombatSkill
{
public:
	ChargeDashBasicSkill() : ICombatSkill("Charge Dash (Basic)") {}

	virtual void attemptCast() { startChargeDash(1); }
};

class ChargeDashRushSkill : public ICombatSkill
{
public:
	ChargeDashRushSkill() : ICombatSkill("Charge Dash (Rush)") {}

	virtual void attemptCast() { startChargeDash(2); }
};

class ChargeDashSweepSkill : public ICombatSkill
{
public:
	ChargeDashSweepSkill() : ICombatSkill("Charge Dash (Sweep)") {}

	virtual void attemptCast() { startChargeDash(3); }
};

#pragma endregion

#pragma region NPC Interface

class INPC
{
public:
	virtual void onClick() = 0;
};

class TraderNPC : public INPC
{
public:
	virtual void onClick()
	{
		addInformationHistory("Clicked on trader");
		clearShopItems();
		for (int i = 1; i <= 10; i++) { addShopItem(i); }
		getUIWindowByTitle("Shop")->setVisible(true);
	}
};

std::unordered_map<int, std::unique_ptr<INPC>> registeredNPCs;

void registerNPC(int id, INPC* iface) { registeredNPCs[id].reset(iface); }

struct LoadedNPC
{
	int id;
	glm::vec3 position;

	LoadedNPC(int _id, const glm::vec3& _pos) : id(_id), position(_pos) {}

	void draw()
	{
		glColor3f(0.0f, 0.0f, 1.0f);
		glPushMatrix();
		glTranslatef(position.x, position.y + 1, position.z);
		glutWireCube(2.0f);
		glPopMatrix();
	}
};

std::vector<std::unique_ptr<LoadedNPC>> loadedNPCs;

void loadNPC(int id, const glm::vec3& pos) { loadedNPCs.push_back(std::unique_ptr<LoadedNPC>(new LoadedNPC(id, pos))); }

void unloadNPC(int id)
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

#pragma endregion

#pragma region Terrain Generation

// Improved Perlin Noise. https://cs.nyu.edu/~perlin/noise/
class PerlinNoise
{
public:
	PerlinNoise()
	{
		int permutation[] = { 151,160,137,91,90,15,
			131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
			190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
			88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
			77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
			102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
			135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
			5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
			223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
			129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
			251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
			49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
			138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
		};

		for (int i = 0; i < 256; i++) p[256 + i] = p[i] = permutation[i];
	}

	double noise(double x, double y, double z) {
		int X = (int)std::floor(x) & 255,                  // FIND UNIT CUBE THAT
			Y = (int)std::floor(y) & 255,                  // CONTAINS POINT.
			Z = (int)std::floor(z) & 255;
		x -= std::floor(x);                                // FIND RELATIVE X,Y,Z
		y -= std::floor(y);                                // OF POINT IN CUBE.
		z -= std::floor(z);
		double u = fade(x),                                // COMPUTE FADE CURVES
			v = fade(y),                                // FOR EACH OF X,Y,Z.
			w = fade(z);
		int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z,      // HASH COORDINATES OF
			B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;      // THE 8 CUBE CORNERS,

		return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),  // AND ADD
			grad(p[BA], x - 1, y, z)), // BLENDED
			lerp(u, grad(p[AB], x, y - 1, z),  // RESULTS
				grad(p[BB], x - 1, y - 1, z))),// FROM  8
			lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),  // CORNERS
				grad(p[BA + 1], x - 1, y, z - 1)), // OF CUBE
				lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
					grad(p[BB + 1], x - 1, y - 1, z - 1))));
	}

private:
	static double fade(double t) { return t * t * t * (t * (t * 6 - 15) + 10); }
	static double lerp(double t, double a, double b) { return a + t * (b - a); }
	static double grad(int hash, double x, double y, double z) {
		int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
		double u = h < 8 ? x : y,                 // INTO 12 GRADIENT DIRECTIONS.
			v = h < 4 ? y : h == 12 || h == 14 ? x : z;
		return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
	}

	int p[512];
};

void setTree(int x, int y, int z)
{
	int height = getRandomInt(4, 8);
	glm::ivec3 shape(getRandomInt(2, 7), getRandomInt(1, height / 2), getRandomInt(2, 7));
	for (int i = 0; i < height; i++) { setVoxel(x, y + i, z, 137, getRandomInt(30, 90), 0); } // trunk
	for (int xx = x - shape.x; xx < x + shape.x; xx++)
	{
		for (int yy = y - shape.y; yy < y + shape.y; yy++)
		{
			for (int zz = z - shape.z; zz < z + shape.z; zz++)
			{
				if (getRandomInt(1, 10) >= 3)
				{
					setVoxel(xx, yy + height + (shape.y / 2), zz, getRandomInt(100, 135), getRandomInt(100, 135), 0);
				}
			}
		}
	}
}

class Dungeon
{
private:
	glm::ivec3 mPosition;
	glm::ivec3 mSize;
	bool mazeWalls[57][57][4];
	glm::ivec2 mazeCurPos;
	int mazeMoveLimit = 300;
	std::vector<glm::ivec2> mazeClearedTiles;

	enum class MazeWall
	{
		FORWARD,
		RIGHT,
		BACKWARD,
		LEFT,
		INVALID
	};

	void mazeInitWalls()
	{
		for (int w = 0; w < 57; w++)
		{
			for (int d = 0; d < 57; d++)
			{
				for (int i = 0; i < 4; i++)
				{
					mazeWalls[w][d][i] = true;
				}
			}
		}
	}

	void mazeRemoveWall(MazeWall dir) { mazeWalls[mazeCurPos.x][mazeCurPos.y][(int)dir] = false; }

	bool mazeCanMove(MazeWall dir)
	{
		return !(dir == MazeWall::FORWARD && mazeCurPos.y + 1 > 56 ||
			dir == MazeWall::RIGHT && mazeCurPos.x + 1 > 56 ||
			dir == MazeWall::BACKWARD && mazeCurPos.y - 1 < 0 ||
			dir == MazeWall::LEFT && mazeCurPos.x - 1 < 0);
	}

	void mazeMoveDirection(MazeWall dir)
	{
		if (dir == MazeWall::FORWARD) { mazeCurPos.y++; }
		else if (dir == MazeWall::RIGHT) { mazeCurPos.x++; }
		else if (dir == MazeWall::BACKWARD) { mazeCurPos.y--; }
		else if (dir == MazeWall::LEFT) { mazeCurPos.x--; }
	}

	MazeWall mazeGetInverseWall(MazeWall dir)
	{
		if (dir == MazeWall::FORWARD) { return MazeWall::BACKWARD; }
		else if (dir == MazeWall::RIGHT) { return MazeWall::LEFT; }
		else if (dir == MazeWall::BACKWARD) { return MazeWall::FORWARD; }
		else if (dir == MazeWall::LEFT) { return MazeWall::RIGHT; }
		else { return MazeWall::INVALID; }
	}

	bool mazeClearPath(MazeWall dir)
	{
		if (!mazeCanMove(dir)) { return false; }

		mazeRemoveWall(dir);
		mazeMoveDirection(dir);
		mazeRemoveWall(mazeGetInverseWall(dir));
		mazeClearedTiles.push_back(mazeCurPos);

		return true;
	}

	void mazeGenerate()
	{
		mazeInitWalls();
		MazeWall lastDirection = MazeWall::INVALID;
		for (int curMoves = 0; curMoves < mazeMoveLimit; curMoves++)
		{
			MazeWall direction = (MazeWall)getRandomInt(0, 3);
			if (lastDirection != MazeWall::INVALID)
			{
				// don't just go forwards and backwards, and don't attempt a move we just confirmed is invalid either
				while (direction == mazeGetInverseWall(lastDirection) || !mazeCanMove(direction))
				{
					if (direction == mazeGetInverseWall(lastDirection))
					{
						printf("%d is inverse %d! regen\n", direction, lastDirection);
					}
					else if (!mazeCanMove(direction))
					{
						printf("can't move in dir %d! regen\n", direction);
					}
					else
					{
						printf("UNKOWN MAZE ERROR!\n");
					}
					direction = (MazeWall)getRandomInt(0, 3);
				}
			}
			int len = getRandomInt(2, 8);

			for (int i = 0; i < len; i++)
			{
				if (!mazeClearPath(direction))
				{
					printf("Hit map bound\n");
					break;
				}
			}

			lastDirection = direction;
			printf("Completed maze move %d\n", curMoves);
		}
	}

	void generateMazeWallVoxels(int baseX, int baseZ, MazeWall wall)
	{
		for (int i = 0; i < 16; i++)
		{
			setVoxel(
				baseX + (wall == MazeWall::FORWARD || wall == MazeWall::BACKWARD ? i : 0) + (wall == MazeWall::RIGHT ? 15 : 0),
				0,
				baseZ + (wall == MazeWall::LEFT || wall == MazeWall::RIGHT ? i : 0) + (wall == MazeWall::FORWARD ? 15 : 0),
				randomNumber(0, 15),
				randomNumber(0, 75),
				randomNumber(0, 15)
			);
		}
	}

	void loadChunks()
	{
		// base terrain floor and edges
		for (int x = mPosition.x; x < mSize.x * 16; x++)
		{
			for (int z = mPosition.z; z < mSize.z * 16; z++)
			{
				setVoxel(x, -1, z, randomNumber(0, 30), randomNumber(100, 255), randomNumber(0, 30));
				//if (x == -200 || x == 200 || z == -200 || z == 200) { setVoxel(x, 0, z, randomNumber(0, 15), randomNumber(0, 75), randomNumber(0, 15)); }
			}
		}

		// generate and apply pathing
		mazeCurPos = glm::ivec2(randomNumber(0, 56), randomNumber(0, 56));
		mazeClearedTiles.push_back(mazeCurPos);
		//cx = (float)mPosition.x + (float)(mazeCurPos.x * 7) + 4;
		//cz = (float)mPosition.z + (float)(mazeCurPos.y * 7) + 4;
		mazeGenerate();

		for (int x = 0; x < mSize.x; x++)
		{
			for (int z = 0; z < mSize.z; z++)
			{
				int baseX = mPosition.x + (x * 16);
				int baseZ = mPosition.z + (z * 16);

				// place walls on voxel terrain for each walled off direction of the cell
				for (int wall = 0; wall < 4; wall++) { if (mazeWalls[x][z][wall]) { generateMazeWallVoxels(baseX, baseZ, (MazeWall)wall); } }

				// fill in the center of completely walled off cells
				if (mazeWalls[x][z][0] && mazeWalls[x][z][1] && mazeWalls[x][z][2] && mazeWalls[x][z][3])
				{
					for (int xx = 0; xx < 14; xx++)
					{
						for (int zz = 0; zz < 14; zz++)
						{
							setVoxel(baseX + 1 + xx, 0, baseZ + 1 + zz, randomNumber(0, 15), randomNumber(0, 75), randomNumber(0, 15));
						}
					}

					// also add a tree in the center
					setTree(baseX + getRandomInt(3, 10), 0, baseZ + getRandomInt(3, 10));
				}
			}
		}

		// load spawnpoints
		for (int i = 0; i < 10; i++)
		{
			glm::ivec2 pos = mazeClearedTiles[randomNumber(3, mazeClearedTiles.size() - 1)];
			spawnPoints.push_back(std::unique_ptr<EnemySpawnPoint>(new EnemySpawnPoint(glm::vec3(mPosition.x + (pos.x * 16) + 3, 0.0f, mPosition.z + (pos.y * 16) + 3))));
		}

		// add wave enter npc
		//loadNPC(2, glm::vec3(cx - 3, 0, cz - 3));
	}

public:
	Dungeon(int x, int z) : mPosition(x, 0, z), mSize(57, 1, 57) { loadChunks(); }

	void drawBorder()
	{
		glm::vec3 pos((float)mPosition.x, (float)mPosition.y, (float)mPosition.z);
		glm::vec3 size((float)mSize.x, (float)mSize.y, (float)mSize.z);
		size *= 16;

		bool inside = isPlayerInside();
		glColor4f(inside ? 1.0f : 0.0f, 0.2f, inside ? 0.0f : 1.0f, 0.75f);

		glBegin(GL_QUADS);

		// front
		glVertex3f(pos.x, 100.0f, pos.z);
		glVertex3f(pos.x + size.x, 100.0f, pos.z);
		glVertex3f(pos.x + size.x, 0.0f, pos.z);
		glVertex3f(pos.x, 0.0f, pos.z);

		// back
		glVertex3f(pos.x, 100.0f, pos.z + size.z);
		glVertex3f(pos.x + size.x, 100.0f, pos.z + size.z);
		glVertex3f(pos.x + size.x, 0.0f, pos.z + size.z);
		glVertex3f(pos.x, 0.0f, pos.z + size.z);

		// left
		glVertex3f(pos.x, 100.0f, pos.z);
		glVertex3f(pos.x, 100.0f, pos.z + size.z);
		glVertex3f(pos.x, 0.0f, pos.z + size.z);
		glVertex3f(pos.x, 0.0f, pos.z);

		// right
		glVertex3f(pos.x + size.x, 100.0f, pos.z);
		glVertex3f(pos.x + size.x, 100.0f, pos.z + size.z);
		glVertex3f(pos.x + size.x, 0.0f, pos.z + size.z);
		glVertex3f(pos.x + size.x, 0.0f, pos.z);

		glEnd();
	}

	bool usesChunk(int x, int y, int z)
	{
		glm::ivec3 chunkStart(getVoxelChunkPos(mPosition.x, mPosition.y, mPosition.z));
		glm::ivec3 chunkEnd(getVoxelChunkPos(mPosition.x + (mSize.x * 16), mPosition.y + (mSize.x * 16), mPosition.z + (mSize.x * 16)));

		if (x >= chunkStart.x && x < chunkEnd.x && z >= chunkStart.z && z < chunkEnd.z) { return true; }
		return false;
	}

	bool usesChunk(const glm::ivec3& pos) { return usesChunk(pos.x, pos.y, pos.z); }

	bool isPlayerInside() { return usesChunk(getVoxelChunkPos(getPlayerPositionVoxelPos())); }
};

#pragma endregion

#pragma region Statistics & Configuration Management

class INIFile
{
private:
	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data;

public:
	int getInt(const std::string& cat, const std::string& name) { return std::stoi(data[cat][name]); }
	float getFloat(const std::string& cat, const std::string& name) { return std::stof(data[cat][name]); }

	void setInt(const std::string& cat, const std::string& name, int val) { data[cat][name] = std::to_string(val); }
	void setFloat(const std::string& cat, const std::string& name, float val) { data[cat][name] = std::to_string(val); }

	void load(const std::string& path)
	{
		std::string currentCategory;

		std::ifstream cfg;
		cfg.open(path);

		while (!cfg.eof())
		{
			std::string line;
			std::getline(cfg, line);

			if (line.length() == 0) { continue; }

			if (line[0] == '[' && line[line.length() - 1] == ']') { currentCategory = line.substr(1, line.length() - 2); }
			else { data[currentCategory][line.substr(0, line.find('='))] = line.substr(line.find('=') + 1); }
		}

		cfg.close();
	}

	void save(const std::string& path)
	{
		std::ofstream cfg;
		cfg.open(path);

		for (auto& cat : data)
		{
			cfg << "[" << cat.first << "]\n";
			for (auto& vals : cat.second)
			{
				cfg << vals.first << "=" << vals.second << "\n";
			}
			cfg << "\n";
		}

		cfg.close();
	}
};

// statistics
int playerDeaths = 0;

void loadConfig()
{
	// check for file
	if (!Tools::FileUtil::exists("settings_.ini"))
	{
		printf("settings.ini not found. no configuration was loaded.\n");

		// TODO: remove! TEMP, TESTING
		playerInventoryItems.addItem(new Item(1000001));
		playerInventoryItems.addItem(new Item(1000002));
		playerInventoryItems.addItem(new Item(1000003));
		playerInventoryItems.addItem(new Item(1000004));
		playerInventoryItems.addItem(new Item(1000005));
		playerInventoryItems.addItem(new Item(1000006));
		playerInventoryItems.addItem(new Item(2000001, 1000));
		playerInventoryItems.addItem(new Item(2000002, 1000));

		return;
	}
	printf("settings.ini found! loading configuration...\n");

	// load from file
	INIFile cfg;
	cfg.load("settings.ini");

	// position
	cx = cfg.getFloat("Player", "x");
	cz = cfg.getFloat("Player", "z");

	// combat stats
	playerHP = cfg.getInt("Player", "hp");
	playerMaxHP = cfg.getInt("Player", "maxHp");
	playerMP = cfg.getInt("Player", "mp");
	playerMaxMP = cfg.getInt("Player", "maxMp");
	playerEXP = cfg.getInt("Player", "exp");
	playerLevel = cfg.getInt("Player", "level");

	// items
	for (unsigned int i = 0; i < (unsigned int)cfg.getInt("PlayerInventory", "itemCount"); i++) { playerInventoryItems.addItem(new Item(cfg.getInt("PlayerInventory", "item" + std::to_string(i)))); }
	for (unsigned int i = 0; i < (unsigned int)cfg.getInt("PlayerEquipment", "itemCount"); i++) { playerEquipmentItems.push_back(cfg.getInt("PlayerEquipment", "item" + std::to_string(i))); }

	// skills
	for (unsigned int i = 0; i < (unsigned int)cfg.getInt("PlayerSkills", "itemCount"); i++)
	{
		playerSkills.push_back(std::unique_ptr<LearnedSkill>(new LearnedSkill(
			cfg.getInt("PlayerSkills", "skill" + std::to_string(i) + "Id"),
			cfg.getInt("PlayerSkills", "skill" + std::to_string(i) + "Level"),
			cfg.getInt("PlayerSkills", "skill" + std::to_string(i) + "Exp")
		)));
	}

	// statistics
	playerDeaths = cfg.getInt("Statistics", "deaths");
}

void saveConfig()
{
	INIFile cfg;

	// position
	cfg.setFloat("Player", "x", cx);
	cfg.setFloat("Player", "z", cz);

	// combat stats
	cfg.setInt("Player", "hp", playerHP);
	cfg.setInt("Player", "maxHp", playerMaxHP);
	cfg.setInt("Player", "mp", playerMP);
	cfg.setInt("Player", "maxMp", playerMaxMP);
	cfg.setInt("Player", "exp", playerEXP);
	cfg.setInt("Player", "level", playerLevel);

	// items
	// TODO: fix item saving!
	//cfg.setInt("PlayerInventory", "itemCount", playerInventoryItems.size());
	//for (unsigned int i = 0; i < playerInventoryItems.size(); i++) { cfg.setInt("PlayerInventory", "item" + std::to_string(i), playerInventoryItems[i]); }
	cfg.setInt("PlayerEquipment", "itemCount", playerEquipmentItems.size());
	for (unsigned int i = 0; i < playerEquipmentItems.size(); i++) { cfg.setInt("PlayerEquipment", "item" + std::to_string(i), playerEquipmentItems[i]); }

	// skills
	cfg.setInt("PlayerSkills", "itemCount", playerSkills.size());
	for (unsigned int i = 0; i < playerSkills.size(); i++)
	{
		LearnedSkill* skill = playerSkills[i].get();
		cfg.setInt("PlayerSkills", "skill" + std::to_string(i) + "Id", skill->getId());
		cfg.setInt("PlayerSkills", "skill" + std::to_string(i) + "Level", skill->getLevel());
		cfg.setInt("PlayerSkills", "skill" + std::to_string(i) + "Exp", skill->getExp());
	}

	// statistics
	cfg.setInt("Statistics", "deaths", playerDeaths);
	cfg.setFloat("Statistics", "totalTravelDistance", playerTotalTravelDistance);

	// save to file
	cfg.save("settings.ini");
}

#pragma endregion

#pragma region Rendering

#pragma region Enemy Wave Management

int currentWave = 0;
int highestWave = 3;
int waveEnemiesKilled = 0;
int getWaveEnemyCount(int level) { return 4 + (level * 7) + (((level - 1) * 3) * level); }
int getWaveEnemySpawnsRemaining() { return getWaveEnemyCount(currentWave) - (waveEnemiesKilled + enemies.size()); }
bool waveTransition = false;
long long lastWaveEndTime = 0;
int getWaveRemainingTransitionTime() { return (int)((lastWaveEndTime + 60000) - Tools::currentTimeMillis()); }

class WaveEnterDialogueWindow : public IDialogueWindow
{
public:
	virtual std::string getMessage() { return "Do you want to start waves?"; }

	virtual void onOk()
	{
		currentWave = 1;
		unloadNPC(2);
	}
};

class WaveEnterNPC : public INPC
{
public:
	virtual void onClick()
	{
		addInformationHistory("Clicked on wave enter npc");
		showDialogueWindow(new WaveEnterDialogueWindow());
	}
};

class WavesCompleteDialogueWindow : public IDialogueWindow
{
public:
	virtual std::string getMessage() { return "Good job! All waves wiped out!"; }
};

class WaveEnemyListener : public ICombatEntityListener
{
public:
	virtual void onKilled(CombatEntity* entity)
	{
		waveEnemiesKilled++;

		if (waveEnemiesKilled == getWaveEnemyCount(currentWave))
		{
			if (currentWave < highestWave)
			{
				lastWaveEndTime = Tools::currentTimeMillis();
				waveTransition = true;
				loadNPC(1, glm::vec3());
			}
			else
			{
				currentWave = 0;
				waveEnemiesKilled = 0;
				loadNPC(2, glm::vec3(10, 0, 10));
				showDialogueWindow(new WavesCompleteDialogueWindow());
			}
		}
	}
};

std::unique_ptr<ICombatEntityListener> waveEnemyListener;

class WavesFailedDialogueWindow : public IDialogueWindow
{
public:
	virtual std::string getMessage() { return "You died! Left wave context."; }
};

class WavePlayerListener : public ICombatEntityListener
{
public:
	virtual void onKilled(CombatEntity* entity)
	{
		currentWave = 0;
		waveEnemiesKilled = 0;
		loadNPC(2, glm::vec3(10, 0, 10));
		enemies.clear();
		for (auto& i : spawnPoints) { i->reset(); }
		//droppedItems.clear();
		playerHP = 5;
		showDialogueWindow(new WavesFailedDialogueWindow());
		playerDeaths++;
	}
};

std::unique_ptr<ICombatEntityListener> wavePlayerListener;

void updateWaveTransition()
{
	if (waveTransition && getWaveRemainingTransitionTime() <= 0)
	{
		currentWave++;
		waveEnemiesKilled = 0;
		waveTransition = false;
		unloadNPC(1);
		getUIWindowByTitle("Shop")->setVisible(false);
	}
}

#pragma endregion

#pragma region Enemy Item Dropping

class DroppedItem
{
private:
	int mId;
	glm::vec3 mPosition;
	long long mDropTime;

public:
	DroppedItem(int id, const glm::vec3& pos) : mId(id), mPosition(pos), mDropTime(Tools::currentTimeMillis()) {}

	const glm::vec3& getPosition() const { return mPosition; }
	const int& getId() const { return mId; }

	void draw()
	{
		float flareFactor = glm::mix(0.0f, 3.141592f, (float)((Tools::currentTimeMillis() - mDropTime) % 4500) / 4500.0f);

		glColor3f(0.5f, 0.5f, 0.5f);
		glPushMatrix();
		glTranslatef(mPosition.x, mPosition.y + 0.25f + (sinf(flareFactor) / 2.0f), mPosition.z);
		glRotatef(flareFactor * 57.2958f, 0.0f, 1.0f, 0.0f);
		glutSolidCube(0.25f);
		glPopMatrix();
	}
};

std::vector<std::unique_ptr<DroppedItem>> droppedItems;

class EnemyDropItemListener : public ICombatEntityListener
{
public:
	virtual void onKilled(CombatEntity* entity)
	{
		if (randomNumber(0, 100) > 50)
		{
			droppedItems.push_back(std::unique_ptr<DroppedItem>(new DroppedItem(1, ((Enemy*)entity)->getPosition())));
		}
	}
};

std::unique_ptr<ICombatEntityListener> enemyDropItemListener;

#pragma endregion

#pragma region Map Loading

std::vector<std::unique_ptr<Dungeon>> dungeons;

bool isDungeonChunk(int x, int y, int z)
{
	for (auto& dungeon : dungeons)
	{
		if (dungeon->usesChunk(x, y, z)) { return true; }
	}
	return false;
}

void loadNewChunks()
{
	PerlinNoise noise;
	glm::ivec3 playerVoxel(getPlayerPositionVoxelPos());
	glm::ivec3 curChunk(getVoxelChunkPos(playerVoxel.x, playerVoxel.y, playerVoxel.z));

	std::vector<glm::ivec3> treeChunks;

	// first perlin on the ground
	for (int x = curChunk.x - volumeRenderDistance; x <= curChunk.x + volumeRenderDistance; x++)
	{
		for (int y = curChunk.y - 1; y <= curChunk.y + 1; y++)
		{
			for (int z = curChunk.z - volumeRenderDistance; z <= curChunk.z + volumeRenderDistance; z++)
			{
				// don't auto-generate noise on dungeon terrain
				if (isDungeonChunk(x, y, z)) { continue; }

				// dynamically load terrain using perlin
				if (mChunks.count(glm::ivec3(x, y, z)) == 0)
				{
					// load unloaded chunk near range
					int xxStart = x * 16;
					int yyStart = y * 16;
					int zzStart = z * 16;

					for (int xx = xxStart; xx < xxStart + 16; xx++)
					{
						for (int yy = yyStart; yy < yyStart + 16; yy++)
						{
							for (int zz = zzStart; zz < zzStart + 16; zz++)
							{
								double n = noise.noise((double)xx / 128.0, (double)yy / 128.0, (double)zz / 128.0);
								n += 1.0; // temporarily push all generation into positive space. physics fucks up at cy < 0
								n *= 16.0;
								if (yy <= (int)std::floor(n))
								{
									setVoxel(xx, yy, zz, randomNumber(0, 30), randomNumber(100, 255), randomNumber(0, 30));
								}
							}
						}
					}

					// add it for tree processing if not on the edges (don't want trees to fuck up perlin)
					if (x > curChunk.x - volumeRenderDistance && x < curChunk.x + volumeRenderDistance &&
						z > curChunk.z - volumeRenderDistance && z < curChunk.z + volumeRenderDistance)
					{ treeChunks.push_back(glm::ivec3(x, y, z)); }
				}
			}
		}
	}

	// then randomly place some trees
	for (auto c : treeChunks)
	{
		int xxStart = c.x * 16;
		int yyStart = c.y * 16;
		int zzStart = c.z * 16;

		glm::ivec3 treeStart(xxStart + getRandomInt(3, 7), yyStart, zzStart + getRandomInt(3, 7));
		// no tree spawns if ground doesn't exist on this chunk
		if (getVoxel(treeStart).a != 0)
		{
			treeStart.y++;
			bool airFound = false;
			for (int i = 0; i < 15; i++)
			{
				if (getVoxel(treeStart).a != 0) { treeStart.y++; }
				else { airFound = true; break; }
			}
			if (airFound) { setTree(treeStart.x, treeStart.y, treeStart.z); }
		}
	}

	// draw dungeon borders
	for (auto& dungeon : dungeons) { dungeon->drawBorder(); }
}

void loadGameMap()
{
	dungeons.push_back(std::unique_ptr<Dungeon>(new Dungeon(-1024, -1024)));
	loadNewChunks();
	while (getVoxel(getPlayerPositionVoxelPos()).a != 0) { cy++; }
}

#pragma endregion

// process and draw map
void drawGameMap(float elapsed)
{
	// Draw ground
	loadNewChunks();
	renderChunks();
	//glColor3f(0.9f, 0.9f, 0.9f);
	//glBegin(GL_QUADS);
	//glVertex3f(-200.0f, 0.0f, -200.0f);
	//glVertex3f(-200.0f, 0.0f, 200.0f);
	//glVertex3f(200.0f, 0.0f, 200.0f);
	//glVertex3f(200.0f, 0.0f, -200.0f);
	//glEnd();

	//glEnableClientState(GL_VERTEX_ARRAY);
	//glEnableClientState(GL_COLOR_ARRAY);
	//glVertexPointer(3, GL_FLOAT, 0, &terrainPositionBuffer[0]);
	//glColorPointer(3, GL_FLOAT, 0, &terrainColorBuffer[0]);
	//glDrawArrays(GL_QUADS, 0, terrainVertexCount);
	//glDisableClientState(GL_VERTEX_ARRAY);
	//glDisableClientState(GL_COLOR_ARRAY);

	// update and draw everything
	for (auto& i : spawnPoints)
	{
		// draw
		glColor3f(0.5f, 0.5f, 0.5f);
		glPushMatrix();
		glTranslatef(i.get()->getPosition().x, i.get()->getPosition().y + 1, i.get()->getPosition().z);
		glutWireCube(2.0f);
		glPopMatrix();

		// process
		if (currentWave > 0 && getWaveEnemySpawnsRemaining() > 0)
		{
			if (i.get()->shouldSpawn())
			{
				// grab base from spawn point
				Enemy* enemy = i.get()->getEnemy();

				// scale with waves
				enemy->setMaxHP(enemy->getMaxHP() * currentWave);
				enemy->setHP(enemy->getMaxHP());
				enemy->setAttackDamage(enemy->getAttackDamage() * currentWave);
				enemy->setMoveSpeed(enemy->getMoveSpeed() + ((currentWave - 1) * 1.5f));

				// prepare with necessary listeners
				enemy->addListener(waveEnemyListener.get());
				enemy->addListener(enemyDropItemListener.get());
				enemy->addListener(enemyGiveExpListener.get());

				// add to the map
				enemies.push_back(std::unique_ptr<Enemy>(enemy));
			}
		}
	}
	for (auto& i : enemies) { i.get()->update(elapsed); i.get()->draw(); }
	for (auto it = skillEffects.begin(); it != skillEffects.end(); )
	{
		it->get()->update(elapsed);
		it->get()->draw();
		if (it->get()->completed()) { it = skillEffects.erase(it); } else { it++; }
	}
	for (auto& i : droppedItems) { i.get()->draw(); }
	for (auto& i : loadedNPCs) { i->draw(); }

	updateWaveTransition();
	updatePlayer(elapsed);
	updateRaycasterAutomaticFire();
}

// high resolution frame timing
long long lastFrameTime = 0;

// fps counter variables
long long fpsTimeBase = 0;
int fpsFrameCount = 0;
char fpsStr[60];

// render a single frame
void renderScene()
{
	long long currFrameTime = Tools::currentTimeMillis();
	float elapsedFrameTime = (float)(currFrameTime - lastFrameTime) / 1000.0f;

	// Clear Color and Depth Buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// prepare 3d drawing
	glLoadIdentity();
	glPushMatrix();

	// update camera view
	//updateCamera(elapsedFrameTime);
	gluLookAt(cx, 1.5f + cy, cz,
		cx + lx, 1.5f + cy + ly, cz + lz,
		0.0f, 1.0f, 0.0f);

	// draw all 3d game modules
	//drawLoadedMapleMap();
	drawGameMap(elapsedFrameTime);
	renderStrokeFontString(0, 0, 0, GLUT_STROKE_ROMAN, "asddff1234test");

	// update these before ortho for proper un/projecting
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	// switch to 2d drawing mode
	glPopMatrix();
	setOrthographicProjection();

	// draw enemy hp bars
	for (auto& enemy : enemies)
	{
		glm::vec3 barPos = project(enemy->getPosition());
		if ((int)barPos.z != 1)
		{
			barPos.y = windowHeight - barPos.y;
			glColor3f(1.0f, 0.0f, 0.0f);
			//printf("Bar pos: %d, %d, %d\n", (int)barWinX, (int)barWinY, (int)barWinZ);
			glColor4f(1.0f, 0.0f, 0.0f, 0.2f);
			drawQuad2D((int)barPos.x - 25, (int)barPos.y, 50, 15);
			glColor4f(1.0f, 0.0f, 0.0f, 0.8f);
			drawQuad2D((int)barPos.x - 25, (int)barPos.y, calcProgressWidth(enemy->getHP(), enemy->getMaxHP(), 50), 15);
		}
	}

	// draw crosshair
	int windowCenterX = windowWidth / 2;
	int windowCenterY = windowHeight / 2;
	glColor4f(0.5f, 0.5f, 0.5f, 0.75f);
	drawQuad2D(windowCenterX - 25, windowCenterY - 2, 50, 4);
	drawQuad2D(windowCenterX - 2, windowCenterY - 25, 4, 50);

	// Update FPS
	fpsFrameCount++;
	if (currFrameTime - fpsTimeBase > 1000)
	{
		sprintf_s(fpsStr, "FPS: %4.2f", fpsFrameCount * 1000.0f / (currFrameTime - fpsTimeBase));
		fpsTimeBase = currFrameTime;
		fpsFrameCount = 0;
	}

	// display various stats
	glColor3f(0.0f, 0.0f, 0.0f);
	//drawLoadedMapleMapStatistics();
	renderSpacedBitmapString(10, 30, 0, GLUT_BITMAP_HELVETICA_18, fpsStr);

	std::string spStr = "Spawn Points: " + std::to_string(spawnPoints.size());
	std::string enStr = "Enemies: " + std::to_string(enemies.size());
	std::string skStr = "Skill Effects: " + std::to_string(skillEffects.size());
	renderString(10, 70, GLUT_BITMAP_HELVETICA_18, spStr);
	renderString(10, 90, GLUT_BITMAP_HELVETICA_18, enStr);
	renderString(10, 110, GLUT_BITMAP_HELVETICA_18, skStr);

	if (currentWave > 0)
	{
		std::string wvStr = "Wave " + std::to_string(currentWave) + " / " + std::to_string(highestWave) + " - Killed " + std::to_string(waveEnemiesKilled) + " / " + std::to_string(getWaveEnemyCount(currentWave));
		std::string wsStr = "Wave " + (waveTransition ? "Transition (" + std::to_string((getWaveRemainingTransitionTime() / 1000) + 1) + " sec left)" : "Active (" + std::to_string(getWaveEnemySpawnsRemaining()) + " more spawns)");
		renderString(10, 130, GLUT_BITMAP_HELVETICA_18, wvStr);
		renderString(10, 150, GLUT_BITMAP_HELVETICA_18, wsStr);
	}
	else
	{
		renderString(10, 130, GLUT_BITMAP_HELVETICA_18, "Waves Disabled");
	}

	std::string vxStr = "Active Chunks: " + std::to_string(mChunks.size()) + " | Extracting: " + std::to_string(activeSurfaceExtractionThreads) + " | Render Dist: " + std::to_string(volumeRenderDistance);
	renderString(5, 190, GLUT_BITMAP_HELVETICA_18, vxStr);
	glm::ivec3 playerVoxel(getPlayerPositionVoxelPos());
	std::string vpStr = "Player Voxel Pos: " + to_string(playerVoxel) + " | Chunk: " + to_string(getVoxelChunkPos(playerVoxel.x, playerVoxel.y, playerVoxel.z));
	renderString(5, 210, GLUT_BITMAP_HELVETICA_18, vpStr);
	std::string cpStr = "Camera: (" + to_string(glm::vec3(cx, cy, cz)) + ") -> (" + to_string(glm::vec3(lx, ly, lz)) + ")";
	renderSpacedBitmapString(5, 230, 0, GLUT_BITMAP_HELVETICA_18, cpStr.c_str());

	// render stat bars at the bottom

	// hp bar
	glColor4f(1.0f, 0.0f, 0.0f, 0.5f);
	drawQuad2D(0, windowHeight - 50, windowWidth / 2, 25);
	glColor4f(1.0f, 0.0f, 0.0f, 0.8f);
	drawQuad2D(0, windowHeight - 50, calcProgressWidth(playerHP, playerMaxHP, windowWidth / 2), 25);
	glColor3f(0.0f, 0.0f, 0.0f);
	renderString(windowWidth / 4, windowHeight - 30, GLUT_BITMAP_HELVETICA_18, std::string("HP: ") + std::to_string(playerHP) + " / " + std::to_string(playerMaxHP));
	// mp bar
	glColor4f(0.0f, 0.0f, 1.0f, 0.5f);
	drawQuad2D(windowWidth / 2, windowHeight - 50, windowWidth / 2, 25);
	glColor4f(0.0f, 0.0f, 1.0f, 0.8f);
	drawQuad2D(windowWidth / 2, windowHeight - 50, calcProgressWidth(playerMP, playerMaxMP, windowWidth / 2), 25);
	glColor3f(0.0f, 0.0f, 0.0f);
	renderString((windowWidth / 2) + (windowWidth / 4), windowHeight - 30, GLUT_BITMAP_HELVETICA_18, std::string("MP: ") + std::to_string(playerMP) + " / " + std::to_string(playerMaxMP));
	// exp bar
	glColor4f(0.0f, 1.0f, 0.0f, 0.5f);
	drawQuad2D(0, windowHeight - 25, windowWidth, 25);
	glColor4f(0.0f, 1.0f, 0.0f, 0.8f);
	drawQuad2D(0, windowHeight - 25, calcProgressWidth(playerEXP, getEXPNeeded(playerLevel), windowWidth), 25);
	glColor3f(0.0f, 0.0f, 0.0f);
	renderString(windowWidth / 2, windowHeight - 5, GLUT_BITMAP_HELVETICA_18, std::string("EXP: ") + std::to_string(playerEXP) + " / " + std::to_string(getEXPNeeded(playerLevel)));
	renderString(25, windowHeight - 5, GLUT_BITMAP_HELVETICA_18, std::string("Level: ") + std::to_string(playerLevel));

	// update and display information history side area
	updateInformationHistory();

	// draw UI windows
	for (unsigned int i = 0; i < uiWindows.size(); i++) { uiWindows[i]->onDraw(); }
	drawDialogueWindow();

	// process mouse movement for UI windows
	for (auto it = uiWindows.rbegin(); it != uiWindows.rend(); it++) { it->get()->onMouseMove(); }

	// return to 3d drawing context
	restorePerspectiveProjection();

	// flip back buffer to screen
	glutSwapBuffers();
	lastFrameTime = currFrameTime;
}

// display window resize event handler
void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if (h == 0)
		h = 1;

	windowWidth = w;
	windowHeight = h;

	float ratio = w * 1.0f / h;

	// Use the Projection Matrix
	glMatrixMode(GL_PROJECTION);

	// Reset Matrix
	glLoadIdentity();

	// Set the viewport to be the entire window
	glViewport(0, 0, w, h);

	// Set the correct perspective.
	gluPerspective(45.0f, ratio, 0.1f, 1000000000.0f);

	// Get Back to the Modelview
	glMatrixMode(GL_MODELVIEW);
}

#pragma endregion

#pragma region Input Processing

void attemptItemPickup()
{
	glm::vec3 playerPos(cx, 0.0f, cz);
	for (auto it = droppedItems.begin(); it != droppedItems.end(); it++)
	{
		if (glm::distance(playerPos, it->get()->getPosition()) <= 1.0f)
		{
			playerInventoryItems.addItem(new Item(it->get()->getId()));
			addInformationHistory("Picked up item (id " + std::to_string(it->get()->getId()) + ")");
			droppedItems.erase(it);
			break;
		}
	}
}

#define GLUT_KEY_Q 113
#define GLUT_KEY_W 119
#define GLUT_KEY_E 101
#define GLUT_KEY_R 114
#define GLUT_KEY_I 105
#define GLUT_KEY_A 97
#define GLUT_KEY_S 115
#define GLUT_KEY_D 100
#define GLUT_KEY_Z 122
#define GLUT_KEY_0 48
#define GLUT_KEY_1 49
#define GLUT_KEY_2 50
#define GLUT_KEY_3 51
#define GLUT_KEY_4 52
#define GLUT_KEY_5 53
#define GLUT_KEY_6 54
#define GLUT_KEY_7 55
#define GLUT_KEY_8 56
#define GLUT_KEY_9 57
#define GLUT_KEY_SPACEBAR 32
#define GLUT_KEY_F 102

void processNormalKeys(unsigned char key, int x, int y)
{
	printf("Normal key: %d\n", key);

	switch (key)
	{
	case GLUT_KEY_Q: toggleCameraMouseLock(); break;
	case GLUT_KEY_W: moveCamForward = true; break;
	case GLUT_KEY_A: charPos.x--; break;
	case GLUT_KEY_S: moveCamBackward = true; break;
	case GLUT_KEY_D: charPos.x++; break;
	case GLUT_KEY_E: getUIWindowByTitle("Equipment")->setVisible(!getUIWindowByTitle("Equipment")->getVisible()); break;
	case GLUT_KEY_I: getUIWindowByTitle("Inventory")->setVisible(!getUIWindowByTitle("Inventory")->getVisible()); break;
	case GLUT_KEY_R: getUIWindowByTitle("Skills")->setVisible(!getUIWindowByTitle("Skills")->getVisible()); break;
	case GLUT_KEY_Z: attemptItemPickup(); break;
	case GLUT_KEY_SPACEBAR: playerJumpRequested = true; break;
	case GLUT_KEY_F: getUIWindowByTitle("Keybinding")->setVisible(!getUIWindowByTitle("Keybinding")->getVisible()); break;
	}

	// auto bind learned skills to number keys 1 - 9
	if (key >= GLUT_KEY_1 && key <= GLUT_KEY_9)
	{
		int skillIndex = key - GLUT_KEY_1;
		if (skillIndex >= (int)playerSkills.size()) { return; }
		auto skill = loadedSkills.find(playerSkills[skillIndex]->getId());
		if (skill != loadedSkills.end()) { skill->second->attemptCast(); }
	}

	if (key == 27) // esc
	{
		saveConfig();
		exit(0);
	}
}

void processNormalKeysUp(unsigned char key, int x, int y) {

	//printf("Normal key up: %d\n", key);

	switch (key)
	{
	case GLUT_KEY_W: moveCamForward = false; break;
	case GLUT_KEY_S: moveCamBackward = false; break;
	}
}

void processSpecialKeys(int key, int x, int y) {

	//int mod = glutGetModifiers();
	//if (mod == (GLUT_ACTIVE_CTRL | GLUT_ACTIVE_ALT | GLUT_ACTIVE_SHIFT))

	switch (key) {
		// lmao
	//case GLUT_KEY_F1:
	//case GLUT_KEY_F2:
	//case GLUT_KEY_F3:

		// useful shit
	case GLUT_KEY_LEFT:
		angle -= 0.01f;
		lx = sinf(angle);
		lz = -cosf(angle);
		break;
	case GLUT_KEY_RIGHT:
		angle += 0.01f;
		lx = sinf(angle);
		lz = -cosf(angle);
		break;
	//case GLUT_KEY_UP:
	//case GLUT_KEY_DOWN:
	}
}

glm::ivec2 windowDragOrigin;
glm::ivec2 draggedWindowOriginalPos;
UIWindow* draggedWindow = 0;

void mouseButton(int button, int state, int x, int y)
{
	// only start motion if the left button is pressed
	if (button == GLUT_LEFT_BUTTON)
	{
		/*
		// when the button is released
		if (state == GLUT_UP) {
			angle += deltaAngle;
			xOrigin = -1;
		}
		else {// state = GLUT_DOWN
			xOrigin = x;
		}
		*/

		if (state == GLUT_DOWN)
		{
			// window dragging
			for (auto it = uiWindows.rbegin(); it != uiWindows.rend(); it++)
			{
				UIWindow* window = it->get();

				if (!window->getVisible()) { continue; }

				// calculate title bar area
				const glm::ivec2& pos = window->getPosition();
				glm::ivec2 titleStart(pos.x + 10, pos.y + 7);
				glm::ivec2 titleEnd(window->getSize().x - 40, 20);
				titleEnd += titleStart;

				if (x >= titleStart.x && y >= titleStart.y && x <= titleEnd.x && y <= titleEnd.y)
				{
					windowDragOrigin.x = x;
					windowDragOrigin.y = y;
					draggedWindowOriginalPos = pos;
					draggedWindow = window;
					break;
				}
			}

			// check UI window clicks
			for (auto it = uiWindows.rbegin(); it != uiWindows.rend(); it++)
			{
				UIWindow* window = it->get();

				if (window->onClick(x, y))
				{
					// last clicked window should be drawn last (front of z-order)
					if (uiWindows.back().get() != window)
					{
						uiWindows.push_back(std::move(*it));
						uiWindows.erase(std::next(it).base());
					}
					break;
				}
			}
			onDialogueWindowClick(x, y);

			// check NPC clicks
			for (unsigned int i = 0; i < loadedNPCs.size(); i++)
			{
				// gen aabb
				glm::vec3 lowerLeft(loadedNPCs[i]->position);
				glm::vec3 upperRight(lowerLeft);
				lowerLeft.x--;
				lowerLeft.z--;
				upperRight.x++;
				upperRight.z++;
				upperRight.y += 2;

				glm::vec3 rayStart(cx, 1.0f, cz);
				glm::vec3 rayEnd(lx, 0.0f, lz);
				rayEnd *= 150.0f;
				rayEnd += rayStart;

				glm::vec3 rayHit;
				if (CheckLineBox(lowerLeft, upperRight, rayStart, rayEnd, rayHit)) { registeredNPCs[loadedNPCs[i]->id]->onClick(); }
			}
		}
		else if (state == GLUT_UP)
		{
			// window dragging
			draggedWindow = 0;
		}
	}
	// shoot on right click
	else if (button == GLUT_RIGHT_BUTTON)
	{
		if (state == GLUT_DOWN) { shootingRaycaster = true; }
		else if (state == GLUT_UP) { shootingRaycaster = false; }
	}
}

void mouseMove(int x, int y)
{
	// update mouse position for the ui
	currentMousePos.x = x;
	currentMousePos.y = y;

	/*
	// this will only be true when the left button is down
	if (xOrigin >= 0) {

		// update deltaAngle
		deltaAngle = (x - xOrigin) * 0.001f;

		// update camera's direction
		lx = sinf(angle + deltaAngle);
		lz = -cosf(angle + deltaAngle);
	}
	*/

	if (draggedWindow)
	{
		glm::ivec2 diff = glm::ivec2(x, y) - windowDragOrigin;
		draggedWindow->setPosition(draggedWindowOriginalPos + diff);
	}
}

void mouseMovePassive(int x, int y)
{
	// update mouse position for the ui
	currentMousePos.x = x;
	currentMousePos.y = y;
}

#pragma endregion

int main(int argc, char** argv)
{
	// init GLUT and create Window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1600, 900);
	glutCreateWindow("wings");

	// register callbacks
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutIdleFunc(renderScene);
	glutKeyboardFunc(processNormalKeys);
	glutKeyboardUpFunc(processNormalKeysUp);
	glutSpecialFunc(processSpecialKeys);
	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMove);
	glutPassiveMotionFunc(mouseMovePassive);

	// OpenGL init
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(BYTE_TO_FLOAT_COLOR(30), BYTE_TO_FLOAT_COLOR(144), BYTE_TO_FLOAT_COLOR(255), 0.0f); // dodgerblue
	glClearColor(0.0f, BYTE_TO_FLOAT_COLOR(191), BYTE_TO_FLOAT_COLOR(255), 0.0f); // deepskyblue
	glClearColor(BYTE_TO_FLOAT_COLOR(100), BYTE_TO_FLOAT_COLOR(149), BYTE_TO_FLOAT_COLOR(237), 0.0f); // cornflowerblue

	// game init
	srand((unsigned int)time(0));
	//switchMapleMap(104040000); // hhg1

	// load enemy listeners
	waveEnemyListener.reset(new WaveEnemyListener());
	enemyDropItemListener.reset(new EnemyDropItemListener());
	enemyGiveExpListener.reset(new EnemyGiveExpListener());

	// load player listeners
	wavePlayerListener.reset(new WavePlayerListener());
	playerEntity->addListener(wavePlayerListener.get());

	// register skills
	registerSkill(1000001, new RaycasterBasicAttackSkill());
	registerSkill(1000002, new RaycasterPowerShotSkill());
	registerSkill(1000003, new RaycasterBlastShotSkill());
	registerSkill(1000004, new ChargeDashBasicSkill());
	registerSkill(1000005, new ChargeDashRushSkill());
	registerSkill(1000006, new ChargeDashSweepSkill());

	// register npcs
	registerNPC(1, new TraderNPC());
	registerNPC(2, new WaveEnterNPC());

	// register items
	registerItem(1000001, new Item_BasicRaycaster());
	registerItem(1000002, new Item_PowerRaycaster());
	registerItem(1000003, new Item_BlastRaycaster());
	registerItem(1000004, new Item_BasicCharger());
	registerItem(1000005, new Item_PowerCharger());
	registerItem(1000006, new Item_BlastCharger());
	registerItem(2000001, new Item_RedPotion());
	registerItem(2000002, new Item_BluePotion());

	// add UI windows
	addUIWindow(new InventoryWindow());
	addUIWindow(new EquipmentWindow());
	addUIWindow(new SkillsWindow());
	addUIWindow(new ShopWindow());
	addUIWindow(new KeybindingWindow());

	initKeybindings();

	// load player config
	loadConfig();

	// load map
	loadGameMap();

	// enter GLUT event processing cycle
	lastFrameTime = Tools::currentTimeMillis();
	glutMainLoop();

	return 1;
}

/*
v1:
mob spawns, basic mob movement, dealing damage, NPCs, death, waves, shops, inventory, equipment, skills, NPC dialogues, stat calculations,
player and mob skills, voxel engine core, player stat regen, maze generation

v2:
a* pathfinding on mobs, voxel terrain physics, jumping

v3:
3 new skills, inventory system upgrades (icon size increased, icon visuals added, tooltips w/ name and description, item stacking),
hp and mp pots, volume surface extraction multithreaded, ui system upgrades (window dragging, universal closing, z-ordering),
terrain generation upgrades (perlin, dungeons, trees)

DONE BUT DISABLED:
Maple map XML wz foothold and layout voxel mapping with depth expansion and dynamic noise, portals, switching maps, ropes/ladders (render only)

NEXT:
trader positioning fixed, mob scaling, skill scaling, wave boss,
equipment window upgrades, skill window icons, bank, crafting, keybinding window, voxel placing & destroying

TENTATIVE TODO:
Map biome, noise, and XML wz mapping and depth expansion generation upgrades
Equipment and item voxel mapping generation & UI windows
Improve combat item and skill support, improve mob support
Map regeneration, mob stat regen, basic tutorial, UI improvement
*/