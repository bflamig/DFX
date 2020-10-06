#pragma once
#include "DfxParser.h"
#include "DrumKit.h"

namespace bryx
{
	class DrumFont : public DfxParser {
	public:

		std::vector<std::shared_ptr<DrumKit>> drumKits;

		DrumFont();
		virtual ~DrumFont() { }

	public:

		DfxVerifyResult LoadFile(std::ostream& slog, std::string fname);
		void DumpPaths(std::ostream& sout); // For testing purposes


	public:

		// Have to pay close attention to const keywords here. What a mess!

		// IMPORTANT: These ASSUME the parse tree has been verified! If
		// you don't call verified or ignore verification errors, it's 
		// likely exceptions will be thrown.

		void BuildFont();

		std::shared_ptr<DrumKit> BuildKit(const nv_type& kit);
		void BuildInstruments(std::shared_ptr<DrumKit>& kit, const object_map_type* instrument_map_ptr);
		void BuildInstrument(std::vector<MultiLayeredDrum>& drums, const nv_type& drum_nv);
		void BuildVelocityLayer(std::vector<std::shared_ptr<VelocityLayer>>& layers, std::shared_ptr<Value>& layer_sh_ptr);
		void BuildRobin(std::vector<Robin>& robins, NameValue* robin_nv_ptr);
	};

}; // end of namespace
