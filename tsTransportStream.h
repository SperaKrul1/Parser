#pragma once
#include "tsCommon.h"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

/*
MPEG-TS packet:
`        3                   2                   1                   0  `
`      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   0 |                             Header                            | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   4 |                  Adaptation field + Payload                   | `
`     |                                                               | `
` 184 |                                                               | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `


MPEG-TS packet header:
`        3                   2                   1                   0  `
`      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0  `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `
`   0 |       SB      |E|S|T|           PID           |TSC|AFC|   CC  | `
`     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ `

Sync byte                    (SB ) :  8 bits
Transport error indicator    (E  ) :  1 bit
Payload unit start indicator (S  ) :  1 bit
Transport priority           (T  ) :  1 bit
Packet Identifier            (PID) : 13 bits
Transport scrambling control (TSC) :  2 bits
Adaptation field control     (AFC) :  2 bits
Continuity counter           (CC ) :  4 bits
*/


//=============================================================================================================================================================================

class xTS {
public:
    static constexpr uint32_t TS_PacketLength = 188;
    static constexpr uint32_t TS_HeaderLength = 4;
    static constexpr uint32_t PES_HeaderLength = 6;
    static constexpr uint32_t BaseClockFrequency_Hz = 90000;
    static constexpr uint32_t ExtendedClockFrequency_Hz = 27000000;
    static constexpr uint32_t BaseClockFrequency_kHz = 90;
    static constexpr uint32_t ExtendedClockFrequency_kHz = 27000;
    static constexpr uint32_t BaseToExtendedClockMultiplier = 300;
};

class xTS_PacketHeader {
public:
    enum class ePID : uint16_t {
        PAT = 0x0000,
        CAT = 0x0001,
        TSDT = 0x0002,
        IPMT = 0x0003,
        NIT = 0x0010,
        SDT = 0x0011,
        NuLL = 0x1FFF,
    };
protected:
    uint8_t  m_SB;
    uint8_t  m_E;
    uint8_t  m_S;
    uint8_t  m_T;
    uint16_t m_PID;
    uint8_t  m_TSC;
    uint8_t  m_AFC;
    uint8_t  m_CC;
public:
    void Reset();
    int32_t Parse(const uint8_t* Input);
    void Print() const;
    uint8_t  getSyncByte() const { return m_SB; }
    uint8_t  getErrorIndicator() const { return m_E; }
    uint8_t  getPayloadUnitStartIndicator() const { return m_S; }
    uint8_t  getTransportPriority() const { return m_T; }
    uint16_t getPID() const { return m_PID; }
    uint8_t  getTransportScramblingControl() const { return m_TSC; }
    uint8_t  getAdaptationFieldControl() const { return m_AFC; }
    uint8_t  getContinuityCounter() const { return m_CC; }
};

class xTS_AdaptationField {
protected:
    uint8_t m_AdaptationFieldControl;
    uint64_t m_PCR = 0, m_OPCR = 0;
    uint8_t  m_StuffingBytes = 0;
    uint8_t m_AdaptationFieldLength;
    bool m_DiscontinuityIndicator;
    bool m_RandomAccessIndicator;
    bool m_ElementaryStreamPriorityIndicator;
    bool m_PCRFlag;
    bool m_OPCRFlag;
    bool m_SplicingPointFlag;
    bool m_TransportPrivateDataFlag;
    bool m_AdaptationFieldExtensionFlag;
public:
    void Reset();
    int32_t Parse(const uint8_t* PacketBuffer, uint8_t AdaptationFieldControl);
    void Print() const;
    uint8_t getAdaptationFieldLength() const { return m_AdaptationFieldLength; }
};

//=============================================================================================================================================================================