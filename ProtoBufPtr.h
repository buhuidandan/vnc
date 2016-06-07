#ifndef _PROTO_BUF_PTR_H_
#define _PROTO_BUF_PTR_H_

#include <cassert>
#include <vector>
#include <memory>
#include "VNCUtil.h"

class ProtoBuf
{
public:
    ProtoBuf(std::size_t hdrSz, std::size_t bodySz, std::size_t trlSz)
            : m_buf(hdrSz + bodySz + trlSz)
    {
        m_pHead = m_buf.data() + hdrSz;
        m_pTail = m_pHead + bodySz;
        assert(m_buf.data() <= m_pHead);
        assert(m_pHead <= m_pTail);
        assert(m_pTail <= (m_buf.data() + m_buf.size()));
    }

    std::size_t totalSize() const { return m_buf.size(); }

    const uint8_t *data() const { return m_pHead; }
    uint8_t *data() { return m_pHead; }

    std::size_t hdrSize() const { return m_pHead - m_buf.data(); }

    uint8_t *moveHead(int delta)
    {
        assert(m_buf.data() <= m_pHead);
        assert(m_pHead <= m_pTail);
        m_pHead += delta;
        assert(m_buf.data() <= m_pHead);
        assert(m_pHead <= m_pTail);

        return m_pHead;
    }
 
    std::size_t bodySize() const { return m_pTail - m_pHead; }

    void setBodySize(std::size_t sz)
    {
        assert(m_pHead <= m_pTail);
        assert(m_pTail <= (m_buf.data() + m_buf.size()));
        m_pTail = m_pHead + sz;
        assert(m_pHead <= m_pTail);
        assert(m_pTail <= (m_buf.data() + m_buf.size()));
    }

    std::size_t trlSize() const
    {
        return m_buf.data() + m_buf.size() - m_pTail;
    }

    uint8_t *moveTail(int delta)
    {
        assert(m_pHead <= m_pTail);
        assert(m_pTail <= (m_buf.data() + m_buf.size()));
        m_pTail += delta;
        assert(m_pHead <= m_pTail);
        assert(m_pTail <= (m_buf.data() + m_buf.size()));

        return m_pTail;
    }

private:
    uint8_t *m_pHead;
    uint8_t *m_pTail;
    std::vector<uint8_t> m_buf;
};

typedef std::unique_ptr<ProtoBuf> ProtoBufPtr;

#endif

