#pragma once
#include "Abstract_File.h"

class File : public Abstract_File {

public:
	File() : Abstract_File() {

	}

	std::string toString() {
		return this->getName();
	}
};