#include <ArduinoJson.h>

namespace {
class StaticAllocator : public ArduinoJson::Allocator {
public:
    StaticAllocator(uint8_t* buf, size_t cap) : buffer_(buf), capacity_(cap), offset_(0) {}
    void* allocate(size_t size) override {
        if (offset_ + size > capacity_) return nullptr;
        void* p = buffer_ + offset_;
        offset_ += size;
        return p;
    }
    void deallocate(void*) override {}
    void* reallocate(void*, size_t) override { return nullptr; }
private:
    uint8_t* buffer_;
    size_t capacity_;
    size_t offset_;
};
} // namespace
