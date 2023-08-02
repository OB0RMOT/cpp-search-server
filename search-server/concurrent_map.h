#pragma once
#include <mutex>
#include <map>
#include <vector>

template <typename Key, typename Value>
class ConcurrentMap
{
private:
    struct Bucket
    {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access
    {
        std::lock_guard<std::mutex> guard;
        Value &ref_to_value;

        Access(const Key &key, Bucket &bucket)
            : guard(bucket.mutex), ref_to_value(bucket.map[key])
        {
        }

        void operator+=(const Value &value)
        {
            ref_to_value += value;
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count)
    {
    }

    Access operator[](const Key &key)
    {
        auto &bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return {key, bucket};
    }

    std::map<Key, Value> BuildOrdinaryMap()
    {
        std::map<Key, Value> result;
        for (auto &[mutex, map] : buckets_)
        {
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

    void Erase(const int &key)
    {
        std::lock_guard(buckets_[static_cast<uint64_t>(key) % buckets_.size()].mutex);
        buckets_[static_cast<uint64_t>(key) % buckets_.size()].map.erase(key);
    }

private:
    std::vector<Bucket> buckets_;
};