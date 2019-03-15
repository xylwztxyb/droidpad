#ifndef __PACKET_H__
#define __PACKET_H__

#include "common.h"

namespace DroidPad
{

typedef class tagPacket
{
private:
  struct _arg_segment
  {
    u_int len;
    char lpData[1];
  };

public:
  tagPacket();
  tagPacket(const tagPacket &);

  template <typename T>
  eSTATUS write(const T &t);

  template <>
  eSTATUS write<std::wstring>(const std::wstring &t);

  template <typename T, int N>
  eSTATUS write(const T (&t)[N]);

  template <typename T>
  eSTATUS read(T &t);

  template <>
  eSTATUS read<std::wstring>(std::wstring &t);

  template <typename T, int N>
  eSTATUS read(T (&t)[N]);

  eCMD getCMD() { return *(eCMD*)buff; }
  void setCMD(eCMD cmd) {*(eCMD*)buff = cmd; }
  int buffLength() {return sizeof(buff);}
  const char* getBuff() const {return buff;}
  char* getBuff() {return buff;}

private:
  u_int freeBytes;
  u_int readPosOffset;
  u_int writePosOffset;
  char buff[sizeof(eCMD) + PACKET_BUFF_MAX];
} PACKET;

/////////////////////////////////////////////////////

template <typename T>
eSTATUS tagPacket::write(const T &t)
{
  _arg_segment *arg = (_arg_segment *)(buff + writePosOffset);
  u_int _len = sizeof(T);
  if (_len > freeBytes)
    return STATUS_OUT_OF_MEM;
  arg->len = _len;
  new (arg->lpData) T(t);
  writePosOffset += (sizeof(arg->len) + arg->len);
  freeBytes -= (sizeof(arg->len) + arg->len);
  return STATUS_NO_ERROR;
}

template <typename T, int N>
eSTATUS tagPacket::write(const T (&t)[N])
{
  _arg_segment *arg = (_arg_segment *)(buff + writePosOffset);
  u_int _len = sizeof(T) * N;
  if (_len > freeBytes)
    return STATUS_OUT_OF_MEM;
  arg->len = _len;
  T *const p = new (arg->lpData) T[N];
  for (int i = 0; i < N; i++)
    p[i] = t[i];
  //now we try to move the memory.
  //Because the pointer returned from new[]
  //maybe not the arg->lpData, but pointed to other address behind of lpData.
  //and we do not accept this behavior
  bool mem_has_moved = false;
  if ((void *)p != (void *)(arg->lpData))
  {
    memmove_s(arg->lpData, _len, p, _len);
    mem_has_moved = true;
  }
  writePosOffset += (sizeof(arg->len) + arg->len);
  freeBytes -= (sizeof(arg->len) + arg->len);
  //Finally, we try to clear the free mem
  if (mem_has_moved)
    memset(buff + writePosOffset, 0, freeBytes);
  return STATUS_NO_ERROR;
}

template <typename T>
eSTATUS tagPacket::read(T &t)
{
  _arg_segment *arg = (_arg_segment *)(buff + readPosOffset);
  if (arg->len == 0)
    return STATUS_NO_MORE_DATA;
  t = *((T *)(arg->lpData));
  readPosOffset += (sizeof(arg->len) + arg->len);
  return STATUS_NO_ERROR;
}

template <typename T, int N>
eSTATUS tagPacket::read(T (&t)[N])
{
  _arg_segment *arg = (_arg_segment *)(buff + readPosOffset);
  if (arg->len == 0)
    return STATUS_NO_MORE_DATA;
  T *p = (T *)arg->lpData;
  for (int i = 0; i < N; i++)
    t[i] = p[i];
  readPosOffset += (sizeof(arg->len) + arg->len);
  return STATUS_NO_ERROR;
}

// Two specializing func for std::wstring

template <>
eSTATUS tagPacket::write<std::wstring>(const std::wstring &t)
{
	_arg_segment *arg = (_arg_segment *)(buff + writePosOffset);
	u_int _len = sizeof(std::wstring::value_type) * t.length();
	if (_len > freeBytes)
		return STATUS_OUT_OF_MEM;
	arg->len = _len;
	memcpy_s(arg->lpData, arg->len, t.data(), arg->len);
	writePosOffset += (sizeof(arg->len) + arg->len);
	freeBytes -= (sizeof(arg->len) + arg->len);
	return STATUS_NO_ERROR;
}

template <>
eSTATUS tagPacket::read<std::wstring>(std::wstring &t)
{
	_arg_segment *arg = (_arg_segment *)(buff + readPosOffset);
	if (arg->len == 0)
		return STATUS_NO_MORE_DATA;
	size_t n = arg->len / sizeof(std::wstring::value_type);
	t.assign((std::wstring::value_type *)arg->lpData, n);
	readPosOffset += (sizeof(arg->len) + arg->len);
	return STATUS_NO_ERROR;
}

}

#endif