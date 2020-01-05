#include "rocksdb_util.h"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

namespace db {

const std::shared_ptr<rocksdb::Options>& DefaultOpenOptions() {
    static std::shared_ptr<rocksdb::Options> options;
    if (!options) {
        options = std::make_shared<rocksdb::Options>();
        options->create_if_missing = true;
    }
    return options;
}

const std::shared_ptr<rocksdb::WriteOptions>& DefaultDBWriteOptions() {
    static std::shared_ptr<rocksdb::WriteOptions> options;
    if (!options) {
        options = std::make_shared<rocksdb::WriteOptions>();
        options->disableWAL = false;
        options->sync = false;
    }
    return options;
}

namespace demo {

// [Key]ID:$tid$uid [Val]$sid$id
void mock_uid_id_mapping(std::shared_ptr<rocksdb::DB> db) {
    srand(time(0));
    int num = 0;
    uint64_t tid = 0;
    uint64_t id = 0;
    uint64_t sid = 0;
    for(int i=0; i<num; i++) {
        uint64_t uid = rand();
        std::string key(DBTableUidIdMappingPrefix);
        key.append((char*)&tid, sizeof(uint64_t));
        key.append((char*)&uid, sizeof(uint64_t));

        std::string val;
        val.append((char*)&sid, sizeof(uint64_t));
        val.append((char*)&id, sizeof(uint64_t));
        auto s = db->Put(*DefaultDBWriteOptions(), key, val);
        if (!s.ok()) {
            std::cout << s.ToString() << std::endl;
            assert(false);
        }
        ++id;
    }
}

void read_all(std::shared_ptr<rocksdb::DB> db, rocksdb::ReadOptions* options, bool do_print) {
    rocksdb::ReadOptions roptions;
    if (options == nullptr) {
        options = &roptions;
    }
    size_t count = 0;
    rocksdb::Iterator* it = db->NewIterator(*options);
    std::map<std::string, std::string> dict;
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        count++;
        if (!do_print) continue;
        auto key = it->key();
        auto val = it->value();
        if (key.starts_with(DBTablePrefix)) {
            std::cout << "[" << key.ToString() << ", " << *(uint64_t*)(it->value().data()) << "]" << std::endl;
        } else if (key.starts_with(DBTableCurrentSegmentPrefix)) {
            auto sid_addr = key.data() + DBTableCurrentSegmentPrefix.size();
            std::cout << "[" << DBTableCurrentSegmentPrefix << *(uint64_t*)(sid_addr) << ", " << *(uint64_t*)(it->value().data()) << "]" << std::endl;
        } else if (key.starts_with(DBTableSequenceKey)) {
            std::cout << "[" << DBTableSequenceKey << ", " << *(uint64_t*)(it->value().data()) << "]" << std::endl;
        } else if (key.starts_with(DBTableSegmentNextIDPrefix)) {
            auto tid_addr = (uint64_t*)(key.data() + DBTableSegmentNextIDPrefix.size());
            auto sid_addr = (uint64_t*)(key.data() + DBTableSegmentNextIDPrefix.size() + 1 + sizeof(uint64_t));
            std::cout << "[" << DBTableSegmentNextIDPrefix << *tid_addr << ":" << *sid_addr;
            std::cout << ", " << *(uint64_t*)(it->value().data()) << "]" << std::endl;
        } else if (key.starts_with(DBTableUidIdMappingPrefix)) {
            auto tid_addr = (uint64_t*)(key.data() + DBTableUidIdMappingPrefix.size());
            auto uid_addr = (uint64_t*)(key.data() + DBTableUidIdMappingPrefix.size() + sizeof(uint64_t));

            auto sid_addr = (uint64_t*)(val.data());
            auto id_addr = (uint64_t*)(val.data() + sizeof(uint64_t));

            std::cout << "[" << DBTableUidIdMappingPrefix << *tid_addr << ":" << *uid_addr;
            std::cout << ", " << *sid_addr << ":" << *id_addr << "]" << std::endl;
        }
    }
    std::cout << "Found " << count << " KVs" << std::endl;
}

void write_batch_demo(std::shared_ptr<rocksdb::DB> db) {
    std::vector<float> vec(512);
    for (auto i=0; i<512; ++i) {
        vec[i] = 0.0001 * i;
    }

    rocksdb::WriteBatch write_batch;
    size_t cap = 8000;
    std::vector<std::string> keys;
    std::vector<std::string> values;
    for (auto i=0; i<cap; ++i) {
        std::string aname_key = "A:";
        std::string aname_val;
        aname_key.append((char*)&i, (size_t)(sizeof(int)));
        aname_val.append((char*)&i, (size_t)(sizeof(int)));

        keys.push_back(std::move(aname_key));
        values.push_back(std::move(aname_val));

        std::string bname_key = "B:";
        std::string bname_val;
        bname_key.append((char*)&i, (size_t)(sizeof(int)));
        bname_val.append((char*)&i, (size_t)(sizeof(int)));

        keys.push_back(std::move(bname_key));
        values.push_back(std::move(bname_val));

        std::string cname_key = "C:";
        std::string cname_val;
        cname_key.append((char*)&i, (size_t)(sizeof(int)));
        cname_val.append((char*)&i, (size_t)(sizeof(int)));
        write_batch.Put(cname_key, cname_val);

        keys.push_back(std::move(cname_key));
        values.push_back(std::move(cname_val));
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (auto i=0; i<keys.size(); ++i) {
        write_batch.Put(keys[i], values[i]);
    }

    std::cout << write_batch.Count() << std::endl;
    auto s = db->Write(*DefaultDBWriteOptions(), &write_batch);

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << __func__ << " takes " << std::chrono::duration<double, std::milli>(end-start).count() << " ms" << std::endl;

    if (!s.ok()) {
        std::cout << s.ToString() << std::endl;
    }
    /* read_all(db); */
}

}

}