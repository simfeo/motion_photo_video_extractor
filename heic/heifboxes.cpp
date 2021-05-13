#include "heifboxes.h"

namespace HeifUtils
{
    RamData::RamData(std::vector<uint8_t>& invec)
        :m_currentPos(0)
    {
        m_bufferVec = std::move(invec);
        m_length = m_bufferVec.size();
        m_buffer = &m_bufferVec[0];
    }

    RamData::RamData(uint8_t* buf, size_t len)
        : m_length(len)
        , m_currentPos(0)
    {
        m_bufferVec.resize(len);
        memcpy_s((void*)(&m_bufferVec[0]), len, buf, len);
        m_buffer = &m_bufferVec[0];
    }

    int RamData::FileStreamRead(uint8_t* buf, int buf_size)
    {
        if (m_currentPos < static_cast<int64_t>(m_length))
        {
            size_t bytesRead = buf_size;
            if (bytesRead > (m_length - m_currentPos))
            {
                bytesRead = m_length - m_currentPos;
            }
            memcpy(buf, m_buffer + m_currentPos, bytesRead);
            m_currentPos += buf_size;
            return static_cast<int>(bytesRead);
        }
        return -1;
    }

    int RamData::setPosition(int64_t newPos)
    {
        if (newPos >= 0 && newPos <= static_cast<int64_t>(m_length))
        {
            m_currentPos = newPos;
            return 0;
        }
        return -1;
    }

    int64_t RamData::getPosition()
    {
        return m_currentPos;
    }

    size_t RamData::getSize()
    {
        return m_length;
    }
}


HeifBoxBase::HeifBoxBase(std::shared_ptr<HeifUtils::RamData> boxmem)
    : m_size(0)
    , m_type("")
    , m_uid({})
    , m_boxMemory(boxmem)
{
}

void HeifBoxBase::parseHeaders()
{
    uint8_t tmp_buf = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (m_boxMemory->FileStreamRead(&tmp_buf, 1) < 1)
        {
            m_size = 0;
            return;
        }
        m_size = ((m_size) << 8) | tmp_buf;
    }
    m_type.resize(4);
    if (m_boxMemory->FileStreamRead(reinterpret_cast<uint8_t*>(&m_type[0]), 4) < 4)
    {
        m_size == 0;
        return;
    }

    if (m_size == 1)
    {
        m_size = 0;
        for (int i = 0; i < 8; ++i)
        {
            m_boxMemory->FileStreamRead(&tmp_buf, 1);
            m_size = ((m_size) << 8) | tmp_buf;
        }
    }

    if (!m_type.compare("uuid"))
    {
        m_uid.resize(16);
        if (m_boxMemory->FileStreamRead(&m_uid[0], 16) < 16)
        {
            m_size = 0;
            return;
        }
    }
}

const std::string HeifBoxBase::getType() const
{
    return m_type;
}

const uint64_t HeifBoxBase::getSize() const
{
    return m_size;
}

HeifBoxBase::HeifBoxBase()
    : m_size(0)
    , m_type("")
    , m_uid({})
    , m_boxMemory(nullptr)
{
}

SefdBox::SefdBox()
    : HeifBoxBase()
    , m_version(0)
    , m_flags(0)
    , m_ftypStartPos(0)
    , m_ftyp()
    , m_mdat()
{
}

SefdBox::SefdBox(std::shared_ptr<HeifUtils::RamData> boxMem)
    : HeifBoxBase(boxMem)
    , m_ftypStartPos(0)
    , m_ftyp()
    , m_mdat()
{
    parseHeaderFull();
    parseSefd();
}

FtypBox& SefdBox::getFtyp()
{
    return m_ftyp;
}

MdatBox& SefdBox::getMdat()
{
    return m_mdat;
}

uint64_t SefdBox::getFtypStartPos()
{
    return m_ftypStartPos;
}

void SefdBox::parseHeaderFull()
{
    parseHeaders();

    m_boxMemory->FileStreamRead(&m_version, 1);

    uint8_t tmp_buf = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (m_boxMemory->FileStreamRead(&tmp_buf, 1) < 1)
        {
            m_size = 0;
            return;
        }
        m_flags = ((m_flags) << 8) | tmp_buf;
    }
}

