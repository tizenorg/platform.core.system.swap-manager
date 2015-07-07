#include "File.h"
#include <cerrno>
#include "swap_debug.h"


File::File() :
    _f(0)
{
}

File::~File()
{
    close();
}

bool File::isOpen()
{
    return static_cast<bool>(_f);
}

int File::open(const std::string &path)
{
    if (isOpen()) {
        LOGE("already open flie '%s'\n", _path.c_str());
        return -EINVAL;
    }

    _f = fopen(path.c_str(), "rb");
    if (_f == 0) {
        LOGE("Failed to open file %s. Error %d!\n", path.c_str(), errno);
        return errno;
    }

    _path = path;

    int ret = doOpen();
    if (ret)
        close();

    return ret;
}

void File::close()
{
    if (_f) {
        fclose(_f);
        _f = 0;
    }
}

int File::read(size_t pos, void *data, size_t size)
{
    if (fseek(_f, pos, SEEK_SET))
        return -ferror(_f);

    if (fread(data, 1, size, _f) != size)
        return -ferror(_f);

    return 0;
}
