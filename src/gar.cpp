// gar.cpp --- extracts files from .gar archives

#include <boost/filesystem.hpp>

#include <fstream>

#include "gar.hpp"
#include "util.hpp"

struct fileinfo {
    std::string filetype;
    uint32_t filesize;
    std::string basename;
    uint32_t fileat;
};

void gar_ext(std::vector<uint8_t> arcdata, std::string outdir) {
    // get header info
    uint32_t arcsize    = vec2int32(arcdata, 4);
    uint16_t numtypes   = vec2int16(arcdata, 8);
    uint16_t numfiles   = vec2int16(arcdata, 0xA);
    uint32_t fnamepoint = vec2int32(arcdata, 0x10);
    uint32_t filepoint  = vec2int32(arcdata, 0x14);

    if (arcsize != arcdata.size()) {
        std::cerr << "Warning! Archive is " << arcdata.size() << " bytes, expecting " << arcsize << ".\n";
    }

    std::vector<fileinfo> FILEIDX(numfiles);

    // get file type info
    uint32_t entryat = 0x20;
    uint32_t tnameat;
    uint32_t tlistat;
    uint32_t tlistsize;
    std::string curtname;

    for (uint16_t i = 0; i < numtypes; i++) {
        tnameat   = vec2int32(arcdata, entryat + 8);
        tlistat   = vec2int32(arcdata, entryat + 4);
        tlistsize = vec2int32(arcdata, entryat);

        curtname = std::string((char*)arcdata.data() + tnameat);

        if (tlistat != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < tlistsize; j++) {
                FILEIDX.at(vec2int32(arcdata, tlistat + 4 * j)).filetype = curtname;
            }
        }

        // move to next entry
        entryat += 0x10;
    }

    // get file names and sizes
    entryat = fnamepoint;

    for (uint16_t i = 0; i < numfiles; i++) {
        // file size
        FILEIDX.at(i).filesize = vec2int32(arcdata, entryat);

        // file name
        FILEIDX.at(i).basename = std::string((char*)arcdata.data() + vec2int32(arcdata, entryat + 4));

        // next entry
        entryat += 0xC;
    }

    // get file pointers
    for (uint16_t i = 0; i < numfiles; i++) {
        FILEIDX.at(i).fileat = vec2int32(arcdata, filepoint + i * 4);
    }

    // file extraction time
    boost::filesystem::path outpath;
    std::string curoutname;
    std::ofstream outfile;

    outpath = outdir;
    outpath = boost::filesystem::absolute(outpath);

    for (auto & i : FILEIDX) {
        curoutname = i.basename + "." + i.filetype;

        outfile.open((outpath / curoutname).string(), std::ios::binary);
        outfile.write((char*)(arcdata.data() + i.fileat), i.filesize);
        outfile.close();
    }
}
