#include "VectorIndex.h"

VectorIndex::VectorIndex(int dimension)
    : m_dim(dimension), m_hnsw(dimension, 32), m_index(&m_hnsw) {}

void VectorIndex::add(int64_t id, const std::vector<float>& embedding) {
    std::lock_guard<std::mutex> lock(m_mutex);
    faiss::idx_t idx_id = id;
    m_index.add_with_ids(1, embedding.data(), &idx_id);
}

std::vector<int64_t> VectorIndex::search(const std::vector<float>& query, int k) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<float> distances(k);
    std::vector<faiss::idx_t> indices(k);

    m_index.search(1, query.data(), k, distances.data(), indices.data());

    std::vector<int64_t> results;
    for (int i = 0; i < k; ++i) {
        if (indices[i] != -1) {
            results.push_back(indices[i]);
        }
    }
    return results;
}