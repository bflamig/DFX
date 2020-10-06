#pragma once
#include <filesystem>
#include "MultiLayeredDrum.h"

class DrumKit {
public:
	std::filesystem::path cumulativePath; // For ease of recursing down
	std::filesystem::path basePath;       // Usually the path of the sound font file
	std::filesystem::path kitPath;        // Relative to sound font location
	std::string name;

	std::vector<MultiLayeredDrum> drums;
public:

	DrumKit();
	DrumKit(const std::string& name_, const std::string& kitPath_);
	DrumKit(const DrumKit& other);
	DrumKit(DrumKit&& other) noexcept;

	virtual ~DrumKit();

public:

	void FinishUp(std::filesystem::path& soundFontPath_);

};

