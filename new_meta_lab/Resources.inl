#include "Resources.h"
#include <sstream>
#include <iostream>

template <typename Derived>
DBBaseResource<Derived>::DBBaseResource(ID_TYPE id, State status, TS_TYPE created_on) :
    id_(id), status_(status), created_on_(created_on) {
}

template <typename Derived>
std::string DBBaseResource<Derived>::ToString() const {
    std::stringstream ss;
    ss << "ID=" << id_ << ", Status=" << status_ << ", TS=" << created_on_;
    return ss.str();
}

Collection::Collection(ID_TYPE id, const std::string& name, State status, TS_TYPE created_on) :
    BaseT(id, status, created_on),
    name_(name) {
}

std::string Collection::ToString() const {
    std::stringstream ss;
    ss << "<" << BaseT::ToString() << ", Name=" << name_ << ">";
    return ss.str();
}

template <typename ResourceT, typename Derived>
void ResourceHolder<ResourceT, Derived>::Dump(const std::string& tag) {
    std::unique_lock<std::mutex> lock(mutex_);
    std::cout << typeid(*this).name() << " Dump Start [" << tag <<  "]:" << id_map_.size() << std::endl;
    /* std::cout << "ResourceHolder Dump Start [" << tag <<  "]:" << id_map_.size() << std::endl; */
    for (auto& kv : id_map_) {
        std::cout << "\t" << kv.second->ToString() << std::endl;
    }
    std::cout << typeid(*this).name() << " Dump   End [" << tag <<  "]" << std::endl;
}

template <typename ResourceT, typename Derived>
typename ResourceHolder<ResourceT, Derived>::ResourcePtr ResourceHolder<ResourceT, Derived>::GetResource(ID_TYPE id) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto cit = id_map_.find(id);
    if (cit == id_map_.end()) {
        return nullptr;
    }
    return cit->second;
}

template <typename ResourceT, typename Derived>
bool ResourceHolder<ResourceT, Derived>::RemoveNoLock(ID_TYPE id) {
    /* std::unique_lock<std::mutex> lock(mutex_); */
    auto it = id_map_.find(id);
    if (it == id_map_.end()) {
        return false;
    }

    id_map_.erase(it);
    return true;
}

template <typename ResourceT, typename Derived>
bool ResourceHolder<ResourceT, Derived>::Remove(ID_TYPE id) {
    std::unique_lock<std::mutex> lock(mutex_);
    return RemoveNoLock(id);
}

template <typename ResourceT, typename Derived>
bool ResourceHolder<ResourceT, Derived>::AddNoLock(typename ResourceHolder<ResourceT, Derived>::ResourcePtr resource) {
    if (!resource) return false;
    /* std::unique_lock<std::mutex> lock(mutex_); */
    if (id_map_.find(resource->GetID()) != id_map_.end()) {
        return false;
    }

    id_map_[resource->GetID()] = resource;
    return true;
}

template <typename ResourceT, typename Derived>
bool ResourceHolder<ResourceT, Derived>::Add(typename ResourceHolder<ResourceT, Derived>::ResourcePtr resource) {
    std::unique_lock<std::mutex> lock(mutex_);
    return AddNoLock(resource);
}

CollectionsHolder::ResourcePtr
CollectionsHolder::GetCollection(const std::string& name) {
    std::unique_lock<std::mutex> lock(BaseT::mutex_);
    auto cit = name_map_.find(name);
    if (cit == name_map_.end()) {
        return nullptr;
    }
    return cit->second;
}

bool CollectionsHolder::Add(CollectionsHolder::ResourcePtr resource) {
    if (!resource) return false;
    std::unique_lock<std::mutex> lock(BaseT::mutex_);
    return BaseT::AddNoLock(resource);
}

bool CollectionsHolder::Remove(const std::string& name) {
    std::unique_lock<std::mutex> lock(BaseT::mutex_);
    auto it = name_map_.find(name);
    if (it == name_map_.end()) {
        return false;
    }

    BaseT::id_map_.erase(it->second->GetID());
    name_map_.erase(it);
    return true;
}

bool CollectionsHolder::Remove(ID_TYPE id) {
    std::unique_lock<std::mutex> lock(mutex_);
    return BaseT::RemoveNoLock(id);
}

CollectionCommit::CollectionCommit(ID_TYPE id, ID_TYPE collection_id,
        const MappingT& mappings, State status, TS_TYPE created_on) :
    BaseT(id, status, created_on), collection_id_(collection_id), mappings_(mappings) {
}

std::string CollectionCommit::ToString() const {
    std::stringstream ss;
    ss << "<" << BaseT::ToString() << ", Mappings=" << "[";
    bool first = true;
    std::string prefix;
    for (auto& id : mappings_) {
        if (!first) prefix = ", ";
        else first = false;
        ss << prefix << id;
    }
    ss << "]>";
    return ss.str();
}