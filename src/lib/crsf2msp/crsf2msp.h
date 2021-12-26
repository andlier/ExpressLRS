#pragma once

#include <cstdint>
#include <cstring>
#include "logging.h"

/*  Takes a CRSF(MSP) frame and converts it to raw MSP frame
    adding the MSP header and checksum. Handles chunked MSP messages.
*/

#ifndef CRSF_MAX_PACKET_LEN
#define CRSF_MAX_PACKET_LEN 64 // Max CRSF packet length
#endif
#define CRSF_MSP_FRAME_OFFSET 6                  // Start of MSP frame in CRSF packet
#define CRSF_MSP_STATUS_BYTE_OFFSET 5            // Status byte index in CRSF packet
#define CRSF_MSP_SRC_OFFSET 4                    // Source ID index in CRSF packet
#define CRSF_MSP_DEST_OFFSET 3                   // MSP type index in CRSF packet
#define CRSF_FRAME_PAYLOAD_LEN_IDX 1         // Extended frame payload length index in CRSF packet
#define CRSF_EXT_FRAME_PAYLOAD_LEN_SIZE_OFFSET 5 // For Ext Frame, playload is this much bigger than it says in the crsf frame
#define CRSF_MSP_MAX_BYTES_PER_CHUNK 57          // Max bytes per MSP chunk in CRSF packet
#define CRSF_MSP_TYPE_IDX 2                      // MSP type index in CRSF packet
#define MSP_FRAME_MAX_LEN 512                    // Max MSP frame length (increase as needed)

class CROSSFIRE2MSP
{
private:
    uint8_t outBuffer[MSP_FRAME_MAX_LEN];
    uint32_t pktLen;    // packet length of the incomming msp frame
    uint32_t idx; // number of bytes received in the current msp frame
    bool frameComplete;
    uint8_t src;  // source of the msp frame (from CRSF ext header)
    uint8_t dest; // destination of the msp frame (from CRSF ext header)

    bool isNewFrame(uint8_t data);
    bool isError(uint8_t data);

    uint8_t getSeqNumber(uint8_t data);
    uint8_t getVersion(uint8_t data);
    uint8_t getHeaderDir(uint8_t data);

public:
    CROSSFIRE2MSP();
    void parse(const uint8_t *data, uint8_t len); // accept crsf frame input
    bool isFrameReady();
    const uint8_t *getFrame();
    uint8_t getFrameLen();
    void resetFrame();
    uint8_t getSrc();
    uint8_t getDest();
};