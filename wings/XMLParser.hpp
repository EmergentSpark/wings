#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>

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