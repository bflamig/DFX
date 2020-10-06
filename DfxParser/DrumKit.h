#pragma once
#include "MultiLayeredDrum.h"

class DrumKit {
public:
	std::string cumulativePath; // For ease of recursing down
	std::string basePath;       // Usually the path of the sound font file
	std::string kitPath;        // Relative to sound font location
	std::string name;

	std::vector<MultiLayeredDrum> drums;
public:

	DrumKit();
	DrumKit(const std::string& name_, const std::string& kitPath_);
	DrumKit(const DrumKit& other);
	DrumKit(DrumKit&& other) noexcept;

	virtual ~DrumKit();

public:

	void FinishUp(std::string& basePath_);

};

