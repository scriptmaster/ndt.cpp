#ifndef SAFETY_SAFE_RESULT_POINTER_H
#define SAFETY_SAFE_RESULT_POINTER_H

#include <utility>
#include <optional>
#include <string>

namespace safety {

/**
 * SafeResultPointer - Non-throwing RAII wrapper with error message
 * 
 * Use in APIs and worker threads where error information is needed
 * without throwing exceptions.
 * 
 * Enforces memory safety by:
 * - Automatic resource cleanup via RAII
 * - No exceptions (safe for all contexts)
 * - Error message on failure
 * - Non-copyable (prevents double-free)
 * - Movable (allows transfer of ownership)
 * 
 * Template parameters:
 * - T: Resource type
 * - CreateFn: Function pointer type for resource creation (const char* -> T*)
 * - DestroyFn: Function pointer type for resource destruction (T* -> void)
 * 
 * Usage:
 *   safety::SafeResultPointer<Resource, decltype(&create_resource), decltype(&destroy_resource)>
 *     ptr("arg", create_resource, destroy_resource);
 *   
 *   if (ptr) {
 *     // Use ptr.get()
 *   } else {
 *     // Log error: ptr.error()
 *   }
 */
template<typename T, typename CreateFn, typename DestroyFn>
class SafeResultPointer {
public:
    /**
     * Construct SafeResultPointer from string argument
     * Calls CreateFn with arg.c_str() to create resource
     * Does NOT throw - stores error message on failure
     * 
     * @param arg String argument passed to CreateFn
     * @param createFn Function to create the resource
     * @param destroyFn Function to destroy the resource
     */
    SafeResultPointer(const char* arg, CreateFn createFn, DestroyFn destroyFn) noexcept
        : ptr_(nullptr), destroyFn_(destroyFn), error_() {
        if (!arg) {
            error_ = "SafeResultPointer: null argument";
            return;
        }
        
        ptr_ = createFn(arg);
        if (!ptr_) {
            error_ = std::string("SafeResultPointer: Resource creation failed for argument: ") + arg;
        }
    }

    /**
     * Destructor - automatically calls DestroyFn on resource
     * Marked noexcept - destructor must not throw
     */
    ~SafeResultPointer() noexcept {
        if (ptr_) {
            destroyFn_(ptr_);
            ptr_ = nullptr;
        }
    }

    // Non-copyable (prevents double-free)
    SafeResultPointer(const SafeResultPointer&) = delete;
    SafeResultPointer& operator=(const SafeResultPointer&) = delete;

    // Movable (allows transfer of ownership)
    SafeResultPointer(SafeResultPointer&& other) noexcept
        : ptr_(other.ptr_), destroyFn_(other.destroyFn_), error_(std::move(other.error_)) {
        other.ptr_ = nullptr;
    }

    SafeResultPointer& operator=(SafeResultPointer&& other) noexcept {
        if (this != &other) {
            // Clean up current resource
            if (ptr_) {
                destroyFn_(ptr_);
            }
            // Transfer ownership
            ptr_ = other.ptr_;
            destroyFn_ = other.destroyFn_;
            error_ = std::move(other.error_);
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
     * Check if SafeResultPointer owns a resource
     * 
     * @return true if owns a resource, false if nullptr, creation failed, or moved
     */
    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    /**
     * Get error message if resource creation failed
     * 
     * @return Error message, or empty string if successful
     */
    const std::string& error() const noexcept {
        if (error_.has_value()) {
            return error_.value();
        }
        static const std::string empty;
        return empty;
    }

    /**
     * Check if there was an error during creation
     * 
     * @return true if error occurred, false if successful
     */
    bool has_error() const noexcept {
        return error_.has_value();
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
    std::optional<std::string> error_;
};

} // namespace safety

#endif // SAFETY_SAFE_RESULT_POINTER_H
