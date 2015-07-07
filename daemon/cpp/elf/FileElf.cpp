#include "FileElf.h"
#include <elf.h>
#include <cerrno>
#include "swap_debug.h"


static int checkFhdr(const Elf32_Ehdr *fhdr)
{
    if ((fhdr->e_ident[EI_MAG0] != ELFMAG0) ||
        (fhdr->e_ident[EI_MAG1] != ELFMAG1) ||
        (fhdr->e_ident[EI_MAG2] != ELFMAG2) ||
        (fhdr->e_ident[EI_MAG3] != ELFMAG3)) {
         LOGE("Unsupported file format! e_ident: %02x %c %c %c.\n",
                fhdr->e_ident[0], fhdr->e_ident[1], fhdr->e_ident[2], fhdr->e_ident[3]);
         return -EINVAL;
    }

    if (fhdr->e_ident[EI_CLASS] != ELFCLASS32) {
         LOGE("Unsupported ELF file class: %u!\n", fhdr->e_ident[EI_CLASS]);
         return -EINVAL;
    }

    if (fhdr->e_ident[EI_VERSION] != EV_CURRENT) {
         LOGE("Unsupported ELF header version: %u!\n", fhdr->e_ident[EI_VERSION]);
         return -EINVAL;
    }

    if (fhdr->e_ehsize < sizeof(Elf32_Ehdr)) {
         LOGE("Invalid ELF header length: %hu\n!", fhdr->e_ehsize);
         return -EINVAL;
    }

    if (fhdr->e_version != EV_CURRENT) {
         LOGE("Unsupported ELF version: %02x!\n", fhdr->e_version);
         return -EINVAL;
    }

    return 0;
}

static bool checkStr(const char *str, const char *endBuf)
{
    for (const char *n = str; n < endBuf; ++n)
        if (*n == '\0')
            return true;

    return false;
}

static uint32_t ror(uint32_t num, uint32_t rorN)
{
    uint32_t p = num % (1 << rorN);

    return (num >> rorN) + (p << (32 - rorN));
}


FileElf::Data *FileElf::getSection(const Elf32_Shdr *shdr)
{
    Data *data = new Data;
    if (data == 0)
        return 0;

    data->size = shdr->sh_size;
    data->data = ::operator new(data->size);
    if (data->data == 0) {
        delete data;
        return 0;
    }

    // read section
    int ret = read(shdr->sh_offset, data->data, data->size);
    if (ret) {
        LOGE("Failed to read section. Error %d!\n", ret);
        putSection(data);
        return 0;
    }

    data->shdr = shdr;
    return data;
}

FileElf::Data *FileElf::getSection(const std::string &name)
{
    const Elf32_Shdr *shdr = getShdr(name);
    if (shdr == 0) {
        LOGE("'%s' section is not found\n", name.c_str());
        return 0;
    }

    return getSection(shdr);
}

void FileElf::putSection(const Data *data)
{
    ::operator delete(data->data);
    delete data;
}

int FileElf::readSectionsInfo()
{
    Elf32_Shdr shdrStr;
    size_t offsetStr = _fhdr.e_shoff + _fhdr.e_shstrndx * _fhdr.e_shentsize;

    _shdrMap.clear();

    // read shstrtab section header
    int ret = read(offsetStr, &shdrStr, sizeof(shdrStr));
    if (ret) {
        LOGE("Failed to read section header 'shstrtab'. Error %d!\n", ret);
        return ret;
    }

    // check shstrtab section
    if (shdrStr.sh_type != SHT_STRTAB) {
         LOGE("Failed to find section header table entry for shstrtab with section names!\n");
         return -EINVAL;
    }

    Data *data = getSection(&shdrStr);
    if (data == 0) {
        LOGE("get data 'shstrtab'' section %d\n");
        return -ENOMEM;
    }

    const char *strData = reinterpret_cast<const char *>(data->data);
    const char *strDataEnd = strData + data->size;
    if (strData == 0) {
        ret = -ENOMEM;
        goto putSect;
    }

    for (int i = 0; i < _fhdr.e_shnum; ++i) {
        Elf32_Shdr shdr;
        size_t offset = _fhdr.e_shoff + i * _fhdr.e_shentsize;

        // read section header
        int ret = read(offset, &shdr, sizeof(shdr));
        if (ret) {
            LOGE("Failed to read section header %d. Error %d!\n", i, ret);
            goto putSect;
        }

        // read section name
        const char *name = strData + shdr.sh_name;
        if (checkStr(name, strDataEnd) == false) {
            LOGE("Failed to get section name %d.!\n", i);
            ret = -EINVAL;
            goto putSect;
        }

        _shdrMap[name] = shdr;
    }

putSect:
    putSection(data);
    return ret;
}

const Elf32_Shdr *FileElf::getShdr(const std::string &name)
{
    ShdrMap::const_iterator it = _shdrMap.find(name);
    if (it == _shdrMap.end())
        return 0;

    return &it->second;
}

