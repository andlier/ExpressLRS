#include "crsf2msp.h"

CROSSFIRE2MSP::CROSSFIRE2MSP()
{
    // clean inital state
    memset(outBuffer, 0x1, MSP_FRAME_MAX_LEN);
    pktLen = 0;
    idx = 0;
    frameComplete = false;
}

void CROSSFIRE2MSP::parse(const uint8_t *data, uint8_t len)
{
    uint8_t statusByte = data[CRSF_MSP_STATUS_BYTE_OFFSET]; // status byte from CRSF MSP frame

    bool error = isError(statusByte);
    bool newFrame = isNewFrame(statusByte);
    uint8_t seqNum = getSeqNumber(statusByte);
    uint8_t MSPvers = getVersion(statusByte);

    if (error)
    {
        pktLen = 0;
        idx = 0;
        frameComplete = false;
        memset(outBuffer, 0, sizeof(outBuffer));
        DBGLN("MSP frame error bit set!");
        return;
    }

    uint8_t CRSFpayloadLen = data[CRSF_FRAME_PAYLOAD_LEN_IDX] - CRSF_EXT_FRAME_PAYLOAD_LEN_SIZE_OFFSET;

    if (newFrame && seqNum == 0) // single packet or first chunk of series of packets
    {
        memset(outBuffer, 0, sizeof(outBuffer));
        src = data[CRSF_MSP_SRC_OFFSET];
        dest = data[CRSF_MSP_DEST_OFFSET];
        uint8_t header[3];
        header[0] = '$';
        header[1] = MSPvers == 1 ? 'M' : 'X';
        header[2] = getHeaderDir(data[CRSF_MSP_TYPE_IDX]);
        memcpy(outBuffer, header, 3);
        pktLen = data[CRSF_MSP_FRAME_OFFSET];
        idx = 3; // offset by 3 bytes for MSP header

        if (pktLen > CRSF_MSP_MAX_BYTES_PER_CHUNK)
        { // we end up here if we have a chunked message
            memcpy(&outBuffer[idx], &data[CRSF_MSP_FRAME_OFFSET], CRSFpayloadLen);
            idx += CRSFpayloadLen;
            return;
        }
        else
        { // fits in a single CRSF frame
            memcpy(&outBuffer[idx], &data[CRSF_MSP_FRAME_OFFSET], pktLen);
            idx += pktLen;
        }
    }
    else
    { // process the next chunk of MSP frame
        // if the last CRSF frame is zero padded we can't use the CRSF payload length
        // but if this isn't the last chunk we can't use the MSP payload length
        // the solution is to use the minimum of the two lengths
        uint32_t a = (uint32_t)CRSFpayloadLen;
        uint32_t b = pktLen - idx;
        uint8_t minLen = !(b < a) ? a : b;
        memcpy(&outBuffer[idx], &data[CRSF_MSP_FRAME_OFFSET], minLen); // next chunk of data
        idx += minLen;
    }

    if (idx == pktLen && idx > 3) // we have a complete MSP frame
    {
        frameComplete = true;
    }
}

bool CROSSFIRE2MSP::isNewFrame(uint8_t data)
{
    return (bool)((data & 0b10000) >> 4); // bit active if there is a new frame
}

bool CROSSFIRE2MSP::isError(uint8_t data)
{
    return (bool)((data & 0b10000000) >> 7);
}

uint8_t CROSSFIRE2MSP::getSeqNumber(uint8_t data)
{
    return data & 0b1111; // first four bits is seq number
}

uint8_t CROSSFIRE2MSP::getVersion(uint8_t data)
{
    return ((data & 0b1100000) >> 5);
}

uint8_t CROSSFIRE2MSP::getHeaderDir(uint8_t data)
{
    if (data == 0x7A)
    {
        return '<';
    }
    else if (data == 0x7B)
    {
        return '>';
    }
    else
    {
        return '!';
    }
}

bool CROSSFIRE2MSP::isFrameReady()
{
    return frameComplete;
}

const uint8_t *CROSSFIRE2MSP::getFrame()
{
    return outBuffer;
}

uint8_t CROSSFIRE2MSP::getFrameLen()
{
    return idx;
}

void CROSSFIRE2MSP::resetFrame()
{
    pktLen = 0;
    idx = 0;
    frameComplete = false;
}

uint8_t CROSSFIRE2MSP::getSrc()
{
    return src;
}

uint8_t CROSSFIRE2MSP::getDest()
{
    return dest;
}