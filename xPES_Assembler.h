#pragma once
#include "xPES_PacketHeader.h"
#include "tsTransportStream.h"
#include <cstdint>

class xPES_Assembler {
public:
    enum class eResult : int32_t {
        UnexpectedPID = 1,
        StreamPackedLost,
        AssemblingStarted,
        AssemblingContinue,
        AssemblingFinished,
    };
protected:
    int32_t m_PID;
    uint8_t* m_Buffer;
    uint32_t m_BufferSize;
    uint32_t m_DataOffset;
    int8_t m_LastContinuityCounter;
    uint16_t m_ExpectedPESLength;
    bool m_Started;
    xPES_PacketHeader m_PESH;
    uint32_t m_MaxBufferSize;
public:
    xPES_Assembler();
    ~xPES_Assembler();
    void Init(int32_t PID);
    eResult AbsorbPacket(const uint8_t* TransportStreamPacket, const xTS_PacketHeader* PacketHeader, const xTS_AdaptationField* AdaptationField);
    void PrintPESH() const { m_PESH.Print(); }
    uint8_t* getPacket() { return m_Buffer; }
    int32_t getNumPacketBytes() const { return m_DataOffset; }
    const xPES_PacketHeader& getPESH() const { return m_PESH; }
protected:
    void xBufferReset();
    void xBufferAppend(const uint8_t* Data, int32_t Size);
};
