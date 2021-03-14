#include "mdv_filesystem.h"
#include "mdv_log.h"
#include "mdv_string.h"
#include "mdv_stack.h"
#include "mdv_alloc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>


static bool mdv_mkdir_impl(char const *path)
{
    struct stat st;

    if (stat(path, &st) != 0)
    {
        if (mkdir(path, 0777) != 0 && mdv_error() != MDV_EEXIST)
        {
            MDV_LOGE("mkdir failed. Unable to create directory: %s", path);
            return false;
        }
    }
    else if (!S_ISDIR(st.st_mode))
    {
        MDV_LOGE("mkdir failed. Invalid path: %s", path);
        return false;
    }
    return true;
}


bool mdv_mkdir(char const *str)
{
    if (!str)
    {
        MDV_LOGE("mkdir failed. Path is null.");
        return false;
    }

    size_t const len = strlen(str);
    if (!len)
    {
        MDV_LOGE("mkdir failed. Path is empty.");
        return false;
    }

    if (len > MDV_PATH_MAX)
    {
        MDV_LOGE("mkdir failed. Path is too long.");
        return false;
    }

    bool const is_cur_dir = *str != '/' && *str != '.';

    mdv_stack(char, MDV_PATH_MAX + 2) mpool;
    mdv_stack_clear(mpool);

    mdv_string path = mdv_str_pdup(mpool, is_cur_dir ? "." : "");

    char prev_char = *str;
    char const *dir = str;

    for(char const *ch = str + 1; *ch; ++ch)
    {
        switch(*ch)
        {
            case '/':
            {
                if (prev_char == '/')
                {
                    dir = ch;
                    continue;
                }

                mdv_string pdir = { ch - dir + 1, (char*)dir };

                path = mdv_str_pcat(mpool, path, pdir);

                if (!mdv_mkdir_impl(path.ptr))
                    return false;

                if (!mdv_mkdir_impl(path.ptr))
                    return false;

                dir = ch;
            }

            default:
                break;
        }

        prev_char = *ch;
    }

    size_t const last_dir_len = str + len - dir;

    if (last_dir_len > 1)
    {
        mdv_string pdir = { last_dir_len + 1, (char*)dir };

        path = mdv_str_pcat(mpool, path, pdir);

        if (!mdv_mkdir_impl(path.ptr))
            return false;
    }

    return true;
}


bool mdv_rmdir(char const *path)
{
    if (!path)
    {
        MDV_LOGE("rmdir failed. Path is null.");
        return false;
    }

    size_t const len = strlen(path);
    if (!len)
    {
        MDV_LOGE("rmdir failed. Path is empty.");
        return false;
    }

    if (len > MDV_PATH_MAX)
    {
        MDV_LOGE("rmdir failed. Path is too long.");
        return false;
    }

    typedef struct
    {
        DIR        *pdir;
        mdv_string  path;
        size_t      dir_name_len;
    } dir_entry;

    mdv_stack(dir_entry, MDV_DIRS_DEPTH_MAX) dirs;
    mdv_stack_clear(dirs);

    mdv_stack(char, MDV_PATH_MAX) mpool;
    mdv_stack_clear(mpool);

    DIR *pdir = opendir(path);
    if (!pdir)
    {
        if (mdv_error() == MDV_ENOENT)
            return true;
        MDV_LOGE("rmdir failed. Directory could not be opened due the error: %d", mdv_error());
        return false;
    }

    dir_entry root_dir = { pdir, mdv_str_pdup(mpool, path), len + 1 };

    static mdv_string const dir_delimeter = mdv_str_static("/");

    (void)mdv_stack_push(dirs, root_dir);

    do
    {
        dir_entry const *dir = (dir_entry const*)mdv_stack_top(dirs);
        dir->path.ptr[dir->path.size - 1] = 0;

        struct dirent *ent = readdir(dir->pdir);

        if(!ent)
        {
            if (remove(dir->path.ptr) == -1)
            {
                mdv_stack_foreach(dirs, dir_entry, dir)
                    closedir(dir->pdir);
                MDV_LOGE("rmdir failed. Directory could not be removed due the error: %d", mdv_error());
                return false;
            }

            closedir(dir->pdir);
            (void)mdv_stack_pop(dirs);
            (void)mdv_stack_pop(mpool, dir->dir_name_len);

            continue;
        }

        size_t const dlen = strlen(ent->d_name);

        if (ent->d_type == DT_DIR)
        {
            if ((dlen == 1 && !strncmp(ent->d_name, ".", 1))
                || (dlen == 2 && !strncmp(ent->d_name, "..", 2)))
                continue;

            mdv_string const dir_name = { dlen + 1, ent->d_name };

            mdv_string subdir_path = mdv_str_pcat(mpool, dir->path, dir_delimeter);
            subdir_path = mdv_str_pcat(mpool, subdir_path, dir_name);

            DIR *pdir = opendir(subdir_path.ptr);
            if (!pdir)
            {
                mdv_stack_foreach(dirs, dir_entry , dir)
                    closedir(dir->pdir);
                MDV_LOGE("rmdir failed. Directory could not be opened due the error: %d", mdv_error());
                return false;
            }

            dir_entry subdir = { pdir, subdir_path, dlen + 1 };
            (void)mdv_stack_push(dirs, subdir);

            continue;
        }
        else
        {
            mdv_string const file_name = { dlen + 1, ent->d_name };
            mdv_string file_path = mdv_str_pcat(mpool, dir->path, dir_delimeter);
            file_path = mdv_str_pcat(mpool, file_path, file_name);

            if (remove(file_path.ptr) == -1)
            {
                mdv_stack_foreach(dirs, dir_entry, dir)
                    closedir(dir->pdir);
                MDV_LOGE("rmdir failed. File could not be removed due the error: %d", mdv_error());
                return false;
            }

            (void)mdv_stack_pop(mpool, dlen + 1);
        }
    }
    while(!mdv_stack_empty(dirs));

    return true;
}


