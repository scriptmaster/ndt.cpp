#ifndef SAFETY_SAFE_SCOPE_H
#define SAFETY_SAFE_SCOPE_H

namespace safety {

/**
 * SafeScope - Marker for no-throw zones
 * 
 * Use at the beginning of worker threads, real-time loops, or any
 * code path that must not throw exceptions.
 * 
 * In SafeScope contexts:
 *   - No SmartPointer construction allowed
 *   - Use SafePointer or SafeResultPointer instead
 *   - All errors must be handled via error codes or logging
 *   - No exception propagation
 * 
 * Example:
 *   void workerThread() {
 *     safety::SafeScope scope;
 *     
 *     // Safe operations only - no exceptions
 *     while (running) {
 *       processData();
 *     }
 *   }
 */
struct SafeScope {
    SafeScope() noexcept = default;
    ~SafeScope() noexcept = default;
    
    // Non-copyable, non-movable
    SafeScope(const SafeScope&) = delete;
    SafeScope& operator=(const SafeScope&) = delete;
    SafeScope(SafeScope&&) = delete;
    SafeScope& operator=(SafeScope&&) = delete;
};

/**
 * SafeBoundary - Marker for exception boundary zones
 * 
 * Use in startup/initialization functions or other boundary points
 * where exceptions are acceptable and will be handled.
 * 
 * In SafeBoundary contexts:
 *   - SmartPointer construction is allowed (must be in try/catch)
 *   - Resource initialization can throw
 *   - All exceptions must be caught at the boundary
 * 
 * Example:
 *   void initialize() {
 *     safety::SafeBoundary boundary;
 *     
 *     try {
 *       // Create resources that may throw
 *       auto resource = createResource();
 *     } catch (const std::exception& e) {
 *       logError(e.what());
 *       throw; // Re-throw to caller if needed
 *     }
 *   }
 */
struct SafeBoundary {
    SafeBoundary() noexcept = default;
    ~SafeBoundary() noexcept = default;
    
    // Non-copyable, non-movable
    SafeBoundary(const SafeBoundary&) = delete;
    SafeBoundary& operator=(const SafeBoundary&) = delete;
    SafeBoundary(SafeBoundary&&) = delete;
    SafeBoundary& operator=(SafeBoundary&&) = delete;
};

} // namespace safety

#endif // SAFETY_SAFE_SCOPE_H
