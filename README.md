MPEG-2 TS → MP2 Audio Extractor
A lightweight C++ tool that parses an MPEG-2 Transport Stream and pulls out a pure MP2 audio file from a specified PID.

Features
TS Packet Parsing

Processes 188-byte TS packets

Decodes all header fields (Sync Byte, Error Indicator, Payload Start, Priority, PID, Scrambling, Adaptation Field control, Continuity Counter)

Adaptation Field Handling

Parses when present (AFC=2 or 3)

Extracts and displays flags (Discontinuity, Random Access, Priority, PCR/OPCR with time in seconds, Splice, Private Data, Extension)

Counts stuffing bytes

PES Assembly & MP2 Extraction

Monitors a single audio PID (default 136)

Detects PES start via Payload Unit Start

Buffers payload bytes until the PES_packet_length is reached

Reports “Started”, “Continue”, “Finished” or “PacketLost” for each PES

Strips off the PES header (typically 14 bytes) and writes only raw MP2 frames to PID136.mp2

PTS Reporting

If present in the PES header, decodes Presentation Time Stamp (PTS) and prints it in seconds for diagnostics
