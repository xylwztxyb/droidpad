package com.droidpad;

import java.io.Serializable;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.Charset;

public class Packet implements Serializable {

    Buffer _buffer = new Buffer();

    public int getCmd() {
        ByteBuffer bb = ByteBuffer.allocate(4);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        bb.put(_buffer.content, 0, 4);
        bb.rewind();
        return bb.getInt();
    }

    public void setCmd(int cmd) {
        ByteBuffer bb = ByteBuffer.allocate(4);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        bb.putInt(cmd);
        byte[] array = bb.array();
        System.arraycopy(array, 0, _buffer.content, 0, 4);
    }

    void writeInt(int i) {
        ByteBuffer bb = ByteBuffer.allocate(4 + 4);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        bb.putInt(4);
        bb.putInt(i);
        byte[] array = bb.array();
        if (array.length > _buffer.freeBytes)
            throw new OutOfMemoryError();
        System.arraycopy(array, 0, _buffer.content, _buffer.writePosOffset, array.length);
        _buffer.writePosOffset += array.length;
        _buffer.freeBytes -= array.length;
    }

    int readInt() throws PacketNoMoreDataException {
        ByteBuffer bb = ByteBuffer.allocate(4);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        bb.put(_buffer.content, _buffer.readPosOffset, 4);
        bb.rewind();
        if (bb.getInt() == 0)
            throw new PacketNoMoreDataException();
        bb.clear();
        bb.put(_buffer.content, _buffer.readPosOffset + 4, 4);
        _buffer.readPosOffset += (4 + 4);
        bb.rewind();
        return bb.getInt();
    }

    void writeStringUnicode(String s) {
        byte[] b = s.getBytes(Charset.forName("UTF_16LE"));
        ByteBuffer bb = ByteBuffer.allocate(4 + b.length);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        bb.putInt(b.length);
        bb.put(b);
        byte[] array = bb.array();
        if (array.length > _buffer.freeBytes)
            throw new OutOfMemoryError();
        System.arraycopy(array, 0, _buffer.content, _buffer.writePosOffset, array.length);
        _buffer.writePosOffset += array.length;
        _buffer.freeBytes -= array.length;
    }

    public class Buffer {
        private int freeBytes = 1024;
        private int readPosOffset = 4;
        private int writePosOffset = 4;
        private byte[] content = new byte[4 + 1024];

        public byte[] get() {
            return content;
        }
    }

    class PacketNoMoreDataException extends Exception {
        PacketNoMoreDataException() {
            super("Error, packet has no more data in buffer.");
        }
    }


}
