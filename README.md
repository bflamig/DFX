# DFX
Drum Font Exchange Format

DFX is a new exchange format for "drum fonts", inspired in part by the "sound font" technology that allows for the management of wave samples that make up an instruments sound. I invented the DFX format to be specifically for midi drum kits on small single-board computers such as the Raspberry Pi. Because of this, the DFX format is greatly simplified from its more general sound font inspiration. Also, I chose a Json-like syntax (easier to read than Json, but with a one-to-one mapping) as I find the sound font syntax hard to follow and a bit adhoc. I'm sure that's just an opinion :)

The basis of this Json-like syntax is a language I call "Bryx" which stands for "Bryan Exchange Format". This format is general and can (and will) be used for applications besides just drum fonts. So it is designed to be general purpose. The DFX language sits on top of Bryx, and can be thought of as a schema specifically for drum fonts.

Due to the one-to-one mapping with Json, Bryx files can be easily translated into Json, and vice versa. That's for ease of use down the road where applications may have access to a ready-to-go Json parser, but not a Bryx parser. I plan on keeping my DFX files in Bryx format because they are simply easier to read and write.

Right now, this code is being developed in Visual Studio, my IDE of choice. As such, it uses Visual Studio project files, not make files. That may change in the future. It is written to the C++ 17 standard, and strives to fully utilize the new features of that standard to their advantage.

This code, as of 09/17/2020, is in an "early days" state. It's not by any means complete, and the drum font features are bare-bones. Key features are velocity layers and round-robin sampling. The plan is to keep this technology simple and not get bogged down with too many features. The goal is for this work on computers like the Raspberry Pi. Other features probably added in the future: reverb and compression support, and that's about it. Also to be added in the future is a full support for actually playing the drum samples on Mac (Core audio), Windows (ASIO), and Linux (Jack).

UPDATE 12/6/2020: ASIO support has been added, and a simple drum player test program has proven that this technology can work.
