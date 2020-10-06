#include "VelocityLayer.h"

#if 0
VelocityRange::VelocityRange()
: velCode(0)
, iMinVel(0)
, iMaxVel(0)
, fMinVel(0)
, fMaxVel(0)
{

}
#endif

void VelocityRange::clear()
{
	velCode = 0;
	iMinVel = 0;
	iMaxVel = 0;
	fMinVel = 0;
	fMaxVel = 0;
}

///

VelocityLayer::VelocityLayer()
: cumulativePath{}
, localPath{}
, vrange{}
, robinMgr{}
{

}


VelocityLayer::VelocityLayer(std::string &localPath_, int vel_code_)
: VelocityLayer{}
{
	localPath = localPath_;
	localPath = localPath.generic_string();
	vrange.velCode = vel_code_;
	vrange.iMinVel = vel_code_;
}

VelocityLayer::VelocityLayer(const VelocityLayer& other)
{
	cumulativePath = other.cumulativePath;
	localPath = other.localPath;
	vrange = other.vrange;
	robinMgr = other.robinMgr;
}

VelocityLayer::VelocityLayer(VelocityLayer&& other) noexcept
{
	cumulativePath = other.cumulativePath;
	localPath = other.localPath;
	vrange = other.vrange;
	robinMgr = std::move(other.robinMgr);

	other.cumulativePath.clear();
	other.localPath.clear();
	other.vrange.clear();
}


void VelocityLayer::FinishUp(std::filesystem::path& cumulativePath_)
{
	cumulativePath = cumulativePath_;
	cumulativePath /= localPath;
	cumulativePath = cumulativePath.generic_string();
	robinMgr.FinishUp(cumulativePath);
}

