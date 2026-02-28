#include <dirent.h>
#include <fcntl.h>
#include <memory>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "filesrch.h"

/// \brief RAII wrapper for DIR handles to ensure proper cleanup
struct DirHandle
{
    DIR* dir;
    
    DirHandle() : dir(nullptr) {}
    explicit DirHandle(DIR* d) : dir(d) {}
    
    // Close DIR on destruction
    ~DirHandle()
    {
        if (dir)
            closedir(dir);
    }
    
    // Non-copyable
    DirHandle(const DirHandle&) = delete;
    DirHandle& operator=(const DirHandle&) = delete;
    
    // Moveable
    DirHandle(DirHandle&& other) noexcept : dir(other.dir)
    {
        other.dir = nullptr;
    }
    
    DirHandle& operator=(DirHandle&& other) noexcept
    {
        if (this != &other)
        {
            if (dir)
                closedir(dir);
            dir = other.dir;
            other.dir = nullptr;
        }
        return *this;
    }
    
    operator DIR*() { return dir; }
    DIR* get() { return dir; }
};

//
// filesearch:
//
// ATTENTION : make sure there is enough space in filename to put a full path (255 or 512)
// if needmd5check==0 there is no md5 check
// if changestring then filename will be change with the full path and name
// maxsearchdepth==0 only search given directory, no subdirs
// return FS_NOTFOUND
//        FS_MD5SUMBAD;
//        FS_FOUND

filestatus_t I_Filesearch(char *filename,
                          char *startpath,
                          unsigned char *wantedmd5sum,
                          bool completepath,
                          int maxsearchdepth)
{
    // Use smart pointers for automatic memory management (RAII)
    std::unique_ptr<DirHandle[]> dirhandle(new DirHandle[maxsearchdepth]);
    std::unique_ptr<int[]> searchpathindex(new int[maxsearchdepth]);
    
    struct dirent *dent;
    struct stat fstat;
    int found = 0;
    std::string searchstring(filename);  // Use std::string instead of strdup
    filestatus_t retval = FS_NOTFOUND;
    int depthleft = maxsearchdepth;
    char searchpath[1024];

    strcpy(searchpath, startpath);
    searchpathindex[--depthleft] = strlen(searchpath) + 1;

    dirhandle[depthleft] = DirHandle(opendir(searchpath));

    if (searchpath[searchpathindex[depthleft] - 2] != '/')
    {
        searchpath[searchpathindex[depthleft] - 1] = '/';
        searchpath[searchpathindex[depthleft]] = 0;
    }
    else
    {
        searchpathindex[depthleft]--;
    }

    while ((!found) && (depthleft < maxsearchdepth))
    {
        searchpath[searchpathindex[depthleft]] = 0;
        dent = readdir(dirhandle[depthleft].get());
        if (dent)
        {
            strcpy(&searchpath[searchpathindex[depthleft]], dent->d_name);
        }

        if (!dent)
        {
            // DirHandle destructor will call closedir automatically
            depthleft++;
        }
        else if (dent->d_name[0] == '.' &&
                 (dent->d_name[1] == '\0' || (dent->d_name[1] == '.' && dent->d_name[2] == '\0')))
        {
            // we don't want to scan uptree
        }
        else if (stat(searchpath, &fstat) <
                 0) // do we want to follow symlinks? if not: change it to lstat
        {
            // was the file (re)moved? can't stat it
        }
        else if (S_ISDIR(fstat.st_mode) && depthleft)
        {
            strcpy(&searchpath[searchpathindex[depthleft]], dent->d_name);
            searchpathindex[--depthleft] = strlen(searchpath) + 1;

            DIR* newdir = opendir(searchpath);
            if (!newdir)
            {
                // can't open it... maybe no read-permissions
                // go back to previous dir
                depthleft++;
            }
            else
            {
                dirhandle[depthleft] = DirHandle(newdir);
            }

            searchpath[searchpathindex[depthleft] - 1] = '/';
            searchpath[searchpathindex[depthleft]] = 0;
        }
        else if (!strcasecmp(searchstring.c_str(), dent->d_name))
        {
            switch (checkfilemd5(searchpath, wantedmd5sum))
            {
                case FS_FOUND:
                    if (completepath)
                    {
                        strcpy(filename, searchpath);
                    }
                    else
                    {
                        strcpy(filename, dent->d_name);
                    }
                    retval = FS_FOUND;
                    found = 1;
                    break;
                case FS_MD5SUMBAD:
                    retval = FS_MD5SUMBAD;
                    break;
                default:
                    // prevent some compiler warnings
                    break;
            }
        }
    }

    // dirhandle array destructor will automatically close any remaining open directories
    // No need for manual cleanup!

    return retval;
}
