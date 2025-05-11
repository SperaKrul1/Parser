#include "tsTransportStream.h"
#include "xPES_Assembler.h"
#include "xPES_PacketHeader.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>

using namespace std;

int main(int argc, char* argv[], char* envp[])
{
    // 1) Otwórz TS
    ifstream tsFile("example_new.ts", ios::binary);
    if (!tsFile) {
        cerr << "Nie mozna otworzyc pliku example_new.ts\n";
        return EXIT_FAILURE;
    }

    // 2) Przygotuj parsery i assembler
    const size_t TS_PACKET_SIZE = xTS::TS_PacketLength;
    uint8_t TS_PacketBuffer[TS_PACKET_SIZE];
    xTS_PacketHeader    TS_PacketHeader;
    xTS_AdaptationField TS_PacketAdaptationField;
    xPES_Assembler      PES_Assembler;
    PES_Assembler.Init(136);  // fonia na PID=136

    // 3) Otwórz plik wyjœciowy MP2
    FILE* out = fopen("PID136.mp2", "wb");
    if (!out) {
        cerr << "Nie mozna utworzyc PID136.mp2\n";
        return EXIT_FAILURE;
    }

    int32_t TS_PacketId = 0;
    const int32_t maxPackets = 10000000;  // d³ugoœc ile przetwarzam

    while (tsFile.read(reinterpret_cast<char*>(TS_PacketBuffer), TS_PACKET_SIZE)) {
        // a) parsuj nag³ówek TS
        TS_PacketHeader.Reset();
        TS_PacketHeader.Parse(TS_PacketBuffer);

        // b) interesuj¹cy nas PID?
        if (TS_PacketHeader.getPID() != 136) {
            ++TS_PacketId;
            continue;
        }

        // c) parsuj AF jeœli jest
        uint8_t afc = TS_PacketHeader.getAdaptationFieldControl();
        if (afc == 2 || afc == 3) {
            TS_PacketAdaptationField.Reset();
            TS_PacketAdaptationField.Parse(TS_PacketBuffer, afc);
        }

        // d) wyœwietl diagnostykê
        printf("%010d ", TS_PacketId);
        TS_PacketHeader.Print();
        if (afc > 1) {
            TS_PacketAdaptationField.Print();
        }

        // e) daj pakiet do assemblera
        auto result = PES_Assembler.AbsorbPacket(TS_PacketBuffer, &TS_PacketHeader, &TS_PacketAdaptationField);
        switch (result) {
        case xPES_Assembler::eResult::StreamPackedLost:
            printf("PcktLost ");
            break;

        case xPES_Assembler::eResult::AssemblingStarted:
            printf("Assembling Started ");
            PES_Assembler.PrintPESH();
            break;

        case xPES_Assembler::eResult::AssemblingContinue:
            printf("Assembling Continue ");
            break;

        case xPES_Assembler::eResult::AssemblingFinished:
        {
            // wydrukujemy ostateczn¹ d³ugoœæ i wypiszemy payload do pliku
            int32_t totalBytes = PES_Assembler.getNumPacketBytes();
            // oblicz header PES na podstawie pola PES_header_data_length
            // (zak³adamy, ¿e xPES_Assembler ma metodê getPESH(), a klasa PES_PacketHeader computeFullHeaderLength())
            const auto& pesh = PES_Assembler.getPESH();
            uint16_t headerLen = pesh.computeFullHeaderLength();
            int32_t payloadLen = max(0, totalBytes - headerLen);

            printf("Assembling Finished PES:  PcktLen=%d, HeadLen=%u,  DataLen=%d\n",
                totalBytes, headerLen, payloadLen);

            if (payloadLen > 0) {
                // zapisz sam payload (bez nag³ówka) do pliku .mp2
                fwrite(PES_Assembler.getPacket() + headerLen, 1, payloadLen, out);
            }
        }
        break;

        default:
            break;
        }

        printf("\n");
        ++TS_PacketId;
        if (TS_PacketId >= maxPackets) break;
    }

    fclose(out);
    tsFile.close();
    return EXIT_SUCCESS;
}
