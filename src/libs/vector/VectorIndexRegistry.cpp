#include "VectorIndexRegistry.h"
#include "db/Database.h"

VectorIndexRegistry::VectorIndexRegistry(int dimension, Database* db)
    : _dimension(dimension), _db(db) {}

VectorIndex& VectorIndexRegistry::getOrCreate(int jobId)
{
    QMutexLocker locker(&_mutex);
    auto it = _indicesByJobId.find(jobId);
    if (it == _indicesByJobId.end()) {
        auto index = std::make_unique<VectorIndex>(_dimension);
        if (_db) {
            _db->loadEmbeddingsForJob(jobId, *index);
        }
        it = _indicesByJobId.emplace(jobId, std::move(index)).first;
    }
    return *(it->second);
}

void VectorIndexRegistry::remove(int jobId)
{
    QMutexLocker locker(&_mutex);
    _indicesByJobId.erase(jobId);
}
