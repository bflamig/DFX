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

void DrumKit::FinishUp(std::filesystem::path& soundFontPath_)
{
	basePath = soundFontPath_;
	basePath.remove_filename();

	cumulativePath = basePath;
	cumulativePath /= kitPath;
	cumulativePath = cumulativePath.generic_string(); // Sigh! THey just had to do it wrong!

	for (auto &d : drums)
	{
		d.cumulativePath = cumulativePath;
		d.cumulativePath /= d.drumPath;
		d.cumulativePath = d.cumulativePath.generic_string();

		d.SortLayers();

		for (auto layer : d.velocityLayers)
		{
			layer->FinishUp(d.cumulativePath);
		}
	}
}
