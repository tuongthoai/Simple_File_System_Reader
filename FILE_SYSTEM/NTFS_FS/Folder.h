#pragma once
#include "Abstract_File.h"

class Folder : public Abstract_File {
private:
	std::vector<Abstract_File*> context;
public:
	Folder() : Abstract_File() {}
	~Folder() {
		for (auto p : context) {
			delete p;
			p = nullptr;
		}
	}

	void add(Abstract_File* f) {
		if (f != nullptr) {
			context.push_back(f);
		}
	}

	std::vector<Abstract_File*>& getContext() { return this->context; }
};