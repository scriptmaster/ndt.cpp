#ifndef SAFETY_SMART_POINTER_H
#define SAFETY_SMART_POINTER_H

#include <string>
#include <stdexcept>
#include <utility>

namespace safety {

/**
 * SmartPointer - RAII wrapper for resource management
 * 
 * Template parameters:
 *   T - Type of the managed pointer
 *   CreateFn - Function pointer type for resource creation
 *   DestroyFn - Function pointer type for resource destruction
 * 
 * Usage:
 *   Must be constructed inside a try/catch block (enforced by clang-tidy)
 *   Non-copyable, movable
 *   Automatically calls DestroyFn on destruction
 * 
 * Example:
 *   auto deleter = [](FILE* f) { if (f) fclose(f); };
 *   auto creator = [](const char* path) -> FILE* { return fopen(path, "r"); };
 *   
 *   try {
 *     SmartPointer<FILE*, decltype(creator), decltype(deleter)> file(
 *       "config.txt", creator, deleter
 *     );
 *     // use file.get()
 *   } catch (const std::exception& e) {
 *     // handle error
 *   }
 */
template<typename T, typename CreateFn, typename DestroyFn>
class SmartPointer {
public:
    /**
     * Constructor - creates resource using CreateFn
     * @param arg String argument passed to CreateFn
     * @param create Function to create the resource
     * @param destroy Function to destroy the resource
     * @throws std::runtime_error if creation fails (CreateFn returns nullptr)
     */
    SmartPointer(const std::string& arg, CreateFn create, DestroyFn destroy)
        : ptr_(create(arg.c_str())), destroy_(destroy) {
        if (!ptr_) {
            throw std::runtime_error("SmartPointer: Resource creation failed for: " + arg);
        }
    }

    /**
     * Destructor - automatically cleans up resource
     * Guaranteed noexcept for RAII safety
     */
    ~SmartPointer() noexcept {
        if (ptr_) {
            destroy_(ptr_);
            ptr_ = nullptr;
        }
    }

    // Non-copyable
    SmartPointer(const SmartPointer&) = delete;
    SmartPointer& operator=(const SmartPointer&) = delete;

    // Movable
    SmartPointer(SmartPointer&& other) noexcept
        : ptr_(other.ptr_), destroy_(std::move(other.destroy_)) {
        other.ptr_ = nullptr;
    }

    SmartPointer& operator=(SmartPointer&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
                destroy_(ptr_);
            }
            ptr_ = other.ptr_;
            destroy_ = std::move(other.destroy_);
            other.ptr_ = nullptr;
        }
        return *this;
    }

    /**
     * Get raw pointer for interfacing with C APIs
     * @return Raw pointer to managed resource
     */
    T get() const noexcept {
        return ptr_;
    }

    /**
     * Check if pointer is valid
     * @return true if pointer is non-null
     */
    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

private:
    T ptr_;
    DestroyFn destroy_;
};

} // namespace safety

#endif // SAFETY_SMART_POINTER_H
