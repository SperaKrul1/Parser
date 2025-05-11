#include "xPES_Assembler.h"
#include <cstdio>
#include <cstring>

// Konstruktor � inicjalizuje pola stanu
xPES_Assembler::xPES_Assembler() {
    m_Buffer = nullptr;              // wska�nik na dynamiczny bufor PES
    m_BufferSize = 0;                // aktualny rozmiar bufora
    m_DataOffset = 0;                // ile danych ju� w buforze
    m_LastContinuityCounter = -1;    // ostatni CC; -1 oznacza �jeszcze nic nie przysz�o�
    m_Started = false;               // czy aktualny PES zosta� rozpocz�ty?
}

// Destruktor � zwalnia bufor, je�li istnieje
xPES_Assembler::~xPES_Assembler() {
    if (m_Buffer)
        delete[] m_Buffer;
}

// Init � ustawiamy PID, kt�rego PES-y b�dziemy sk�ada�, i czy�cimy bufor
void xPES_Assembler::Init(int32_t PID) {
    m_PID = PID;
    xBufferReset();
}

// xBufferReset � usuwamy stary bufor i alokujemy nowy o minimalnym rozmiarze
void xPES_Assembler::xBufferReset() {
    if (m_Buffer)
        delete[] m_Buffer;
    // startujemy z rozmiarem jednego pakietu TS (188 bajt�w)
    m_Buffer = new uint8_t[xTS::TS_PacketLength];
    m_BufferSize = xTS::TS_PacketLength;
    m_DataOffset = 0;
}

// xBufferAppend � dopisuje 'Size' bajt�w z 'Data' do naszego bufora,
// automatycznie realokuj�c go, je�li nie starcza miejsca
void xPES_Assembler::xBufferAppend(const uint8_t* Data, int32_t Size) {
    if (m_DataOffset + Size > m_BufferSize) {
        // realokacja: nowy rozmiar dok�adnie tyle, ile potrzeba
        uint8_t* newBuffer = new uint8_t[m_DataOffset + Size];
        memcpy(newBuffer, m_Buffer, m_DataOffset);
        delete[] m_Buffer;
        m_Buffer = newBuffer;
        m_BufferSize = m_DataOffset + Size;
    }
    memcpy(m_Buffer + m_DataOffset, Data, Size);
    m_DataOffset += Size;
}

// AbsorbPacket � g��wna funkcja: wrzucamy do niej ka�dy TS, a ona sk�ada PES-y
xPES_Assembler::eResult xPES_Assembler::AbsorbPacket(
    const uint8_t* TransportStreamPacket,
    const xTS_PacketHeader* PacketHeader,
    const xTS_AdaptationField* AdaptationField)
{
    // 1) Sprawd�, czy to nasz PID
    if (PacketHeader->getPID() != m_PID)
        return eResult::UnexpectedPID;

    // 2) Sprawd� CC (ci�g�o��). Je�li si� nie zgadza, traktujemy jako utrat� pakietu.
    if (m_LastContinuityCounter != -1 &&
        PacketHeader->getContinuityCounter() != (m_LastContinuityCounter + 1) % 16)
        return eResult::StreamPackedLost;

    // 3) Oblicz offset pocz�tku payloadu w pakiecie TS:
    //    - najpierw 4 bajty nag��wka TS
    //    - je�li jest AF, to (AF_length + 1) bajt�w
    int offset = xTS::TS_HeaderLength;
    if (PacketHeader->getAdaptationFieldControl() > 1)
        offset += (*(TransportStreamPacket + xTS::TS_HeaderLength)) + 1;

    int payloadLength = xTS::TS_PacketLength - offset;

    // 4) Sprawd� S-flag� (Payload Unit Start); to oznacza pocz�tek nowego PES
    if (PacketHeader->getPayloadUnitStartIndicator() == 1) {
        if (m_Started) {
            //    a) je�li ju� montowali�my PES, to:
            //       - dopisz ko�c�wk�
            xBufferAppend(TransportStreamPacket + offset, payloadLength);
            m_LastContinuityCounter = PacketHeader->getContinuityCounter();

            //       - je�li osi�gn�li�my oczekiwan� d�ugo�� PES, ko�czymy
            if (m_ExpectedPESLength > 0 && m_DataOffset >= m_ExpectedPESLength) {
                m_Started = false;
                return eResult::AssemblingFinished;
            }
            //       - mimo �e length nie zosta� osi�gni�ty, traktujemy to jako zako�czenie
            m_Started = false;
            return eResult::AssemblingFinished;
        } else {
            //    b) je�eli to pierwszy fragment nowego PES:
            m_Started = true;
            m_PESH.Reset();
            // odczytaj nag��wek PES, by pozna� m_ExpectedPESLength
            m_PESH.Parse(TransportStreamPacket + offset);
            m_ExpectedPESLength = m_PESH.getPacketLength();
            // wyczy�� i rozpocznij nowy bufor
            xBufferReset();
            xBufferAppend(TransportStreamPacket + offset, payloadLength);
            m_LastContinuityCounter = PacketHeader->getContinuityCounter();

            // je�eli PES ma zero d�ugo�ci albo payload ju� spe�nia length, od razu zako�cz
            if (m_ExpectedPESLength > 0 && m_DataOffset >= m_ExpectedPESLength)
                return eResult::AssemblingFinished;
            return eResult::AssemblingStarted;
        }
    } else {
        // 5) Je�eli to nie pocz�tek PES, po prostu dopisz payload
        xBufferAppend(TransportStreamPacket + offset, payloadLength);
        m_LastContinuityCounter = PacketHeader->getContinuityCounter();

        // je�eli ju� zebrali�my wszystkie dane � koniec
        if (m_ExpectedPESLength > 0 && m_DataOffset >= m_ExpectedPESLength) {
            m_Started = false;
            return eResult::AssemblingFinished;
        }
        return eResult::AssemblingContinue;
    }
}
