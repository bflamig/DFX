#include <algorithm>
#include "MultiLayeredDrum.h"

MultiLayeredDrum::MultiLayeredDrum(const std::string& name_, const std::string& drumPath_, int midiNote_)
: cumulativePath()
, drumPath(drumPath_)
, name(name_)
, velocityLayers()
, midiNote(midiNote_)
{

}

void MultiLayeredDrum::SortLayers()
{
	// First, take all the velocity codes (aka min velocity values) from
	// the instrument, and begin to populate the velocity table.

	std::sort(velocityLayers.begin(), velocityLayers.end(),
		[](const std::shared_ptr<VelocityLayer>& a, const std::shared_ptr<VelocityLayer>& b) // Hmm, where's the return type?
		{
			return a->vrange.iMinVel < b->vrange.iMinVel;
		}
	);

	// Check for duplicates. If see any, we're outta here

	const auto nlayers = velocityLayers.size();

	int prev_vel_code = velocityLayers[0]->vrange.iMinVel;

	for (size_t i = 1; i < nlayers; i++)
	{
		int next_vel_code = velocityLayers[i]->vrange.iMinVel;
		if (next_vel_code == prev_vel_code)
		{
			//oStream_ << "MultiLayeredDrum::SortVelocityRanges(): There is a least one duplicate velocity code in the set.\n";
			//handleError(StkError::FILE_CONFIGURATION);
		}
		prev_vel_code = next_vel_code;
	}

	// Set the max vel's.

	// But first, let's inspect the first layer. If its minimum velocity
	// isn't zero, then make it. It kinda needs to be.

	if (velocityLayers[0]->vrange.iMinVel != 0)
	{
		velocityLayers[0]->vrange.iMinVel = 0;
	}

	// onward

	// const auto n = velocityRanges.size();

	for (size_t i = 0; i < nlayers - 1; i++)
	{
		velocityLayers[i]->vrange.iMaxVel = velocityLayers[i+1]->vrange.iMinVel - 1;
	}

	// Set the maximum velocity of the last element

	velocityLayers[nlayers-1]->vrange.iMaxVel = 127; // by definition

	// Okay, set the floating point scaled versions of these things

	for (auto& vl : velocityLayers)
	{
		vl->vrange.fMinVel = (double)vl->vrange.iMinVel / 127.0;
		vl->vrange.fMaxVel = (double)vl->vrange.iMaxVel / 127.0;
	}

	// Whew! We're done!
}

int MultiLayeredDrum::FindVelocityLayer(int vel)         // Mostly for debugging
{
	int idx = -1;

	if (vel < 0 || vel > 127)
	{
		//oStream_ << "MultiLayeredDrum::SelectVelocityLayer(): Velocity " << vel << " out of range.\n";
		//handleError(StkError::FUNCTION_ARGUMENT);
	}

	auto n = velocityLayers.size();

	for (size_t i = 0; i < n; i++)
	{
		const auto& e = velocityLayers[i]->vrange;
		if ((vel >= e.iMinVel) && (vel <= e.iMaxVel)) return i; //  velocityLayers[i].vrange.robinsIdx;
	}

	//oStream_ << "MultiLayeredDrum::SelectVelocityLayer(): Couldn't find a range for velocity " << vel << "\n";
	//handleError(StkError::UNSPECIFIED);
	//return -1; // Won't get here, but whatever

	return idx;
}

int MultiLayeredDrum::FindVelocityLayer(double vel)
{
	int idx = -1;

	if (vel < 0.0 || vel > 1.0)
	{
		//oStream_ << "MultiLayeredDrum::SelectVelocityLayer(): Velocity " << vel << " out of range.\n";
		//handleError(StkError::FUNCTION_ARGUMENT);
	}

	auto n = velocityLayers.size();

	for (size_t i = 0; i < n; i++)
	{
		const auto& e = velocityLayers[i]->vrange;
		if ((vel >= e.fMinVel) && (vel <= e.fMaxVel)) return i; //  velocityLayers[i].vrange.robinsIdx;
	}

	//oStream_ << "MultiLayeredDrum::SelectVelocityLayer(): Couldn't find a range for velocity " << vel << "\n";
	//handleError(StkError::UNSPECIFIED);
	//return -1; // Won't get here, but whatever

	return idx;
}


RobinMgr& MultiLayeredDrum::SelectVelocityLayer(int vel)  // Mostly for debugging
{
	auto r = FindVelocityLayer(vel);
	return velocityLayers[r]->robinMgr;
}

RobinMgr& MultiLayeredDrum::SelectVelocityLayer(double vel)
{
	auto r = FindVelocityLayer(vel);
	return velocityLayers[r]->robinMgr;
}
