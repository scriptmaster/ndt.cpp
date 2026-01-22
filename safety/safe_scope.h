#ifndef SAFETY_SAFE_SCOPE_H
#define SAFETY_SAFE_SCOPE_H

namespace safety {

/**
 * SafeScope - Marker for no-throw zones
 * 
 * Use at the beginning of worker threads, realtime loops, and other
 * contexts where exceptions must not escape.
 * 
 * In SafeScope contexts:
 * - SmartPointer construction is NOT allowed (throws exceptions)
 * - Use SafePointer or SafeResultPointer instead (future implementation)
 * - All operations must be noexcept or handle exceptions internally
 * 
 * Example:
 *   void workerLoop() {
 *       safety::SafeScope scope;
 *       // Worker code here - no SmartPointer allowed
 *   }
 */
struct SafeScope {
    SafeScope() noexcept = default;
    ~SafeScope() noexcept = default;
    
    // Non-copyable, non-movable (scope marker)
    SafeScope(const SafeScope&) = delete;
    SafeScope& operator=(const SafeScope&) = delete;
    SafeScope(SafeScope&&) = delete;
    SafeScope& operator=(SafeScope&&) = delete;
};

/**
 * SafeBoundary - Marker for exception boundaries
 * 
 * Use at the beginning of startup, initialization, and boundary functions
 * where exceptions are allowed and handled.
 * 
 * In SafeBoundary contexts:
 * - SmartPointer construction MUST be inside try/catch blocks
 * - Exceptions must be caught and handled appropriately
 * - Startup failures should be logged and propagated
 * 
 * Example:
 *   void init() {
 *       safety::SafeBoundary boundary;
 *       try {
 *           safety::SmartPointer<...> resource(...);
 *           startSystem(resource.get());
 *       } catch (const std::exception& e) {
 *           logFatal(e.what());
 *           throw;
 *       }
 *   }
 */
struct SafeBoundary {
    SafeBoundary() noexcept = default;
    ~SafeBoundary() noexcept = default;
    
    // Non-copyable, non-movable (scope marker)
    SafeBoundary(const SafeBoundary&) = delete;
    SafeBoundary& operator=(const SafeBoundary&) = delete;
    SafeBoundary(SafeBoundary&&) = delete;
    SafeBoundary& operator=(SafeBoundary&&) = delete;
};

} // namespace safety

#endif // SAFETY_SAFE_SCOPE_H
