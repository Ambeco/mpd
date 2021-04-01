#pragma once
#include <cassert>
#include <cstddef>
#include <memory>
#if __cplusplus >= 201704L
#include <optional>
#endif
#include <type_traits>
#include <utility>

namespace mpd {
    template<std::size_t left, std::size_t right>
    struct static_assert_less_eq { static_assert(left <= right, "left must be less or equal to right"); };
    template<std::size_t left, std::size_t right, std::size_t mod = left % right>
    struct static_assert_mod_0 { static_assert(mod == 0, "left must be a multiple of right"); };
    template<bool left, bool right>
    struct static_assert_if_then_also { static_assert(!left || right, "if left, then also right"); };

#if __cplusplus >= 201704L
    using type_identity = std::type_identity;
    using in_place_t = std::in_place_t;
#else
    template< class T >
    struct type_identity {
        using type = T;
    };
    template< class T >
    using type_identity_t = typename type_identity<T>::type;
    struct in_place_t {
        explicit in_place_t() = default;
    };
    constexpr in_place_t in_place{};
    template <class T> struct in_place_type_t {
        explicit in_place_type_t() = default;
    };
#endif

    //hybrid std::unique_ptr/std::optional, that it can hold any impl that implements an interface, with no allocations.
    template<class Interface, std::size_t buffer_size, std::size_t align_size = alignof(std::max_align_t), bool noexcept_move = false, bool noexcept_copy = false>
    class erasable {
        using buffer_t = std::aligned_storage_t<buffer_size, align_size>;
        struct erasing {
            virtual void copy_construct(erasing* buffer, Interface*& ptr) const noexcept(noexcept_copy) = 0;
            virtual void move_construct(erasing* buffer, Interface*& ptr) noexcept(noexcept_move) = 0;
            virtual void copy_assign(erasing* buffer, Interface*& ptr) const noexcept(noexcept_copy) = 0;
            virtual void move_assign(erasing* buffer, Interface*& ptr) noexcept(noexcept_move) = 0;
            virtual bool swap(erasing* other) = 0;
            virtual Interface* get() = 0;
            virtual const Interface* get() const = 0;
            virtual ~erasing() {}
        };
        template<class Impl>
        struct erased : erasing {
            Impl val;
            template<class...Ts>
            erased(in_place_t, Ts&&...vs) : val(std::forward<Ts>(vs)...) {}
            void copy_construct(erasing* buffer, Interface*& ptr) const noexcept(noexcept_copy) { 
                assert(buffer);
                assert(!ptr);
                erased* other=new(buffer)erased(*this); 
                ptr=&(other->val);
            }
            void move_construct(erasing* buffer, Interface*& ptr) noexcept(noexcept_move) { 
                assert(buffer); 
                assert(!ptr); 
                erased* other=new(buffer)erased(std::move(*this)); 
                ptr=&(other->val); 
            }
            void copy_assign(erasing* buffer, Interface*& ptr) const noexcept(noexcept_copy) {
                assert(buffer);
                erased* othererased = dynamic_cast<erased*>(buffer);
                if (othererased) {
                    othererased->val = val;
                } else {
                    buffer->~erasing();
                    ptr = nullptr;
                    copy_construct(buffer, ptr);
                }
            }
            void move_assign(erasing* buffer, Interface*& ptr) noexcept(noexcept_move) {
                assert(buffer);
                erased* othererased = dynamic_cast<erased*>(buffer);
                if (othererased) {
                    othererased->val = std::move(val);
                } else {
                    buffer->~erasing();
                    ptr = nullptr;
                    move_construct(buffer, ptr);
                }
            }
            bool swap(erasing* other) {
                assert(other);
                erased* othererased = dynamic_cast<erased*>(other);
                if (othererased) {
                    using std::swap;
                    swap(val, othererased->val);
                    return true;
                }
                return false;
            }
            Interface* get() { return &val; }
            const Interface* get() const { return &val; }
            ~erased() {
                static_assert_less_eq<sizeof(erased), sizeof(buffer_t)>{};
                static_assert_mod_0<align_size, alignof(erased)>{};
                static_assert_if_then_also<noexcept_move, std::is_nothrow_move_constructible_v<Impl>>{};
                static_assert_if_then_also<noexcept_copy, std::is_nothrow_copy_constructible_v<Impl>>{};
            }
        };
        Interface* ptr;
        buffer_t rawbuffer;
#if __cplusplus >= 201704L
        erasing* buffer() { return std::launder(reinterpret_cast<erasing*>(&rawbuffer)); }
        const erasing* buffer() const { return std::launder(reinterpret_cast<const erasing*>(&rawbuffer)); }
#else
        erasing* buffer() { return reinterpret_cast<erasing*>(&rawbuffer); }
        const erasing* buffer() const { return reinterpret_cast<const erasing*>(&rawbuffer); }
#endif
    public:
        constexpr erasable() noexcept :ptr(nullptr), rawbuffer{} {}
#if __cplusplus >= 201704L
        constexpr erasable(std::nullopt_t) noexcept :ptr(nullptr), rawbuffer{} {}
#endif
        constexpr erasable(std::nullptr_t) noexcept :ptr(nullptr), rawbuffer{} {}
        constexpr erasable(const erasable& other) noexcept(noexcept_copy) :ptr(nullptr) { operator=(other); }
        constexpr erasable(erasable&& other) noexcept(noexcept_move) :ptr(nullptr) { operator=(std::move(other)); other.reset(); }
        template<class Impl, class...Ts>
        constexpr erasable(type_identity<Impl> impl, Ts&&...vs) noexcept(std::is_nothrow_constructible_v<Impl, Ts...>) :ptr(nullptr) { emplace(impl, std::forward<Ts>(vs)...); }
        template<class Impl, class allowed = std::enable_if_t<std::is_base_of_v<Interface, Impl>>>
        explicit erasable(const Impl& rhs) noexcept(std::is_nothrow_constructible_v<Impl, const Impl&>) :ptr(nullptr) { emplace(rhs); }
        template<class Impl, class allowed = std::enable_if_t<std::is_base_of_v<Interface, Impl>>>
        explicit erasable(Impl&& rhs) noexcept(std::is_nothrow_constructible_v<Impl, Impl&&>) :ptr(nullptr) { emplace(std::move(rhs)); }
        constexpr erasable& operator=(const erasable& other) noexcept(noexcept_copy) {
            if (other.ptr && ptr) other.buffer()->copy_assign(buffer(), ptr);
            else if (other.ptr) other.buffer()->copy_construct(buffer(), ptr);
            else reset();
            return *this;
        }
        constexpr erasable& operator=(erasable&& other) noexcept(noexcept_move) {
            if (other.ptr && ptr) other.buffer()->move_assign(buffer(), ptr);
            else if (other.ptr) other.buffer()->move_construct(buffer(), ptr);
            else reset();
            return *this;
        }
        template<class Impl>
        constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, erasable&> operator=(const Impl& rhs) {
            Impl* self;
            if (ptr && (self=dynamic_cast<Impl*>(ptr))) *self = rhs;
            else emplace(rhs);
            return *this;
        }
        template<class Impl>
        constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, erasable&> operator=(Impl&& rhs) {
            Impl* self;
            if (ptr && (self=dynamic_cast<Impl*>(ptr))) *self = std::move(rhs);
            else emplace(std::move(rhs));
            return *this;
        }
#if __cplusplus >= 201704L
        constexpr erasable& operator=(std::nullopt_t) { reset(); return *this; }
#endif
        constexpr erasable& operator=(std::nullptr_t) { reset(); return *this; }
        ~erasable() { reset(); }
        constexpr Interface* operator->() noexcept { return ptr; }
        constexpr const Interface* operator->() const noexcept { return ptr; }
        constexpr Interface& operator*() noexcept { assert(ptr); return *ptr; }
        constexpr const Interface& operator*() const noexcept { assert(ptr); return *ptr; }
        constexpr explicit operator bool() const noexcept { return ptr != nullptr; }
        constexpr Interface* get() noexcept { return ptr ? ptr : nullptr; }
        constexpr const Interface* get() const noexcept { return ptr ? ptr : nullptr; }
        constexpr bool has_value() const noexcept { return ptr != nullptr; }
        constexpr Interface& value() noexcept { assert(ptr); return *ptr; }
        constexpr const Interface& value() const noexcept { assert(ptr); return *ptr; }
        constexpr Interface& value_or(Interface& impl) noexcept {return ptr ? *ptr : impl;}
        constexpr const Interface& value_or(const Interface& impl) const noexcept { return ptr ? *ptr : impl; }
        constexpr void swap(erasable& other) {
            if (ptr && other.ptr && buffer()->swap(other.buffer())) return;
            else if (other.ptr && ptr) std::swap(*this, other);
            else if (other.ptr) { operator=(other); other.reset(); }
            else {other = *this; reset();}
        }
        constexpr void reset() { if (ptr) { buffer()->~erasing(); ptr = nullptr; } }
        template<class Impl, class...Ts>
        constexpr void emplace(type_identity<Impl>, Ts&&...vs) noexcept(std::is_nothrow_constructible_v<Impl, Ts...>) { 
            reset(); 
            erased<Impl>* o = new(buffer())erased<Impl>(in_place, std::forward<Ts>(vs)...); 
            ptr = o->get();
        }
        template<class Impl, class...Ts>
        constexpr void emplace(Ts&&...vs) noexcept(std::is_nothrow_constructible_v<Impl, Ts...>) {
            reset();
            erased<Impl>* o = new(buffer())erased<Impl>(in_place, std::forward<Ts>(vs)...); 
            ptr = o->get();
        }
        template<class Impl>
        constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, void> emplace(const Impl& rhs) { emplace(type_identity<Impl>{}, rhs); }
        template<class Impl>
        constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, void> emplace(Impl&& rhs) { emplace(type_identity<Impl>{}, std::move(rhs)); }
    };
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    std::ostream& operator<<(std::ostream& stream, const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& val) { return val ? stream << *val : stream << "null"; }

    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator==(const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& lhs, 
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& rhs) {
        return lhs.has_value()==rhs.has_value() && (!lhs.has_value() || lhs.value()==rhs.value());
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator!=(const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& lhs,
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& rhs) {
        return lhs.has_value() != rhs.has_value() || (lhs.has_value() && lhs.value() != rhs.value());
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator==(std::nullptr_t impl, const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased) {
        return !erased.has_value();
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator==(
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased, const std::nullptr_t& impl) {
        return !erased.has_value();
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator!=(std::nullptr_t impl, const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased) {
        return erased.has_value();
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator!=(
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased, const std::nullptr_t& impl) {
        return erased.has_value();
    }

#if __cplusplus >= 201704L
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator==(std::nullopt_t impl, const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased) {
        return !erased.has_value();
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator==(
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased, const std::nullopt_t& impl) {
        return !erased.has_value();
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator!=(std::nullopt_t impl, const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased) {
        return erased.has_value();
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    constexpr bool operator!=(
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased, const std::nullopt_t& impl) {
        return erased.has_value();
    }
#endif

    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy, class Impl>
    constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, bool> operator==(const Impl& impl, 
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased) {
        return erased.has_value() && impl == erased.value();
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy, class Impl>
    constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, bool> operator==(
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased, const Impl& impl) {
        return erased.has_value() && erased.value() == impl;
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy, class Impl>
    constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, bool> operator!=(const Impl& impl,
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased) {
        return !erased.has_value() || impl != erased.value();
    }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy, class Impl>
    constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, bool> operator!=(
        const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& erased, const Impl& impl) {
        return !erased.has_value() || erased.value() != impl;
    }
}
namespace std {
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    void swap(mpd::erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& lhs, mpd::erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& rhs) { lhs.swap(rhs); }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    struct hash<mpd::erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>> {
        std::size_t operator()(const mpd::erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& val) noexcept { return std::hash<Interface>{}(*val); }
    };
}
