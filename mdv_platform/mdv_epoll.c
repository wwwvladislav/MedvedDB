#include "mdv_epoll.h"
#include "mdv_platform.h"
#include "mdv_log.h"
#include "mdv_errno.h"
#include <unistd.h>

#ifdef MDV_PLATFORM_LINUX
    #include <sys/epoll.h>
#endif


mdv_descriptor mdv_epoll_create()
{
    int epoll_fd = epoll_create1(0);

    if (epoll_fd < 0)
    {
        int err = mdv_error();
        MDV_LOGE("Epoll creation was failed with error: '%s' %d", mdv_strerror(err), err);
        return MDV_INVALID_DESCRIPTOR;
    }

    MDV_LOGD("Epoll %d opened", epoll_fd);

    mdv_descriptor fd = 0;

    *(int*)&fd = epoll_fd;

    return fd;
}


void mdv_epoll_close(mdv_descriptor epfd)
{
    if (epfd != MDV_INVALID_DESCRIPTOR)
    {
        int s = *(int*)&epfd;
        close(s);
        MDV_LOGD("Epoll %d closed", s);
    }
}


inline uint32_t mdv_epoll_to_platform_events(uint32_t events)
{
    uint32_t platform_events = 0;

    if (events & MDV_EPOLLIN)           platform_events |= EPOLLIN;
    if (events & MDV_EPOLLOUT)          platform_events |= EPOLLOUT;
    if (events & MDV_EPOLLRDHUP)        platform_events |= EPOLLRDHUP;
    if (events & MDV_EPOLLERR)          platform_events |= EPOLLERR;
    if (events & MDV_EPOLLET)           platform_events |= EPOLLET;
    if (events & MDV_EPOLLONESHOT)      platform_events |= EPOLLONESHOT;
    if (events & MDV_EPOLLEXCLUSIVE)    platform_events |= EPOLLEXCLUSIVE;

    return platform_events;
}


inline uint32_t mdv_epoll_from_platform_events(uint32_t platform_events)
{
    uint32_t events = 0;

    if (platform_events & EPOLLIN)           events |= MDV_EPOLLIN;
    if (platform_events & EPOLLOUT)          events |= MDV_EPOLLOUT;
    if (platform_events & EPOLLRDHUP)        events |= MDV_EPOLLRDHUP;
    if (platform_events & EPOLLERR)          events |= MDV_EPOLLERR;
    if (platform_events & EPOLLET)           events |= MDV_EPOLLET;
    if (platform_events & EPOLLONESHOT)      events |= MDV_EPOLLONESHOT;
    if (platform_events & EPOLLEXCLUSIVE)    events |= MDV_EPOLLEXCLUSIVE;

    return events;
}


mdv_errno mdv_epoll_add(mdv_descriptor epfd, mdv_descriptor fd, mdv_epoll_event event)
{
    int epoll_fd = *(int*)&epfd;

    struct epoll_event epoll_event = {0};
    epoll_event.data.ptr = event.data;
    epoll_event.events = mdv_epoll_to_platform_events(event.events);

    int err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, *(int*)&fd, &epoll_event);

    if(err == -1)
    {
        err = mdv_error();
        MDV_LOGE("epoll_add failed due the error '%s' (%d)", mdv_strerror(err), err);
        return err;
    }

    return MDV_OK;
}


mdv_errno mdv_epoll_del(mdv_descriptor epfd, mdv_descriptor fd)
{
    int epoll_fd = *(int*)&epfd;

    int err = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, *(int*)&fd, 0);

    if(err == -1)
    {
        err = mdv_error();
        MDV_LOGE("epoll_del failed due the error '%s' (%d)", mdv_strerror(err), err);
        return err;
    }

    return MDV_OK;
}


mdv_errno mdv_epoll_wait(mdv_descriptor epfd, mdv_epoll_event *events, uint32_t *size, int timeout)
{
    int epoll_fd = *(int*)&epfd;

    struct epoll_event epoll_events[*size];

    int err = epoll_wait(epoll_fd, epoll_events, *size, timeout);

    if (err == -1)
    {
        err = mdv_error();
        MDV_LOGE("epoll_wait failed due the error '%s' (%d)", mdv_strerror(err), err);
        return err;
    }

    uint32_t const ret_events = (uint32_t)err;

    for(uint32_t i = 0; i < ret_events; ++i)
    {
        events[i].data = epoll_events[i].data.ptr;
        events[i].events = mdv_epoll_from_platform_events(epoll_events[i].events);
    }

    *size = ret_events;

    return MDV_OK;
}