typedef struct
{
    mdv_enumerator base;
    DIR *pdir;
    struct dirent *entry;
} mdv_dir_enumerator_t;


static mdv_enumerator * mdv_dir_enumerator_retain(mdv_enumerator *enumerator)
{
    atomic_fetch_add_explicit(&enumerator->rc, 1, memory_order_acquire);
    return enumerator;
}


static uint32_t mdv_dir_enumerator_release(mdv_enumerator *enumerator)
{
    uint32_t rc = 0;

    if (enumerator)
    {
        rc = atomic_fetch_sub_explicit(&enumerator->rc, 1, memory_order_release) - 1;

        if (!rc)
        {
            mdv_dir_enumerator_t *impl = (mdv_dir_enumerator_t *)enumerator;
            closedir(impl->pdir);
            mdv_free(enumerator, "dir_enumerator");
        }
    }

    return rc;
}


static mdv_errno mdv_dir_enumerator_reset(mdv_enumerator *enumerator)
{
    (void)enumerator;
    return MDV_NO_IMPL;
}


static mdv_errno mdv_dir_enumerator_next(mdv_enumerator *enumerator)
{
    mdv_dir_enumerator_t *impl = (mdv_dir_enumerator_t *)enumerator;

    while ((impl->entry = readdir(impl->pdir)) != 0
            && impl->entry->d_type != DT_REG);

    return impl->entry ? MDV_OK : MDV_FALSE;
}


static void * mdv_dir_enumerator_current(mdv_enumerator *enumerator)
{
    mdv_dir_enumerator_t *impl = (mdv_dir_enumerator_t *)enumerator;
    if (!impl->entry)
        return 0;
    return impl->entry->d_name;
}


mdv_enumerator * mdv_dir_enumerator(char const *path)
{
    mdv_dir_enumerator_t *enumerator = mdv_alloc(sizeof(mdv_dir_enumerator_t), "dir_enumerator");

    if (!enumerator)
    {
        MDV_LOGE("No memory for directory content enumerator");
        return 0;
    }

    enumerator->pdir = opendir(path);

    if (!enumerator->pdir)
    {
        MDV_LOGE("Directory '%s' could not be opened due the error: %d", path, mdv_error());
        mdv_free(enumerator, "dir_enumerator");
        return 0;
    }

    enumerator->entry = 0;

    atomic_init(&enumerator->base.rc, 1);

    static const mdv_ienumerator vtbl =
    {
        .retain = mdv_dir_enumerator_retain,
        .release = mdv_dir_enumerator_release,
        .reset = mdv_dir_enumerator_reset,
        .next = mdv_dir_enumerator_next,
        .current = mdv_dir_enumerator_current
    };

    enumerator->base.vptr = &vtbl;

    return &enumerator->base;
}
