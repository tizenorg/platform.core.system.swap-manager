#ifndef FILE_H
#define FILE_H


#include <cstdio>
#include <string>


class File
{
public:
    explicit File();
    virtual ~File();

    int open(const std::string &path);
    void close();
    int read(size_t pos, void *data, size_t size);
    bool isOpen();
    std::string path() { return _path; }

private:
    virtual int doOpen() { return 0; }

    FILE *_f;
    std::string _path;
};

#endif // FILE_H
