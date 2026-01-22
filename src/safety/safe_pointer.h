#ifndef SAFETY_SAFE_POINTER_H
#define SAFETY_SAFE_POINTER_H

#include <utility>

namespace safety {

/**
 * SafePointer - Non-throwing RAII wrapper for C-style resources
 * 
 * Use everywhere, including worker threads and realtime code.
 * Does NOT throw exceptions - returns nullptr on failure.
 * 
 * Enforces memory safety by:
 * - Automatic resource cleanup via RAII
 * - No exceptions (safe for all contexts)
 * - Non-copyable (prevents double-free)
 * - Movable (allows transfer of ownership)
 * 
 * Template parameters:
 * - T: Resource type
 * - CreateFn: Function pointer type for resource creation (const char* -> T*)
 * - DestroyFn: Function pointer type for resource destruction (T* -> void)
 * 
 * Usage:
 *   safety::SafePointer<Resource, decltype(&create_resource), decltype(&destroy_resource)>
 *     ptr("arg", create_resource, destroy_resource);
 *   
 *   if (ptr) {
 *     // Use ptr.get()
 *   } else {
 *     // Handle failure
 *   }
 */
template<typename T, typename CreateFn, typename DestroyFn>
class SafePointer {
public:
    /**
     * Construct SafePointer from string argument
     * Calls CreateFn with arg.c_str() to create resource
     * Does NOT throw - returns nullptr on failure
     * 
     * @param arg String argument passed to CreateFn
     * @param createFn Function to create the resource
     * @param destroyFn Function to destroy the resource
     */
    SafePointer(const char* arg, CreateFn createFn, DestroyFn destroyFn) noexcept
        : ptr_(nullptr), destroyFn_(destroyFn) {
        if (arg) {
            ptr_ = createFn(arg);
        }
    }

    /**
     * Destructor - automatically calls DestroyFn on resource
     * Marked noexcept - destructor must not throw
     */
    ~SafePointer() noexcept {
        if (ptr_) {
            destroyFn_(ptr_);
            ptr_ = nullptr;
        }
    }

    // Non-copyable (prevents double-free)
    SafePointer(const SafePointer&) = delete;
    SafePointer& operator=(const SafePointer&) = delete;

    // Movable (allows transfer of ownership)
    SafePointer(SafePointer&& other) noexcept
        : ptr_(other.ptr_), destroyFn_(other.destroyFn_) {
        other.ptr_ = nullptr;
    }

    SafePointer& operator=(SafePointer&& other) noexcept {
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
     * @return Raw pointer to resource, or nullptr if creation failed or moved
     */
    T* get() const noexcept {
        return ptr_;
    }

    /**
     * Check if SafePointer owns a resource
     * 
     * @return true if owns a resource, false if nullptr, creation failed, or moved
     */
    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    /**
     * Release ownership of the resource without destroying it
     * 
     * @return Raw pointer to resource (caller takes ownership)
     */
    T* release() noexcept {
        T* temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }

private:
    T* ptr_;
    DestroyFn destroyFn_;
};

} // namespace safety

#endif // SAFETY_SAFE_POINTER_H
