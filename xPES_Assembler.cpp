#include "xPES_Assembler.h"
#include <cstdio>
#include <cstring>

// Konstruktor – inicjalizuje pola stanu
xPES_Assembler::xPES_Assembler() {
    m_Buffer = nullptr;              // wskaŸnik na dynamiczny bufor PES
    m_BufferSize = 0;                // aktualny rozmiar bufora
    m_DataOffset = 0;                // ile danych ju¿ w buforze
    m_LastContinuityCounter = -1;    // ostatni CC; -1 oznacza “jeszcze nic nie przysz³o”
    m_Started = false;               // czy aktualny PES zosta³ rozpoczêty?
}

// Destruktor – zwalnia bufor, jeœli istnieje
xPES_Assembler::~xPES_Assembler() {
    if (m_Buffer)
        delete[] m_Buffer;
}

// Init – ustawiamy PID, którego PES-y bêdziemy sk³adaæ, i czyœcimy bufor
void xPES_Assembler::Init(int32_t PID) {
    m_PID = PID;
    xBufferReset();
}

// xBufferReset – usuwamy stary bufor i alokujemy nowy o minimalnym rozmiarze
void xPES_Assembler::xBufferReset() {
    if (m_Buffer)
        delete[] m_Buffer;
    // startujemy z rozmiarem jednego pakietu TS (188 bajtów)
    m_Buffer = new uint8_t[xTS::TS_PacketLength];
    m_BufferSize = xTS::TS_PacketLength;
    m_DataOffset = 0;
}

// xBufferAppend – dopisuje 'Size' bajtów z 'Data' do naszego bufora,
// automatycznie realokuj¹c go, jeœli nie starcza miejsca
void xPES_Assembler::xBufferAppend(const uint8_t* Data, int32_t Size) {
    if (m_DataOffset + Size > m_BufferSize) {
        // realokacja: nowy rozmiar dok³adnie tyle, ile potrzeba
        uint8_t* newBuffer = new uint8_t[m_DataOffset + Size];
        memcpy(newBuffer, m_Buffer, m_DataOffset);
        delete[] m_Buffer;
        m_Buffer = newBuffer;
        m_BufferSize = m_DataOffset + Size;
    }
    memcpy(m_Buffer + m_DataOffset, Data, Size);
    m_DataOffset += Size;
}

// AbsorbPacket – g³ówna funkcja: wrzucamy do niej ka¿dy TS, a ona sk³ada PES-y
xPES_Assembler::eResult xPES_Assembler::AbsorbPacket(
    const uint8_t* TransportStreamPacket,
    const xTS_PacketHeader* PacketHeader,
    const xTS_AdaptationField* AdaptationField)
{
    // 1) SprawdŸ, czy to nasz PID
    if (PacketHeader->getPID() != m_PID)
        return eResult::UnexpectedPID;

    // 2) SprawdŸ CC (ci¹g³oœæ). Jeœli siê nie zgadza, traktujemy jako utratê pakietu.
    if (m_LastContinuityCounter != -1 &&
        PacketHeader->getContinuityCounter() != (m_LastContinuityCounter + 1) % 16)
        return eResult::StreamPackedLost;

    // 3) Oblicz offset pocz¹tku payloadu w pakiecie TS:
    //    - najpierw 4 bajty nag³ówka TS
    //    - jeœli jest AF, to (AF_length + 1) bajtów
    int offset = xTS::TS_HeaderLength;
    if (PacketHeader->getAdaptationFieldControl() > 1)
        offset += (*(TransportStreamPacket + xTS::TS_HeaderLength)) + 1;

    int payloadLength = xTS::TS_PacketLength - offset;

    // 4) SprawdŸ S-flagê (Payload Unit Start); to oznacza pocz¹tek nowego PES
    if (PacketHeader->getPayloadUnitStartIndicator() == 1) {
        if (m_Started) {
            //    a) jeœli ju¿ montowaliœmy PES, to:
            //       - dopisz koñcówkê
            xBufferAppend(TransportStreamPacket + offset, payloadLength);
            m_LastContinuityCounter = PacketHeader->getContinuityCounter();

            //       - jeœli osi¹gnêliœmy oczekiwan¹ d³ugoœæ PES, koñczymy
            if (m_ExpectedPESLength > 0 && m_DataOffset >= m_ExpectedPESLength) {
                m_Started = false;
                return eResult::AssemblingFinished;
            }
            //       - mimo ¿e length nie zosta³ osi¹gniêty, traktujemy to jako zakoñczenie
            m_Started = false;
            return eResult::AssemblingFinished;
        } else {
            //    b) je¿eli to pierwszy fragment nowego PES:
            m_Started = true;
            m_PESH.Reset();
            // odczytaj nag³ówek PES, by poznaæ m_ExpectedPESLength
            m_PESH.Parse(TransportStreamPacket + offset);
            m_ExpectedPESLength = m_PESH.getPacketLength();
            // wyczyœæ i rozpocznij nowy bufor
            xBufferReset();
            xBufferAppend(TransportStreamPacket + offset, payloadLength);
            m_LastContinuityCounter = PacketHeader->getContinuityCounter();

            // je¿eli PES ma zero d³ugoœci albo payload ju¿ spe³nia length, od razu zakoñcz
            if (m_ExpectedPESLength > 0 && m_DataOffset >= m_ExpectedPESLength)
                return eResult::AssemblingFinished;
            return eResult::AssemblingStarted;
        }
    } else {
        // 5) Je¿eli to nie pocz¹tek PES, po prostu dopisz payload
        xBufferAppend(TransportStreamPacket + offset, payloadLength);
        m_LastContinuityCounter = PacketHeader->getContinuityCounter();

        // je¿eli ju¿ zebraliœmy wszystkie dane – koniec
        if (m_ExpectedPESLength > 0 && m_DataOffset >= m_ExpectedPESLength) {
            m_Started = false;
            return eResult::AssemblingFinished;
        }
        return eResult::AssemblingContinue;
    }
}