void SefdBox::parseSefd()
{
    while (!m_motionPhotoDataFound && m_size)
    {
        uint32_t next_field_length = 0;
        uint8_t tmp_buf = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (m_boxMemory->FileStreamRead(&tmp_buf, 1) < 1)
            {
                m_size = 0;
                return;
            }
            next_field_length = next_field_length | (tmp_buf << (i * 8));
        }
        std::string field;
        field.resize(next_field_length);

        if (m_boxMemory->FileStreamRead(reinterpret_cast<uint8_t*>(&field[0]), next_field_length) < next_field_length)
        {
            m_size == 0;
            return;
        }

        fillPropertyByType(field);
    }

    while (true)
    {
        size_t currentPos = m_boxMemory->getPosition();
        HeifBoxBase box(m_boxMemory);
        box.parseHeaders();
        if (box.getType() == "ftyp")
        {
            m_ftypStartPos = currentPos;
            m_boxMemory->setPosition(currentPos);
            m_ftyp = FtypBox(m_boxMemory);
        }
        else if (box.getType() == "mdat")
        {
            m_boxMemory->setPosition(currentPos);
            m_mdat = MdatBox(m_boxMemory);
        }
        else
        {
            break;
        }
    }
}

void SefdBox::fillPropertyData(std::string& pdata)
{
    std::vector<uint8_t> tmp_data;
    tmp_data.reserve(64);
    uint8_t tmp_buf = 0;
    do
    {
        if (m_boxMemory->FileStreamRead(&tmp_buf, 1) < 1)
        {
            m_size = 0;
            return;
        }
        tmp_data.push_back(tmp_buf);
    } while (tmp_buf != 0);

    uint32_t crap = 0;
    if (m_boxMemory->FileStreamRead(reinterpret_cast<uint8_t*>(&crap), 3) < 3)
    {
        m_size = 0;
        return;
    }

    if (tmp_data.size())
    {
        pdata.resize(tmp_data.size());
        memcpy(&pdata[0], &tmp_data[0], tmp_data.size());
    }
}

void SefdBox::fillPropertyByType(std::string& ptype)
{
    if (ptype == "Image_UTC_Data")
    {
        fillPropertyData(m_imageUtcData);
    }
    else if (ptype == "MCC_Data") // nothing realy interesting
    {
        fillPropertyData(m_imageMCCData);
    }
    else if (ptype == "MotionPhoto_Data")
    {
        m_motionPhotoDataFound = true;
    }
}

FtypBox::FtypBox()
    : HeifBoxBase()
    , m_version(0)
{
}

FtypBox::FtypBox(std::shared_ptr<HeifUtils::RamData> boxMem)
    : HeifBoxBase(boxMem)
{
    parseHeaderFull();
}

const std::string FtypBox::GetMajorBand() const
{
    return m_majorBrand;
}

const std::vector<std::string> FtypBox::getMinorBrands() const
{
    return m_minorBrands;
}

void FtypBox::parseHeaderFull()
{
    int64_t currentPos = m_boxMemory->getPosition();
    parseHeaders();
    m_majorBrand.resize(4);
    if (m_boxMemory->FileStreamRead(reinterpret_cast<uint8_t*>(&m_majorBrand[0]), 4) < 4)
    {
        m_size == 0;
        return;
    }

    uint32_t m_version = 0;
    uint8_t tmp_buf = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (m_boxMemory->FileStreamRead(&tmp_buf, 1) < 1)
        {
            m_size = 0;
            return;
        }
        m_version = (m_version << 8) | tmp_buf;
    }

    uint32_t minorBrandsNum = (m_size - (m_boxMemory->getPosition() - currentPos)) / 4;

    for (int i = 0; i < minorBrandsNum; ++i)
    {
        std::string minType;
        minType.resize(4);
        if (m_boxMemory->FileStreamRead(reinterpret_cast<uint8_t*>(&minType[0]), 4) < 4)
        {
            m_size == 0;
            return;
        }
        m_minorBrands.emplace_back(std::move(minType));
    }

}

MdatBox::MdatBox()
    : m_startPos(0)
    , m_endPos(0)
{
}

MdatBox::MdatBox(std::shared_ptr<HeifUtils::RamData> boxMem)
    : HeifBoxBase(boxMem)
{
    parseHeaderFull();
}

const uint64_t MdatBox::startPosition() const
{
    return m_startPos;
}

const uint64_t MdatBox::endPosition() const
{
    return m_endPos;
}

void MdatBox::parseHeaderFull()
{
    auto currentPos = m_boxMemory->getPosition();
    parseHeaders();
    m_startPos = m_boxMemory->getPosition();
    m_endPos = currentPos + m_size - m_startPos;
    m_boxMemory->setPosition(m_size + currentPos);
}
