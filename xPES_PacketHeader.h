#pragma once
#include <cstdint>
#include <cstdio>

class xPES_PacketHeader {
public:
    enum eStreamId : uint8_t {
        eStreamId_program_stream_map = 0xBC,
        eStreamId_padding_stream = 0xBE,
        eStreamId_private_stream_2 = 0xBF,
        eStreamId_ECM = 0xF0,
        eStreamId_EMM = 0xF1,
        eStreamId_program_stream_directory = 0xFF,
        eStreamId_DSMCC_stream = 0xF2,
        eStreamId_ITUT_H222_1_type_E = 0xF8,
    };

protected:
    uint32_t m_PacketStartCodePrefix;
    uint8_t  m_StreamId;
    uint16_t m_PacketLength;
    uint8_t  m_PESHeaderDataLength;

    // nowe pola
    bool     m_hasPTS = false;
    bool     m_hasDTS = false;
    uint64_t m_PTS = 0;
    uint64_t m_DTS = 0;

public:
    void     Reset();
    int32_t  Parse(const uint8_t* Input);
    void     Print() const;

    uint32_t getPacketStartCodePrefix() const { return m_PacketStartCodePrefix; }
    uint8_t  getStreamId()              const { return m_StreamId; }
    uint16_t getPacketLength()          const { return m_PacketLength; }
    uint8_t  getPESHeaderDataLength()   const { return m_PESHeaderDataLength; }
    bool     hasPTS()                   const { return m_hasPTS; }
    bool     hasDTS()                   const { return m_hasDTS; }
    uint64_t getPTS()                   const { return m_PTS; }
    uint64_t getDTS()                   const { return m_DTS; }
    uint16_t computeFullHeaderLength()  const { return uint16_t(6 + 3 + m_PESHeaderDataLength); }
};
