#pragma once
#include <vector>
#include <mutex>
#include <faiss/IndexHNSW.h>
#include <faiss/IndexIDMap.h>

class VectorIndex {
public:
    explicit VectorIndex(int dimension);
    ~VectorIndex() = default;

    void add(int64_t id, const std::vector<float>& embedding);
    std::vector<int64_t> search(const std::vector<float>& query, int k);

private:
    int m_dim;
    std::mutex m_mutex;
    faiss::IndexHNSWFlat m_hnsw;
    faiss::IndexIDMap m_index;
};