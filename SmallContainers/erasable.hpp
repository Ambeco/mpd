#pragma once
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

template<std::size_t left, std::size_t right>
struct static_assert_less_eq { static_assert(left <= right); };
template<std::size_t left, std::size_t right, std::size_t mod = left % right>
struct static_assert_mod_0 { static_assert(mod == 0); };
template<bool left, bool right>
struct static_assert_if_then_also { static_assert(!left || right); };

//hybrid std::unique_ptr/std::optional, that it can hold any impl that implements an interface, with no allocations.
template<class Interface, std::size_t buffer_size, std::size_t align_size = alignof(std::max_align_t), bool noexcept_move = false, bool noexcept_copy = false>
class erasable {
    using buffer_t = std::aligned_storage_t<buffer_size, align_size>;
    struct erasing {
        virtual erasing* copy_construct(erasing* buffer) const noexcept(noexcept_copy) = 0;
        virtual erasing* move_construct(erasing* buffer) noexcept(noexcept_move) = 0;
        virtual void copy_assign(erasing*& buffer) const noexcept(noexcept_copy) = 0;
        virtual void move_assign(erasing*& buffer) noexcept(noexcept_move) = 0;
        virtual bool swap(erasing* other) = 0;
        virtual Interface* get() = 0;
        virtual const Interface* get() const = 0;
        virtual ~erasing() {}
    };
    template<class Impl>
    struct erased : erasing {
        Impl val;
        template<class...Ts>
        erased(std::in_place_t, Ts&&...vs) : val(std::forward<Ts>(vs)...) {}
        erasing* copy_construct(erasing* buffer) const noexcept(noexcept_copy) { assert(buffer); return new(buffer)erased(*this); }
        erasing* move_construct(erasing* buffer) noexcept(noexcept_move) { assert(buffer); return new(buffer)erased(std::move(*this)); }
        void copy_assign(erasing*& other) const noexcept(noexcept_copy) {
            assert(other);
            erased* othererased = dynamic_cast<erased*>(other);
            if (othererased) {
                othererased->val = val;
            }
            else {
                other->~erasing();
                erasing* tempptr = other;
                other = nullptr;
                other = copy_construct(tempptr);
            }
        }
        void move_assign(erasing*& other) noexcept(noexcept_move) {
            assert(other);
            erased* othererased = dynamic_cast<erased*>(other);
            if (othererased) {
                othererased->val = std::move(val);
            }
            else {
                other->~erasing();
                erasing* tempptr = other;
                other = nullptr;
                other = move_construct(tempptr);
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
    erasing* ptr;
    buffer_t rawbuffer;
    erasing* buffer() { return reinterpret_cast<erasing*>(&rawbuffer); }
public:
    constexpr erasable() noexcept :ptr(nullptr) {}
    constexpr erasable(std::nullopt_t) noexcept :ptr(nullptr) {}
    constexpr erasable(std::nullptr_t) noexcept :ptr(nullptr) {}
    constexpr erasable(const erasable& other) noexcept(noexcept_copy) :ptr(nullptr) { operator=(other); }
    constexpr erasable(erasable&& other) noexcept(noexcept_move) :ptr(nullptr) { operator=(std::move(other)); other.reset(); }
    template<class Impl, class...Ts>
    constexpr erasable(std::type_identity<Impl> impl, Ts&&...vs) noexcept(std::is_nothrow_constructible_v<Impl, Ts...>) :ptr(nullptr) { emplace(impl, std::forward<Ts>(vs)...); }
    template<class Impl, class allowed = std::enable_if_t<std::is_base_of_v<Interface, Impl>>>
    explicit erasable(const Impl& rhs) noexcept(std::is_nothrow_constructible_v<Impl, const Impl&>) :ptr(nullptr) { emplace(rhs); }
    template<class Impl, class allowed = std::enable_if_t<std::is_base_of_v<Interface, Impl>>>
    explicit erasable(Impl&& rhs) noexcept(std::is_nothrow_constructible_v<Impl, Impl&&>) :ptr(nullptr) { emplace(std::move(rhs)); }
    constexpr erasable& operator=(const erasable& other) noexcept(noexcept_copy) {
        if (other.ptr && ptr) other.ptr->copy_assign(ptr);
        else if (other.ptr) ptr = other.ptr->copy_construct(buffer());
        else reset();
        return *this;
    }
    constexpr erasable& operator=(erasable&& other) noexcept(noexcept_move) {
        if (other.ptr && ptr) other.ptr->move_assign(ptr);
        else if (other.ptr) ptr = other.ptr->move_construct(buffer());
        else reset();
        return *this;
    }
    template<class Impl>
    constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, erasable&> operator=(const Impl& rhs) {
        if (ptr && dynamic_cast<Impl*>(ptr->get())) *dynamic_cast<Impl*>(ptr->get()) = rhs;
        else emplace(rhs);
        return *this;
    }
    template<class Impl>
    constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, erasable&> operator=(Impl&& rhs) {
        if (ptr && dynamic_cast<Impl*>(ptr->get())) *dynamic_cast<Impl*>(ptr->get()) = std::move(rhs);
        else emplace(std::move(rhs));
        return *this;
    }
    constexpr erasable& operator=(std::nullopt_t) { reset(); }
    constexpr erasable& operator=(std::nullptr_t) { reset(); }
    constexpr ~erasable() { reset(); }
    constexpr Interface* operator->() noexcept { return ptr ? ptr->get() : nullptr; }
    constexpr const Interface* operator->() const noexcept { return ptr ? ptr->get() : nullptr; }
    constexpr Interface& operator*() noexcept { assert(ptr); return *(ptr->get()); }
    constexpr const Interface& operator*() const noexcept { assert(ptr); return *(ptr->get()); }
    constexpr explicit operator bool() const noexcept { return ptr != nullptr; }
    constexpr Interface* get() noexcept { return ptr ? ptr->get() : nullptr; }
    constexpr const Interface* get() const noexcept { return ptr ? ptr->get() : nullptr; }
    constexpr bool has_value() noexcept { return ptr != nullptr; }
    constexpr Interface& value() noexcept { assert(ptr); return *(ptr->get()); }
    constexpr const Interface& value() const noexcept { assert(ptr); return *(ptr->get()); }
    template<class Impl>
    constexpr Interface& value_or(Impl& impl) noexcept { return ptr ? *(ptr->get()) : impl; }
    template<class Impl>
    constexpr const Interface& value_or(Impl& impl) const noexcept { return ptr ? *(ptr->get()) : impl; }
    constexpr void swap(erasable& other) {
        if (ptr && ptr->swap(other.ptr)) return;
        else if (other.ptr && ptr) std::swap(*this, other);
        else if (other.ptr) operator=(other);
        else other = *this;
    }
    constexpr void reset() { if (ptr) { std::destroy_at(ptr); ptr = nullptr; } }
    template<class Impl, class...Ts>
    constexpr void emplace(std::type_identity<Impl>, Ts&&...vs) noexcept(std::is_nothrow_constructible_v<Impl, Ts...>) { reset(); ptr = new(buffer())erased<Impl>(std::in_place, std::forward<Ts>(vs)...); }
    template<class Impl, class...Ts>
    constexpr void emplace(Ts&&...vs) noexcept(std::is_nothrow_constructible_v<Impl, Ts...>) { reset(); ptr = new(buffer())erased<Impl>(std::in_place, std::forward<Ts>(vs)...); }
    template<class Impl>
    constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, void> emplace(const Impl& rhs) { emplace(std::type_identity<Impl>{}, rhs); }
    template<class Impl>
    constexpr std::enable_if_t<std::is_base_of_v<Interface, Impl>, void> emplace(Impl&& rhs) { emplace(std::type_identity<Impl>{}, std::move(rhs)); }
};
template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
std::ostream& operator<<(std::ostream& stream, const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& val) { return val ? stream << *val : stream << "null"; }
namespace std {
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    void swap(erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& lhs, erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& rhs) { lhs.swap(rhs); }
    template<class Interface, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    struct hash<erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>> {
        std::size_t operator()(const erasable<Interface, buffer_size, align_size, noexcept_move, noexcept_copy>& val) noexcept { return std::hash<Interface>{}(*val); }
    };
}






/*
#include <iostream>
#include <vector>
struct Animal {
    virtual std::string noise() const = 0;
    virtual ~Animal() {};
};
std::ostream& operator<<(std::ostream& stream, const Animal& animal) { return stream << animal.noise(); }
struct Dog : Animal {
    std::string name;
    Dog(std::string name_) : name(std::move(name_)) {}
    std::string noise() const { return "bark\n"; }
};
std::type_identity<Dog> dog_type;
struct Cat : Animal {
    std::string noise() const { return "meow\n"; }
};
std::type_identity<Cat> cat_type;

int main() {
    std::vector<erasable<Animal, 48>> animals;
    animals.emplace_back(dog_type, "Fido");
    animals.emplace_back(cat_type);
    std::cout << animals[0]; //bark
    std::cout << animals[1]; //meow

    std::swap(animals[0], animals[1]);
    std::cout << animals[0]; //meow
    std::cout << animals[1]; //bark

    Dog otherdog("Spot");
    animals[0] = otherdog;
    animals[1] = otherdog;
    std::cout << animals[0]; //bark
    std::cout << animals[1]; //bark

}
*/
