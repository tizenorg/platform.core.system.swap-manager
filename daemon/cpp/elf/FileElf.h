#ifndef FILEELF_H
#define FILEELF_H


#include "File.h"
#include <elf.h>
#include <map>


class FileElf : public File
{
public:
    FileElf();

    int getAddrPlt(const char *names[], uint32_t addrs[], size_t cnt);

private:
    int doOpen();

    struct Data {
        size_t size;
        void *data;
        const Elf32_Shdr *shdr;
    };

    int readSectionsInfo();
    Data *getSection(const Elf32_Shdr *shdr);
    Data *getSection(const std::string &name);
    void putSection(const Data *data);
    int makeRelocMap(const uint8_t jump_slot);
    const Elf32_Shdr *getShdr(const std::string &name);

    typedef std::map <std::string, Elf32_Shdr> ShdrMap;
    typedef std::map <uint32_t, std::string> RelocMap;

    Elf32_Ehdr _fhdr;
    ShdrMap _shdrMap;
    RelocMap _relocMap;
};


#endif // FILEELF_H
