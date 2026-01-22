#ifndef SAFETY_FOREIGN_POINTER_H
#define SAFETY_FOREIGN_POINTER_H

#include <utility>

namespace safety {

/**
 * ForeignPointer - RAII wrapper for C/FFI/GPU/OS handles
 * 
 * Use for owned C API handles, foreign function interface resources,
 * GPU contexts, OS handles, and other external library resources.
 * 
 * Enforces memory safety by:
 * - Automatic resource cleanup via RAII
 * - Non-copyable (prevents double-free)
 * - Movable (allows transfer of ownership)
 * - Custom deleter support for any cleanup function
 * 
 * Template parameters:
 * - T: Handle/resource type (can be pointer or opaque handle type)
 * - Deleter: Functor type that cleans up the resource
 * 
 * Usage:
 *   struct WhisperDeleter {
 *     void operator()(whisper_context* ctx) const noexcept {
 *       if (ctx) whisper_free(ctx);
 *     }
 *   };
 * 
 *   safety::ForeignPointer<whisper_context*, WhisperDeleter> ctx;
 *   ctx.reset(whisper_init_from_file("model.bin"));
 *   if (ctx) {
 *     whisper_full(ctx.get(), ...);
 *   }
 */
template<typename T, typename Deleter>
class ForeignPointer {
public:
    /**
     * Construct empty ForeignPointer
     */
    ForeignPointer() noexcept : ptr_(nullptr), deleter_() {}

    /**
     * Construct ForeignPointer with resource
     * 
     * @param ptr Resource to manage (can be nullptr)
     */
    explicit ForeignPointer(T ptr) noexcept : ptr_(ptr), deleter_() {}

    /**
     * Construct ForeignPointer with resource and custom deleter
     * 
     * @param ptr Resource to manage (can be nullptr)
     * @param deleter Custom deleter instance
     */
    ForeignPointer(T ptr, const Deleter& deleter) noexcept 
        : ptr_(ptr), deleter_(deleter) {}

    /**
     * Destructor - automatically calls Deleter on resource
     * Marked noexcept - destructor must not throw
     */
    ~ForeignPointer() noexcept {
        if (ptr_) {
            deleter_(ptr_);
        }
    }

    // Non-copyable (prevents double-free)
    ForeignPointer(const ForeignPointer&) = delete;
    ForeignPointer& operator=(const ForeignPointer&) = delete;

    // Movable (allows transfer of ownership)
    ForeignPointer(ForeignPointer&& other) noexcept
        : ptr_(other.ptr_), deleter_(std::move(other.deleter_)) {
        other.ptr_ = nullptr;
    }

    ForeignPointer& operator=(ForeignPointer&& other) noexcept {
        if (this != &other) {
            // Clean up current resource
            if (ptr_) {
                deleter_(ptr_);
            }
            // Transfer ownership
            ptr_ = other.ptr_;
            deleter_ = std::move(other.deleter_);
            other.ptr_ = nullptr;
        }
        return *this;
    }

    /**
     * Get raw pointer/handle to resource
     * Use for passing to C APIs
     * Do not delete the returned pointer/handle
     * 
     * @return Raw pointer/handle to resource, or nullptr if empty or moved
     */
    T get() const noexcept {
        return ptr_;
    }

    /**
     * Check if ForeignPointer owns a resource
     * 
     * @return true if owns a resource, false if nullptr or moved
     */
    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    /**
     * Release ownership of the resource without destroying it
     * 
     * @return Raw pointer/handle to resource (caller takes ownership)
     */
    T release() noexcept {
        T temp = ptr_;
        ptr_ = nullptr;
        return temp;
    }

    /**
     * Replace the managed resource
     * Deletes the current resource (if any) and takes ownership of the new one
     * 
     * @param ptr New resource to manage (can be nullptr)
     */
    void reset(T ptr = nullptr) noexcept {
        if (ptr_) {
            deleter_(ptr_);
        }
        ptr_ = ptr;
    }

    /**
     * Get reference to the deleter
     * 
     * @return Reference to the deleter functor
     */
    Deleter& get_deleter() noexcept {
        return deleter_;
    }

    /**
     * Get const reference to the deleter
     * 
     * @return Const reference to the deleter functor
     */
    const Deleter& get_deleter() const noexcept {
        return deleter_;
    }

private:
    T ptr_;
    Deleter deleter_;
};

} // namespace safety

#endif // SAFETY_FOREIGN_POINTER_H
