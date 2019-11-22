/**
 * @file mdv_objects.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief DB objects storage
 * @version 0.1
 * @date 2019-11-15
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include <mdv_def.h>
#include <mdv_types.h>


/// DB objects storage
typedef struct mdv_objects mdv_objects;


/**
 * @brief Creates new or opens existing DB objects storage
 *
 * @param root_dir [in]     Root directory for DB objects storage
 * @param storage_name [in] storage name
 *
 * @return DB objects storage
 */
mdv_objects * mdv_objects_open(char const *root_dir, char const *storage_name);


/**
 * @brief Add reference counter
 *
 * @param objs [in] DB objects storage
 *
 * @return pointer which provided as argument
 */
mdv_objects * mdv_objects_retain(mdv_objects *objs);


/**
 * @brief Decrement references counter. Storage is closed and memory is freed if references counter is zero.
 *
 * @param objs [in] DB objects storage
 *
 * @return references counter
 */
uint32_t mdv_objects_release(mdv_objects *objs);


/**
 * @brief Reserves identifiers range for objects
 *
 * @param objs [in]     DB objects storage
 * @param range [in]    Identifiers range length
 *
 * @return first identifier from range
 */
uint64_t mdv_objects_reserve_ids_range(mdv_objects *objs, uint32_t range);


/**
 * @brief Stores information about the new object.
 *
 * @param objs [in]     DB objects storage
 * @param id [in]       Unique object identifier
 * @param objs_arr [in] Serialized objects array
 *
 * @return On success, return MDV_OK.
 * @return On error, return non zero value
 */
mdv_errno mdv_objects_add(mdv_objects *objs, mdv_data const *id, mdv_data const *obj);


/**
 * @brief Reads and returns the stored object
 *
 * @param objs [in]     DB objects storage
 * @param id [in]       Unique object identifier
 * @param restore [in]  Function for object reading (or deserialization)
 *
 * @return On success, returns nonzero object pointer
 * @return On error, returns NULL
 */
void * mdv_objects_get(mdv_objects *objs, mdv_data const *id, void * (*restore)(mdv_data const *));
