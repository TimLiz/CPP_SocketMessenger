#ifndef MYMESSENGER_RAII_H
#define MYMESSENGER_RAII_H
#include "boost/noncopyable.hpp"
#include "type_traits"

/**
 * Wraps object into RAII-compliant container with custom destructor.
 * Custom destructor MUST be nullptr safe, otherwise, refer to RAII_GEN_RESOURCE_WRAPPER_EXT
 *
 * @param WrappedTypeName The RAII type to define
 * @param Type The object to wrap
 * @param DestructorFn Destructor function
 */
#define RAII_GEN_RESOURCE_WRAPPER(WrappedTypeName, Type, DestructorFn)                                                 \
    RAII_GEN_RESOURCE_WRAPPER_EXT(WrappedTypeName, Type, DestructorFn, false);

/**
 * Wraps object into RAII-compliant container with custom destructor.
 *
 * @param WrappedTypeName The RAII type to define
 * @param Type The object to wrap
 * @param DestructorFn Destructor function
 * @param nullGuard Should macro implement null guard?
 */
#define RAII_GEN_RESOURCE_WRAPPER_EXT(WrappedTypeName, Type, DestructorFn, nullGuard)                                  \
    static_assert(std::is_invocable_v<decltype(DestructorFn), Type*>, "Invalid destructor for RAII wrapper gen");      \
    struct WrappedTypeName : public boost::noncopyable {                                                               \
        private:                                                                                                       \
            Type* r = nullptr;                                                                                         \
                                                                                                                       \
            void freeResource() {                                                                                      \
                if (nullGuard and !r) {                                                                                \
                    return;                                                                                            \
                }                                                                                                      \
                DestructorFn(r);                                                                                       \
            };                                                                                                         \
                                                                                                                       \
        public:                                                                                                        \
            WrappedTypeName(Type* r) : r(r) {};                                                                        \
            WrappedTypeName(WrappedTypeName&& oth) noexcept : r(oth.r) { oth.r = nullptr; };                           \
            WrappedTypeName& operator=(WrappedTypeName&& oth) noexcept {                                               \
                freeResource();                                                                                        \
                r = oth.r;                                                                                             \
                oth.r = nullptr;                                                                                       \
                return *this;                                                                                          \
            };                                                                                                         \
            ~WrappedTypeName() noexcept { freeResource(); }                                                            \
            operator Type*() const { return r; };                                                                      \
    }
#endif // MYMESSENGER_RAII_H
