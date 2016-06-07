#ifndef _VNC_UTIL_H_
#define _VNC_UTIL_H_

#include <cstdint>

inline uint16_t get_be16(const uint8_t *p)
{
    return (p[0] << 8) | p[1];
}

inline uint32_t get_be32(const uint8_t *p)
{
    return (static_cast<uint32_t>(get_be16(p)) << 16) | get_be16(p+2);
}

inline uint64_t get_be64(const uint8_t *p)
{
    return (static_cast<uint64_t>(get_be32(p)) << 32) | get_be32(p+4);
}

inline void set_be16(uint8_t *p, uint16_t v)
{
    p[0] = static_cast<uint8_t>(v >> 8);
    p[1] = static_cast<uint8_t>(v);
}

inline void set_be32(uint8_t *p, uint32_t v)
{
    set_be16(p, static_cast<uint16_t>(v >> 16));
    set_be16(p+2, static_cast<uint16_t>(v));
}

inline void set_be64(uint8_t *p, uint64_t v)
{
    set_be32(p, static_cast<uint32_t>(v >> 32));
    set_be32(p+4, static_cast<uint32_t>(v));
}

#endif

