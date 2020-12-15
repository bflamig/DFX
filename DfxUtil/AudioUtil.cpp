//#include <sstream>
#include "AudioUtil.h"

namespace dfx
{
	std::string to_string(AudioResult r)
	{
		switch (r)
		{
		case AudioResult::NoError: return "everything fine";
		case AudioResult::STATUS: return "STATUS";
		case AudioResult::WARNING: return "WARNING";
		case AudioResult::DEBUG_PRINT: return "DEBUG_PRINT";
		case AudioResult::MEMORY_ALLOCATION: return "MEMORY_ALLOCATION";
		case AudioResult::MEMORY_ACCESS: return "MEMORY_ACCESS";
		case AudioResult::FUNCTION_ARGUMENT: return "FUNCTION_ARGUMENT";
		case AudioResult::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
		case AudioResult::FILE_UNKNOWN_FORMAT: return "FILE_UNKNOWN_FORMAT";
		case AudioResult::FILE_NAMING: return "FILE_NAMING";
		case AudioResult::FILE_CONFIGURATION: return "FILE_CONFIGURATION";
		case AudioResult::FILE_ERROR: return "FILE_ERROR";
		case AudioResult::PROCESS_THREAD: return "PROCESS_THREAD";
		case AudioResult::PROCESS_SOCKET: return "PROCESS_SOCKET";
		case AudioResult::PROCESS_SOCKET_IPADDR: return "PROCESS_SOCKET_IPADDR";
		case AudioResult::AUDIO_SYSTEM: return "AUDIO_SYSTEM";
		case AudioResult::MIDI_SYSTEM: return "MIDI_SYSTEM";
		case AudioResult::UNSPECIFIED:
		default: return "UNSPECIFIED";
		}
	}
}