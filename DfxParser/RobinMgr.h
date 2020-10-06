#pragma once
#include <vector>
#include <string>
#include <filesystem>

class Robin {
public:
	std::filesystem::path fullPath;
	std::filesystem::path fileName;
	double peak; // In db, as given in the drum font
	double rms;  // In db, as given in the drum font
	size_t offset; // in frames, as given in the drum font

	Robin(std::string fileName_, double peak_, double rms_, size_t offset_);
	Robin(const Robin& other);
	Robin(Robin&& other) noexcept;

	Robin& operator=(const Robin& other);
	Robin& operator=(Robin&& other) noexcept;

	void FinishUp(std::filesystem::path& cumulativePath_);
};



class RobinMgr { // : Stk {
public:

	std::vector<Robin> robins;
	size_t lastRobinChosen;

public:

	RobinMgr();
	RobinMgr(const RobinMgr& other);
	RobinMgr(RobinMgr&& other) noexcept;
	virtual ~RobinMgr();

	RobinMgr& operator=(const RobinMgr& other);
	RobinMgr& operator=(RobinMgr&& other) noexcept;

	void FinishUp(std::filesystem::path& cumulativePath_);

	//void LoadWaves(std::string pathToWaves, bool is_directory, FileEncoding encoding);

	//MemWvIn& ChooseRobin();

	Robin& ChooseRobin();
};

