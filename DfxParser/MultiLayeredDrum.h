#pragma once
#include <memory>
#include "VelocityLayer.h"

class MultiLayeredDrum {
public:

	std::string cumulativePath;  // For ease of recursing down
	std::string drumPath;        // Relative to kit location
	std::string name;

	std::vector<std::shared_ptr<VelocityLayer>> velocityLayers;

	int midiNote; // 0 - 127

public:

	MultiLayeredDrum(const std::string &name_, const std::string& drumPath_, int midiNote_);
	virtual ~MultiLayeredDrum() { }

	void SortLayers();

	int FindVelocityLayer(int vel);         // Mostly for debugging
	int FindVelocityLayer(double vel);

	RobinMgr& SelectVelocityLayer(int vel);  // Mostly for debugging
	RobinMgr& SelectVelocityLayer(double vel);

};

