#ifndef SAFETY_SMART_POINTER_H
#define SAFETY_SMART_POINTER_H

#include <stdexcept>
#include <string>
#include <utility>

namespace safety {

/**
 * SmartPointer - RAII wrapper for C-style resources
 * 
 * Enforces memory safety by:
 * - Automatic resource cleanup via RAII
 * - Exception on creation failure
 * - Non-copyable (prevents double-free)
 * - Movable (allows transfer of ownership)
 * 
 * Template parameters:
 * - T: Resource type
 * - CreateFn: Function pointer type for resource creation (const char* -> T*)
 * - DestroyFn: Function pointer type for resource destruction (T* -> void)
 * 
 * Usage:
 *   safety::SmartPointer<Resource, decltype(&create_resource), decltype(&destroy_resource)>
 *     ptr("arg", create_resource, destroy_resource);
 */
template<typename T, typename CreateFn, typename DestroyFn>
class SmartPointer {
public:
    /**
     * Construct SmartPointer from string argument
     * Calls CreateFn with arg.c_str() to create resource
     * Throws std::runtime_error if creation fails (returns nullptr)
     * 
     * @param arg String argument passed to CreateFn
     * @param createFn Function to create the resource
     * @param destroyFn Function to destroy the resource
     * @throws std::runtime_error if CreateFn returns nullptr
     */
    SmartPointer(const std::string& arg, CreateFn createFn, DestroyFn destroyFn)
        : ptr_(nullptr), destroyFn_(destroyFn) {
        ptr_ = createFn(arg.c_str());
        if (!ptr_) {
            throw std::runtime_error("SmartPointer: Resource creation failed for argument: " + arg);
        }
    }

    /**
     * Destructor - automatically calls DestroyFn on resource
     * Marked noexcept - destructor must not throw
     */
    ~SmartPointer() noexcept {
        if (ptr_) {
            destroyFn_(ptr_);
            ptr_ = nullptr;
        }
    }

    // Non-copyable (prevents double-free)
    SmartPointer(const SmartPointer&) = delete;
    SmartPointer& operator=(const SmartPointer&) = delete;

    // Movable (allows transfer of ownership)
    SmartPointer(SmartPointer&& other) noexcept
        : ptr_(other.ptr_), destroyFn_(other.destroyFn_) {
        other.ptr_ = nullptr;
    }

    SmartPointer& operator=(SmartPointer&& other) noexcept {
        if (this != &other) {
            // Clean up current resource
            if (ptr_) {
                destroyFn_(ptr_);
            }
            // Transfer ownership
            ptr_ = other.ptr_;
            destroyFn_ = other.destroyFn_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    /**
     * Get raw pointer to resource
     * Use only for passing to C APIs
     * Do not delete the returned pointer
     * 
     * @return Raw pointer to resource, or nullptr if moved
     */
    T* get() const noexcept {
        return ptr_;
    }

    /**
     * Check if SmartPointer owns a resource
     * 
     * @return true if owns a resource, false if nullptr or moved
     */
    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

private:
    T* ptr_;
    DestroyFn destroyFn_;
};

} // namespace safety

#endif // SAFETY_SMART_POINTER_H
