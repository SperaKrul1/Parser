# MPEG-2 TS → MP2 Audio Extractor

A lightweight C++ tool that parses an MPEG-2 Transport Stream and extracts a pure MP2 audio file from a specified PID.

---

## Features

### TS Packet Parsing
- Processes 188-byte TS packets  
- Decodes all header fields:
  - **Sync Byte**  
  - **Error Indicator**  
  - **Payload Unit Start**  
  - **Transport Priority**  
  - **PID**  
  - **Scrambling Control**  
  - **Adaptation Field Control**  
  - **Continuity Counter**

### Adaptation Field Handling
- Parses adaptation field when present (AFC = 2 or 3)  
- Extracts and displays flags:
  - **Discontinuity**  
  - **Random Access**  
  - **Elementary Stream Priority**  
  - **PCR** / **OPCR** (with time in seconds)  
  - **Splice Point**  
  - **Private Data**  
  - **Extension**  
- Counts stuffing bytes

### PES Assembly & MP2 Extraction
- Monitors a single audio PID (default: **136**)  
- Detects PES start via **Payload Unit Start** indicator  
- Buffers payload bytes until the `PES_packet_length` is reached  
- Reports one of:
  - **Started**  
  - **Continue**  
  - **Finished**  
  - **PacketLost**  
- Strips off the PES header (typically 14 bytes)  
- Writes only raw MP2 frames to `PID136.mp2`

### PTS Reporting
- If present in the PES header, decodes Presentation Time Stamp (PTS)  
- Prints PTS in seconds for diagnostics

---
