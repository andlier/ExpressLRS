#include <cstdint>
#include <iostream>
#include <unity.h>
#include "common.h"
#include "crsf2msp.h"

using namespace std;

CROSSFIRE2MSP crsf2msp;

const uint8_t CRSFdata1[] = {0x00, 0x0C, 0x7A, 0xC8, 0xEA, 0x31, 0x1E, 0xCA, 0x29, 0x28, 0x1E, 0x3A, 0x32};
const uint8_t CRSFdata2[] = {0x00,0x0C,0x7A,0xC8,0xEA,0x22,0x23,0x46,0x2D,0x14,0x32,0x00,0x00};

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    // std::cout << "data not large enough\n";

    crsf2msp.parse(CRSFdata2, sizeof(CRSFdata1));

    if (crsf2msp.isFrameReady())
    {
        cout << "Frame ready\n";
        cout << "Frame Length: " << (int)crsf2msp.getFrameLen() << "\n";

        const uint8_t *array = crsf2msp.getFrame();

        for (int i = 0; i < crsf2msp.getFrameLen(); i++)
        {
            cout << "0x" << hex << (int)array[i] << " ";
        }
    }
    else
    {
        cout << "Frame not ready\n";
    }

    UNITY_END();

    return 0;
}