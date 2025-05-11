#include "xPES_PacketHeader.h"
#include "tsTransportStream.h"  // dla xTS::BaseClockFrequency_Hz

void xPES_PacketHeader::Reset() {
    m_PacketStartCodePrefix = 0;
    m_StreamId = 0;
    m_PacketLength = 0;
    m_PESHeaderDataLength = 0;
    m_hasPTS = false;
    m_hasDTS = false;
    m_PTS = 0;
    m_DTS = 0;
}

int32_t xPES_PacketHeader::Parse(const uint8_t* Input) {
    // 1) Odczytanie 3-bajtowego prefiksu startowego PES (powinien być zawsze 0x00 00 01)
    m_PacketStartCodePrefix =
        (uint32_t(Input[0]) << 16) |   // bajt 0 → bity 23–16
        (uint32_t(Input[1]) << 8) |   // bajt 1 → bity 15–8
        uint32_t(Input[2]);           // bajt 2 → bity 7–0

    // 2) Odczyt stream_id (bajt 3)
    m_StreamId = Input[3];

    // 3) Odczyt 2-bajtowej długości całego PES (bajty 4–5)
    m_PacketLength = (uint16_t(Input[4]) << 8)
        | uint16_t(Input[5]);

    // 4) Pobranie dwóch bajtów flag i długości części opcjonalnej
    uint8_t flags1 = Input[6];            // ogólne flagi (nieużywane tutaj)
    uint8_t flags2 = Input[7];            // tu są bity PTS/DTS
    m_PESHeaderDataLength = Input[8];     // ile bajtów opcji po tych trzech

    // 5) Wyciągnięcie informacji o obecności PTS/DTS:
    //    bity 7–6 w flags2 mówią:
    //      10b (2) → tylko PTS
    //      11b (3) → PTS i DTS
    uint8_t ptsDtsFlags = (flags2 >> 6) & 0x03;

    // 6) Indeks pierwszego bajtu danych opcjonalnych (poz. 9)
    int idx = 9;

    // 7) Jeśli jest PTS (flaga 2 lub 3), to odczytujemy 5 bajtów:
    if (ptsDtsFlags == 2 || ptsDtsFlags == 3) {
        m_hasPTS = true;
        // składamy 33-bitową wartość PTS z rozrzuconych fragmentów:
        m_PTS =
            (uint64_t(Input[idx + 0] & 0x0E) << 29) |  // 3 bity + marker
            (uint64_t(Input[idx + 1]) << 22) |  // kolejne 8 bitów
            (uint64_t(Input[idx + 2] & 0xFE) << 14) |  // 7 bitów + marker
            (uint64_t(Input[idx + 3]) << 7) |  // kolejne 8 bitów
            (uint64_t(Input[idx + 4]) >> 1);   // 7 bitów + marker
        idx += 5;  // przesuwamy wskaźnik o te 5 bajtów
    }

    // 8) Jeśli ponadto mamy DTS (flaga == 3), czytamy kolejne 5 bajtów tak samo:
    if (ptsDtsFlags == 3) {
        m_hasDTS = true;
        m_DTS =
            (uint64_t(Input[idx + 0] & 0x0E) << 29) |
            (uint64_t(Input[idx + 1]) << 22) |
            (uint64_t(Input[idx + 2] & 0xFE) << 14) |
            (uint64_t(Input[idx + 3]) << 7) |
            (uint64_t(Input[idx + 4]) >> 1);
        idx += 5;
    }

    return 0;
}


void xPES_PacketHeader::Print() const {
    // Wyświetlamy pola PES w formacie dziesiętnym
    printf(
        "PES: PSCP=%u SID=%u L=%u",
        m_PacketStartCodePrefix,
        m_StreamId,
        unsigned(m_PacketLength)
    );
    if (m_hasPTS) {
        double pts_s = double(m_PTS) / xTS::BaseClockFrequency_Hz;
        printf(" PTS=%.3f", pts_s);
    }
    if (m_hasDTS) {
        double dts_s = double(m_DTS) / xTS::BaseClockFrequency_Hz;
        printf(" DTS=%.3f", dts_s);
    }
}