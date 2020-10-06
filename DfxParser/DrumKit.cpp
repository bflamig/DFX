#include <iostream>
#include "DrumKit.h"

DrumKit::DrumKit()
{
	std::cout << "DrumKit default ctor called" << std::endl;
}

DrumKit::DrumKit(const std::string& name_, const std::string& kitPath_)
: kitPath(kitPath_)
, name(name_)
, drums()
{
	std::cout << "DrumKit ctor called" << std::endl;
}

DrumKit::DrumKit(const DrumKit& other)
: kitPath(other.kitPath)
, name(other.name)
, drums(other.drums)
{
	std::cout << "Drumkit copy ctor called" << std::endl;
}

DrumKit::DrumKit(DrumKit&& other) noexcept
: kitPath(other.kitPath)
, name(other.name)
, drums(other.drums)
{
	std::cout << "DrumKit mtor called" << std::endl;
}

DrumKit::~DrumKit()
{
	std::cout << "DrumKit dtor called" << std::endl;
}

void DrumKit::FinishUp(std::string& basePath_)
{
	basePath = basePath_;

	for (auto &d : drums)
	{
		d.SortLayers();

		for (auto layer : d.velocityLayers)
		{

		}
	}
}
