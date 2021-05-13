#ifndef HEIFBOXES_H
#define HEIFBOXES_H
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace HeifUtils
{
    class RamData
    {
    public:
        RamData(std::vector<uint8_t>& invec);
        RamData(uint8_t* buf, size_t len);

        int FileStreamRead(uint8_t* buf, int buf_size);
        int setPosition(int64_t newPos);
        int64_t getPosition();
        size_t getSize();

    private:
        size_t                  m_length;
        int64_t                 m_currentPos;
        uint8_t* m_buffer;
        std::vector<uint8_t>    m_bufferVec;
    };

}


class HeifBoxBase
{
public:
    HeifBoxBase(std::shared_ptr<HeifUtils::RamData> boxMem);
    void parseHeaders();
    const std::string getType() const;
    const uint64_t getSize() const;
protected:
    HeifBoxBase();
    uint64_t                    m_size;
    std::string                 m_type;
    std::vector<uint8_t>        m_uid;
    std::shared_ptr<HeifUtils::RamData> m_boxMemory;
};

class FtypBox : public HeifBoxBase
{
public:
    FtypBox();
    FtypBox(std::shared_ptr<HeifUtils::RamData> boxMem);
    const std::string GetMajorBand() const;
    const std::vector<std::string> getMinorBrands() const;
private:
    void parseHeaderFull();

    std::uint32_t               m_version;
    std::string                 m_majorBrand;
    std::vector<std::string>    m_minorBrands;
};

class MdatBox : public HeifBoxBase
{
public:
    MdatBox();
    MdatBox(std::shared_ptr<HeifUtils::RamData> boxMem);
    const uint64_t startPosition() const;
    const uint64_t endPosition() const;
private:
    void parseHeaderFull();

    std::uint64_t               m_startPos;
    std::uint64_t               m_endPos;
};

class SefdBox : public HeifBoxBase
{
public:
    SefdBox();
    SefdBox(std::shared_ptr<HeifUtils::RamData> boxMem);
    FtypBox& getFtyp();
    MdatBox& getMdat();
    uint64_t getFtypStartPos();

private:
    void parseHeaderFull();
    void parseSefd();
    void fillPropertyByType(std::string& ptype);
    void fillPropertyData(std::string& pdata);

    bool        m_motionPhotoDataFound = false;
    uint8_t     m_version = 0;
    uint32_t    m_flags = 0; // only 24 bit can be set
    uint64_t    m_ftypStartPos;

    std::string m_imageUtcData;
    std::string m_imageMCCData;
    FtypBox     m_ftyp;
    MdatBox     m_mdat;
};
#endif // HEIFBOXES_H
