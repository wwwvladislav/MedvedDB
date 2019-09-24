/**
 * @file mdv_idmap.h
 * @author Vladislav Volkov (wwwvladislav@gmail.com)
 * @brief Persistent storage for identifiers map
 * @version 0.1
 * @date 2019-09-22
 *
 * @copyright Copyright (c) 2019, Vladislav Volkov
 *
 */
#pragma once
#include "mdv_storage.h"


/// Identifiers map descriptor
typedef struct mdv_idmap mdv_idmap;


/**
 * @brief Opens or creates identifiers map
 *
 * @param storage [in]  Key-value storage descriptor
 * @param name [in]     Table name (should be unique in storage)
 * @param size [in]     Map size
 *
 * @return identifiers map pointer
 */
mdv_idmap * mdv_idmap_open(mdv_storage *storage, char const *name, size_t size);


/**
 * @brief Frees identifiers map opened by mdv_idmap_open()
 *
 * @param idmap [in]
 */
void mdv_idmap_free(mdv_idmap *idmap);


/**
 * @brief Returns a reference to the element at specified location pos, with bounds checking.
 *
 * @param idmap [in]    Identifiers map
 * @param pos [in]      position of the element
 * @param val [out]     place for the element to return
 *
 * @return true if element successfully returned
 */
bool mdv_idmap_at(mdv_idmap *idmap, uint64_t pos, uint64_t *val);


/**
 * @brief Changes value at specified position
 *
 * @param idmap [in]    Identifiers map
 * @param pos [in]      position of the element
 * @param val [in]      new element to set
 *
 * @return true if element successfully changed
 */
bool mdv_idmap_set(mdv_idmap *idmap, uint64_t pos, uint64_t val);

