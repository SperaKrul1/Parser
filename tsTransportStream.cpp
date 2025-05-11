#include "tsTransportStream.h"
#include "tsCommon.h"
#include <cstdio>
#include <cstring>

void xTS_PacketHeader::Reset() {
    m_SB = m_E = m_S = m_T = 0;
    m_PID = 0;
    m_TSC = m_AFC = m_CC = 0;
}

int32_t xTS_PacketHeader::Parse(const uint8_t* Input) {
    uint32_t header = *((const uint32_t*)Input);
    header = xSwapBytes32(header);      // zmiana strumenia na big endian
    m_SB = (header >> 24) & 0xFF;      // maskuje 8 bit�w:    0b11111111
    m_E = (header >> 23) & 0x01;      // maskuje 1 bit:      0b00000001
    m_S = (header >> 22) & 0x01;      // maskuje 1 bit:      0b00000001
    m_T = (header >> 21) & 0x01;      // maskuje 1 bit:      0b00000001
    m_PID = (header >> 8) & 0x1FFF;    // maskuje 13 bit�w:   0b000111111111111
    m_TSC = (header >> 6) & 0x03;      // maskuje 2 bity:     0b00000011
    m_AFC = (header >> 4) & 0x03;      // maskuje 2 bity:     0b00000011
    m_CC = header & 0x0F;     // maskuje 4 bity:     0b00001111
    return 0;
}

void xTS_PacketHeader::Print() const {
    printf("TS: SB= %d E= %d S= %d P= %d PID= %d TSC= %d AF= %d CC= %d ",
        m_SB, m_E, m_S, m_T, m_PID, m_TSC, m_AFC, m_CC);
}

