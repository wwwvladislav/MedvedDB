#pragma once
#include "mdv_platform/mdv_filesystem.h"
#include "mdv_platform/mdv_stack.h"
#include "mdv_platform/mdv_vector.h"
#include "mdv_platform/mdv_rollbacker.h"
#include "mdv_platform/mdv_queue.h"
#include "mdv_platform/mdv_hashmap.h"
#include "mdv_platform/mdv_list.h"
#include "mdv_platform/mdv_string.h"
#include "mdv_platform/mdv_bloom.h"
#include "mdv_platform/mdv_socket.h"
#include "mdv_platform/mdv_eventfd.h"
#include "mdv_platform/mdv_condvar.h"
#include "mdv_platform/mdv_queuefd.h"
#include "mdv_platform/mdv_threadpool.h"
#include "mdv_platform/mdv_chaman.h"
#include "mdv_platform/mdv_dispatcher.h"
#include "mdv_platform/mdv_jobber.h"
#include "mdv_platform/mdv_ebus.h"
#include "mdv_platform/mdv_algorithm.h"
#include "mdv_platform/mdv_topology.h"
#include "mdv_platform/mdv_mst.h"
#include "mdv_platform/mdv_bitset.h"


MU_TEST_SUITE(platform)
{
    MU_RUN_TEST(platform_mkdir);
    MU_RUN_TEST(platform_stack);
    MU_RUN_TEST(platform_vector);
    MU_RUN_TEST(platform_rollbacker);
    MU_RUN_TEST(platform_queue);
    MU_RUN_TEST(platform_hashmap);
    MU_RUN_TEST(platform_list);
    MU_RUN_TEST(platform_string);
    MU_RUN_TEST(platform_bloom);
    MU_RUN_TEST(platform_bitset);
    MU_RUN_TEST(platform_socket);
    MU_RUN_TEST(platform_eventfd);
    MU_RUN_TEST(platform_condvar);
    MU_RUN_TEST(platform_queuefd);
    MU_RUN_TEST(platform_threadpool);
    MU_RUN_TEST(platform_chaman);
    MU_RUN_TEST(platform_dispatcher);
    MU_RUN_TEST(platform_jobber);
    MU_RUN_TEST(platform_ebus);
    MU_RUN_TEST(platform_algorithm);
    MU_RUN_TEST(platform_topology);
    MU_RUN_TEST(platform_mst);
}
