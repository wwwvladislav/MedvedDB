/**
 * @file
 * @brief Bloom filter
 */
#pragma once
#include <math.h>
#include <stdint.h>
#include <stdbool.h>


/// Bloom filter
typedef struct mdv_bloom mdv_bloom;


/**
 * @brief Create new bloom filter
 *
 * @param entries [in]  Number of entries in bloom filter (i.e. capacity)
 * @param err [in]      Collisions probability
 *
 * @return On success return nonzero pointer to new created bloom filter
 * @return On error return NULL pointer
 */
mdv_bloom * mdv_bloom_create(uint32_t entries, double err);


/**
 * @brief Deallocate bloom filter
 *
 * @param bloom [in] Bloom filter
 */
void mdv_bloom_free(mdv_bloom *bloom);


/**
 * @brief Insert new item into a Bloom filter
 *
 * @param bloom [in] Bloom filter
 * @param data [in]  data
 * @param len [in]   data size
 *
 * @return True     if data is successfully inserted
 * @return False    if collision is found and item isn't inserted
 */
bool mdv_bloom_insert(mdv_bloom *bloom, void const *data, uint32_t const len);


/**
 * @brief Check data in bloom filter
 *
 * @param bloom [in] Bloom filter
 * @param data [in]  data
 * @param len [in]   data size
 *
 * @return True     if bloom filter probably contains the data
 * @return False    if data isn't inserted into a Bloom filter
 */
bool mdv_bloom_contains(mdv_bloom *bloom, void const *data, uint32_t const len);


/**
 * @brief Return Bloom filter capacity
 *
 * @param bloom [in] Bloom filter
 *
 * @return Bloom filter capacity
 */
uint32_t mdv_bloom_capacity(mdv_bloom const *bloom);


/**
 * @brief Return number of items inserted into a Bloom filter
 *
 * @param bloom [in] Bloom filter
 *
 * @return number of items inserted into a Bloom filter
 */
uint32_t mdv_bloom_size(mdv_bloom const *bloom);

