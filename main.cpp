// Copyright © 2021 Smbat Makiyan (a.k.a. idimus, a.k.a. simfeo). All rights reserved.

#include <heifreader.h>
#include <heifboxes.h>

#include <TinyEXIF.h>
#include <argparse.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>


bool check_extension(std::string const& img_file, std::string const& extension)
{
    if (img_file.length() >= extension.length())
    {
        return (0 == img_file.compare(img_file.length() - extension.length(), extension.length(), extension));
    }
    else
    {
        return false;
    }
}

int main(int argc, char** argv)
{
    ArgumentParser parser;

    parser.addArgument("-i", "--input", 1);
    parser.addArgument("-o", "--output", 1);
    parser.addArgument("--help");
    //parser.addArgument("--license");

    try
    {
        if (argc > 1)
        {
            const char** ptr = new const char* [argc];
            for (int i = 0; i < argc; ++i)
            {
                ptr[i] = argv[i];
            }
            parser.parse(static_cast<size_t>(argc), ptr);
            delete[] ptr;
        }
        else
        {
            std::cout << parser.usage() << std::endl;
            return 0;
        }

    }
    catch (const std::exception&)
    {
        std::cout << parser.usage() << std::endl;
        return 1;
    }


    if (parser.count("help"))
    {
        std::cout << parser.usage() << std::endl;
        return 0;
    }

    //if (parser.exists("license"))
    //{
    //    return 0;
    //}

    if (!parser.count("input") || !parser.count("output"))
    {
        std::cerr << "you should specify both input and output files" << std::endl;
        std::cout << parser.usage() << std::endl;
        return 2;
    }

    std::string input_file = parser.retrieve<std::string>("input");
    std::string output_file = parser.retrieve<std::string>("output");
    std::string input_file_lower;
    input_file_lower.resize(input_file.size());
    std::transform(input_file.begin(), input_file.end(), input_file_lower.begin(), std::tolower);

    if ( !check_extension(input_file_lower, ".jpg")
        && !check_extension(input_file_lower, ".jpeg")
        && !check_extension(input_file_lower, ".heic"))
    {
        std::cerr << R"(this application works only with ".jpg", ".jpeg" and ".heic" files.)" << std::endl;
        return 3;
    }

    std::shared_ptr<std::ifstream> imgFile(new std::ifstream(input_file.c_str(), std::ifstream::in | std::ifstream::binary), [](std::ifstream* p) {if (p) { p->close(), delete p; }});
    if (imgFile->bad())
    {
        std::cerr << "cannot read input file" << std::endl;
        return 3;
    }

    imgFile->seekg(0, std::ios::end);
    std::streampos length = imgFile->tellg();
    imgFile->seekg(0, std::ios::beg);
    std::vector<uint8_t> data(length);
    imgFile->read((char*)data.data(), length);

    if (check_extension(input_file_lower, ".heic"))
    {
        if (imgFile->bad())
        {
            imgFile.reset(new std::ifstream(input_file.c_str(), std::ifstream::in | std::ifstream::binary), [](std::ifstream* p) {if (p) { p->close(), delete p; }});
        }

        HeifReader heif;
        heif.load(*imgFile.get());
        SefdBox sf = heif.getSefdBox();
        if (sf.getSize() != 0
            && sf.getFtyp().getSize() != 0
            && sf.getFtyp().GetMajorBand() == "mp42"
            && sf.getMdat().getSize() != 0
            && sf.getMdat().startPosition() != 0
            && sf.getMdat().endPosition() != 0)
        {
            std::vector<uint8_t> videoData;
            videoData.resize(sf.getSize() - sf.getFtypStartPos());
            if (imgFile->bad())
            {
                imgFile.reset(new std::ifstream(input_file.c_str(), std::ifstream::in | std::ifstream::binary), [](std::ifstream* p) {if (p) { p->close(), delete p; }});
            }

            imgFile->seekg(heif.getSefdOffset() + sf.getMdat().startPosition());
            imgFile->read(reinterpret_cast<char*>(&videoData[0]), videoData.size());
            std::ofstream ofile(output_file.c_str(), std::ios::binary);
            if (ofile.bad())
            {
                std::cerr << "cannot open out file" << std::endl;
                return 5;
            }
            ofile.write((char*)(videoData.data()), videoData.size());
            ofile.close();
        }
    }
    else
    {
        auto exif_info = TinyEXIF::EXIFInfo(data.data(), length);
        if (exif_info.Fields)
        {
            exif_info.parseFromXMPSegment(data.data(), length);
            if (exif_info.MicroVideo.HasMicroVideo)
            {
                size_t offset = data.size() - exif_info.MicroVideo.MicroVideoOffset;
                std::ofstream ofile(output_file.c_str(), std::ios::binary);
                if (ofile.bad())
                {
                    std::cerr << "cannot open out file" << std::endl;
                    return 5;
                }
                offset = data.size() - exif_info.MicroVideo.MicroVideoOffset;
                ofile.write((char*)(data.data() + offset), data.size() - offset);
                ofile.close();
            }
            else
            {
                std::cerr << "there is no any video in this file" << std::endl;
                return 4;
            }
        }
    }

    std::cout << "job is done" << std::endl;
    return 0;
}