void xTS_AdaptationField::Reset() {
    // Adaptation-field control bit okre�la obecno�� AF i payloadu
    m_AdaptationFieldControl = 0;

    // D�ugo�� pola AF 
    m_AdaptationFieldLength = 0;

    m_DiscontinuityIndicator = false;  // przerwanie ci�g�o�ci strumienia
    m_RandomAccessIndicator = false;  // pocz�tek nowego punktu dost�pu
    m_ElementaryStreamPriorityIndicator = false;  // priorytet strumienia
    m_PCRFlag = false;  // obecno�� pola PCR
    m_OPCRFlag = false;  // obecno�� pola OPCR
    m_SplicingPointFlag = false;  // splice point
    m_TransportPrivateDataFlag = false;  // prywatne dane transportu
    m_AdaptationFieldExtensionFlag = false;  // dodatkowe rozszerzenie AF

    // Warto�ci PCR/OPCR
    m_PCR = 0;
    m_OPCR = 0;

    // Liczba bajt�w �wype�nienia� (stuffing) � zero
    m_StuffingBytes = 0;
}
int32_t xTS_AdaptationField::Parse(const uint8_t* PacketBuffer, uint8_t AdaptationFieldControl) {
    // Je�li brak Adaptation Field (AFC != 2 lub 3), nic nie robimy
    if (!(AdaptationFieldControl == 2 || AdaptationFieldControl == 3))
        return 0;

    m_AdaptationFieldControl = AdaptationFieldControl;
    // Wska�nik na pierwszy bajt Adaptation Field (zaraz po 4-bajtowym nag��wku TS)
    const uint8_t* AF_ptr = PacketBuffer + xTS::TS_HeaderLength;
    // Pierwszy bajt to d�ugo�� Adaptation Field (bez samego bajtu d�ugo�ci)
    m_AdaptationFieldLength = *AF_ptr;

    if (m_AdaptationFieldLength > 0) {
        // Drugi bajt to zbi�r flag (DC, RA, SP, PCR, OPCR, SF, TP, EX)
        uint8_t flags = *(AF_ptr + 1);
        m_DiscontinuityIndicator = (flags >> 7) & 0x01;  // DC
        m_RandomAccessIndicator = (flags >> 6) & 0x01;  // RA
        m_ElementaryStreamPriorityIndicator = (flags >> 5) & 0x01; // SP
        m_PCRFlag = (flags >> 4) & 0x01;  // PCR present?
        m_OPCRFlag = (flags >> 3) & 0x01;  // OPCR present?
        m_SplicingPointFlag = (flags >> 2) & 0x01;  // splice point
        m_TransportPrivateDataFlag = (flags >> 1) & 0x01;  // private data
        m_AdaptationFieldExtensionFlag = flags & 0x01;  // AF extension

        int index = 1;  // pomijamy bajt d�ugo�ci + bajt flag
        // Je�li jest PCR, odczytaj 6 bajt�w: 33-bit base + 9-bit extension
        if (m_PCRFlag && m_AdaptationFieldLength >= index + 6) {
            // 1) Budujemy 33-bitow� cz�� bazow� (base)
            uint64_t base =
                (uint64_t(PacketBuffer[4 + index + 0]) << 25) |  // bajt 0 ? bity 32..25
                (uint64_t(PacketBuffer[4 + index + 1]) << 17) |  // bajt 1 ? bity 24..17
                (uint64_t(PacketBuffer[4 + index + 2]) << 9) |  // bajt 2 ? bity 16..9
                (uint64_t(PacketBuffer[4 + index + 3]) << 1) |  // bajt 3 ? bity 8..1
                (uint64_t(PacketBuffer[4 + index + 4]) >> 7);   // bajt 4 (bit 7) ? bit 0

            // 2) Budujemy 9-bitow� cz�� rozszerzenia (extension)
            uint16_t ext =
                ((PacketBuffer[4 + index + 4] & 0x01) << 8) |   // bajt 4 (bit 0) ? bit 8
                PacketBuffer[4 + index + 5];                   // bajt 5 ? bity 7..0

            // 3) Ca�y PCR w jednostkach 27 MHz = base*300 + ext
            m_PCR = base * 300 + ext;

            index += 6;  // przesuwamy wska�nik o te 6 bajt�w PCR
        }
        // Analogicznie odczyt OPCR, je�li jest
        if (m_OPCRFlag && m_AdaptationFieldLength >= index + 6) {
            uint64_t base =
                (uint64_t(PacketBuffer[4 + index + 0]) << 25) |
                (uint64_t(PacketBuffer[4 + index + 1]) << 17) |
                (uint64_t(PacketBuffer[4 + index + 2]) << 9) |
                (uint64_t(PacketBuffer[4 + index + 3]) << 1) |
                (uint64_t(PacketBuffer[4 + index + 4]) >> 7);
            uint16_t ext =
                ((PacketBuffer[4 + index + 4] & 0x01) << 8) |
                PacketBuffer[4 + index + 5];
            m_OPCR = base * 300 + ext;
            index += 6;
        }
        // Pozosta�e bajty to wype�nienie (stuffing)
        if (m_AdaptationFieldLength > index)
            m_StuffingBytes = m_AdaptationFieldLength - index;
    }

    // Zwracamy liczb� bajt�w Adaptation Field = d�ugo�� + 1 bajt d�ugo�ci
    return static_cast<int32_t>(m_AdaptationFieldLength) + 1;
}


void xTS_AdaptationField::Print() const {
    printf("AF: L %d ", m_AdaptationFieldLength);
    if (m_AdaptationFieldLength > 0) {
        printf("DC=%d RA=%d SP=%d PR=%d OR=%d SP=%d TP=%d EX=%d ",
            m_DiscontinuityIndicator,
            m_RandomAccessIndicator,
            m_ElementaryStreamPriorityIndicator,
            m_PCRFlag,
            m_OPCRFlag,
            m_SplicingPointFlag,
            m_TransportPrivateDataFlag,
            m_AdaptationFieldExtensionFlag);
        if (m_PCRFlag) {
            // Aby otrzyma� czas w sekundach, dzielimy liczb� tych takt�w przez cz�stotliwo�� zegara (27 MHz = 27 000 000 Hz).
            double pcr_seconds = static_cast<double>(m_PCR) / xTS::ExtendedClockFrequency_Hz;
            printf("PCR:%llu (%.6f sec) ", m_PCR, pcr_seconds);
        }
        if (m_OPCRFlag) {
            double opcr_seconds = static_cast<double>(m_OPCR) / xTS::ExtendedClockFrequency_Hz;
            printf("OPCR:%llu (%.6f sec) ", m_OPCR, opcr_seconds);
        }
        printf("Stuffing=%d ", m_StuffingBytes);
    }
}
