#pragma once

/******************************************************************************\
 * DFX - "Drum font exchange format" - source code
 *
 * Copyright (c) 2020 by Bryan Flamig
 *
 * This software helps facilitate the real-time playing of multi-layered drum
 * samples, by implementing a language that specifies a master directory of the
 * the sample wave files for a drum kit: where the samples are, what they are
 * for, and a summary of their properties. The DFX format allows modifications
 * of sample levels to achieve a unified, pleasing mix of sounds. Mechanisms
 * such as velocity layers and round robins are supported for this purpose.
 *
 * This exchange format has a one to one mapping to the widely used Json syntax,
 * simplified to be easier to read and write. It is easy to translate DFX files
 * into Json files that can be parsed by any software supporting Json syntax.
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "DfxParser.h"
#include "DrumKit.h"
#include <string>

namespace dfx
{
	using namespace bryx;

	class DrumFont : public DfxParser {
	public:

		std::vector<std::shared_ptr<DrumKit>> drumKits;

		DrumFont();
		virtual ~DrumFont() { }

	public:

		DfxResult LoadFile(std::ostream& slog, std::string_view &fname);
		void DumpRobins(std::ostream& sout); // For testing purposes


	public:

		// Have to pay close attention to const keywords here. What a mess!

		// IMPORTANT: These ASSUME the parse tree has been verified! If
		// you don't call verified or ignore verification errors, it's 
		// likely exceptions will be thrown.

		void BuildFont();

		std::shared_ptr<DrumKit> BuildKit(std::filesystem::path base_path, const nv_type& kit);
		void BuildInstruments(std::shared_ptr<DrumKit>& kit, const curly_list_type* instrument_map_ptr);
		//void BuildInstrument(std::vector<drum_ptr>& drums, std::filesystem::path cumulativePath, const nv_type& drum_nv);
		void BuildInstrument(std::shared_ptr<DrumKit>& kit, const nv_type& drum_nv);
		drum_ptr MakeInstrument(const std::string &drum_name, std::filesystem::path cumulativePath, std::filesystem::path drumPath, int midiNote, const curly_list_type* drum_map_ptr);
		void BuildVelocityLayer(std::vector<VelocityLayer>& layers, std::shared_ptr<Value>& layer_sh_ptr);
		void BuildRobin(std::vector<Robin>& robins, NameValue* robin_nv_ptr);
	};

}; // end of namespace