int FileElf::makeRelocMap(const uint8_t jump_slot)
{
    if (_relocMap.empty() == false)
        return 0;

    static const char *nameRel = ".rel.plt";
    static const char *nameSym = ".dynsym";
    static const char *nameStr = ".dynstr";
    int ret = 0;
    Data *dataRel;
    Data *dataSym;
    Data *dataStr;
    Elf32_Rel *rel;
    size_t relCnt;
    Elf32_Sym *sym;
    size_t symCnt;
    const char *strEnd;
    const char *strBegin;

    // section '.rel.plt'
    dataRel = getSection(nameRel);
    if (dataRel == 0)
        return -ENOMEM;

    if (dataRel->size % sizeof(Elf32_Rel)) {
        LOGE("'%s' section incorrect\n", nameRel);
        ret = -EINVAL;
        goto putSectRel;
    }
    rel = reinterpret_cast<Elf32_Rel *>(dataRel->data);
    relCnt = dataRel->size / sizeof(Elf32_Rel);

    // section '.dynsym'
    dataSym = getSection(nameSym);
    if (dataSym == 0) {
        ret = -ENOMEM;
        goto putSectRel;
    }

    // section '.dynstr'
    if (dataSym->size % sizeof(Elf32_Sym)) {
        LOGE("'%s' section incorrect\n", nameSym);
        ret = -EINVAL;
        goto putSectSym;
    }
    sym = reinterpret_cast<Elf32_Sym *>(dataSym->data);
    symCnt = dataSym->size / sizeof(Elf32_Sym);

    dataStr = getSection(nameStr);
    if (dataStr == 0) {
        ret = -ENOMEM;
        goto putSectSym;
    }
    strBegin = reinterpret_cast<const char *>(dataStr->data);
    strEnd = strBegin + dataStr->size;


    for (int i = 0; i < relCnt; ++i) {
        if (ELF32_R_TYPE(rel[i].r_info) == jump_slot) {
            uint32_t offset = rel[i].r_offset;
            uint32_t symId = ELF32_R_SYM(rel[i].r_info);

            if (symId >= symCnt) {
                LOGE("symID is very big, symId=%lx\n", symId);
                continue;
            }

            uint32_t strOff = sym[symId].st_name;

            // read symbol name
            const char *name = strBegin + strOff;
            if (checkStr(name, strEnd) == false) {
                LOGE("Failed to get symbol name %d.!\n", strOff);
                continue;
            }

            _relocMap[offset] = name;
        }
    }

    putSection(dataStr);
putSectSym:
    putSection(dataSym);
putSectRel:
    putSection(dataRel);
    return ret;
}

int FileElf::getAddrPlt(const char *names[], uint32_t addrs[], size_t cnt)
{
    if (_fhdr.e_machine != EM_ARM) {
        LOGE("mach[%lu] is not support\n", _fhdr.e_machine);
        return -EINVAL;
    }

    int ret = makeRelocMap(R_ARM_JUMP_SLOT);
    if (ret)
        return ret;

    static const char *secName = ".plt";
    const Data *data = getSection(secName);
    if (data == 0)
        return -ENOMEM;

    // for ARM
    const uint32_t *inst = reinterpret_cast<const uint32_t *>(data->data);
    size_t instCnt = data->size / 4;

    typedef std::map<std::string, uint32_t> FuncMap;
    FuncMap funcMap;
    uint32_t addr, n0, n1, n2, offset, rorN;


    for (int i = 0, step = 0; i < instCnt; ++i) {
        uint32_t p = inst[i] & 0xfffff000;

        switch (step) {
        case 0:
            if (p != 0xe28fc000)
                continue;

            rorN = 2 * ((inst[i] % 0x1000) / 0x100);
            addr = data->shdr->sh_addr + 4 * i;
            n0 = ror(inst[i] % 0x100, rorN) + addr + 8;     // add ip, pc, #a, b
            step = 1;
            break;

        case 1:
            if (p != 0xe28cc000) {
                step = 0;
                continue;
            }

            rorN = 2 * ((inst[i] % 0x1000) / 0x100);
            n1 = ror(inst[i] % 0x100, rorN);                // add ip, ip, #c, d
            step = 2;
            break;

        case 2:
            if (p != 0xe5bcf000) {
                step = 0;
                continue;
            }

            n2 = inst[i] % 0x1000;
            offset = n0 + n1 + n2;                          // ldr pc, [ip, #e]!

            RelocMap::const_iterator it = _relocMap.find(offset);
            if (it != _relocMap.end())
                funcMap[it->second] = addr;

            step = 0;
        }
    }

    // populate addrs
    for (int i = 0; i < cnt; ++i) {
        FuncMap::const_iterator it = funcMap.find(names[i]);
        addrs[i] = it == funcMap.end() ? 0 : it->second;
    }

    putSection(data);
    return 0;
}

FileElf::FileElf()
{
}

int FileElf::doOpen()
{
    int ret = read(0, &_fhdr, sizeof(_fhdr));
    if (ret) {
        LOGE("Failed to read file's header. Error %d!\n", ret);
        return -errno;
    }

    ret = checkFhdr(&_fhdr);
    if (ret)
        return ret;

    ret = readSectionsInfo();

    return ret;
}
