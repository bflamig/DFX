#pragma once
#include "DfxParser.h"
#include "DrumKit.h"

namespace bryx
{
	class DrumFont : public DfxParser {
	public:

		DfxVerifyResult LoadFile(std::string fname)
		{
			auto rv = DfxParser::LoadFile(fname);

			if (rv == ParserResult::NoError)
			{

			}
			else return DfxVerifyResult::UnspecifiedError; // @@ TODO: Last error?
		}

	public:

		// Have to pay close attention to const keywords here. What a mess!

		// IMPORTANT: These ASSUME the parse tree has been verified! If
		// you don't call verified or ignore verification errors, it's 
		// likely exceptions will be thrown.

		std::shared_ptr<DrumKit> BuildKit(const nv_type& kit);
		void BuildInstruments(std::shared_ptr<DrumKit>& kit, const object_map_type* instrument_map_ptr);
		void BuildInstrument(std::vector<MultiLayeredDrum>& drums, const nv_type& drum_nv);
		void BuildVelocityLayer(std::vector<std::shared_ptr<VelocityLayer>>& layers, std::shared_ptr<Value>& layer_sh_ptr);
		void BuildRobin(std::vector<Robin>& robins, NameValue* robin_nv_ptr);
	};

}; // end of namespace
