#include "packet.h"

namespace DroidPad
{

tagPacket::tagPacket() : freeBytes(PACKET_BUFF_MAX),
                         readPosOffset(sizeof(eCMD)),
                         writePosOffset(sizeof(eCMD))
{
    memset(buff, 0, sizeof(buff));
}

tagPacket::tagPacket(const tagPacket &p)
{
    freeBytes = p.freeBytes;
    readPosOffset = sizeof(eCMD);
    writePosOffset = p.writePosOffset;
    memcpy_s(buff, sizeof(buff), p.buff, sizeof(p.buff));
}

}