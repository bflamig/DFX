#pragma once
#include "RobinMgr.h"

struct VelocityRange
{
	// Scaled 0 - 127 

	int velCode;  // As given in the drum font - the nominal iMinVel

	int iMinVel;  // Determined after sorting
	int iMaxVel;  // Determined after sorting

	// Scaled 0 - 1.0 versions of the above

	//stk::StkFloat fMinVel;
	//stk::StkFloat fMaxVel;

	double fMinVel;
	double fMaxVel;

	// VelocityRange();

	void clear();
};


class VelocityLayer {
public:

	std::string cumulativePath;
	std::string localPath;

	VelocityRange vrange;

	RobinMgr robinMgr;

public:

	VelocityLayer();
	VelocityLayer(std::string& localPath_, int vel_code_);
	VelocityLayer(const VelocityLayer &other);
	VelocityLayer(VelocityLayer&& other) noexcept;
	virtual ~VelocityLayer() { }

};

