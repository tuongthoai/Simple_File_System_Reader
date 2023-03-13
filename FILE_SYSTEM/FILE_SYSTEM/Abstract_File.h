#pragma once
#include <iostream>
#include <Windows.h>
#include <vector>

class Abstract_File {
private:
	std::string fileName;
	BYTE fileState;
	UINT32 startCluster;
	UINT32 fileSize;
	std::vector<UINT32> clusters;

public:
	void setName(std::string _name) {
		this->fileName = _name;
	}

	void setFileState(BYTE _state) {
		this->fileState = _state;
	}

	void setStartCluster(UINT32 _cluster) {
		this->startCluster = _cluster;
	}

	void setFileSize(UINT32 _size) {
		this->fileSize = _size;
	}

	std::string getName() { return this->fileName; }
	std::string getFileState() {
		if (fileState & 0b00100000) {
			return "FILE";
		}

		if (fileState & 0b00010000) {
			return "DIRECTORY";
		}

		return "OTHER";
	}

	UINT32 getStartCluster() { return this->startCluster; }
	UINT32 getFileSize() { return this->fileSize; }

	void addCluster(UINT32 clus) {
		this->clusters.push_back(clus);
	}

	const std::vector<UINT32>& getClusters() {
		return this->clusters;
	}

	virtual std::string toString() = 0;
};