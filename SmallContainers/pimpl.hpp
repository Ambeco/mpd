#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

//allows you to have Impl as a member without the definition available in the header
template<class Impl, std::size_t buffer_size, std::size_t align_size = alignof(std::max_align_t), bool noexcept_move = false, bool noexcept_copy = true>
class pimpl {
    std::aligned_storage_t<buffer_size, align_size> rawbuffer;
    Impl* ptr;
    Impl* buffer() { return reinterpret_cast<Impl*>(&rawbuffer); }
public:
    template<class...Ts>
    constexpr pimpl(Ts&&...vs) noexcept(std::is_nothrow_constructible_v<Impl, Ts...>) :ptr(new(buffer())Impl(std::forward<Ts>(vs)...)) {}
    constexpr pimpl& operator=(const pimpl& other) noexcept(noexcept_copy) { *ptr = *(other.ptr); }
    constexpr pimpl& operator=(pimpl&& other) noexcept(noexcept_move) { *ptr = std::move(*(other.ptr)); }
    constexpr ~pimpl() { std::destroy_at(ptr); }
    constexpr Impl* operator->() noexcept { return ptr; }
    constexpr const Impl* operator->() const noexcept { return ptr; }
    constexpr Impl& operator*() noexcept { return *ptr; }
    constexpr const Impl& operator*() const noexcept { return *ptr; }
    constexpr Impl* get() noexcept { return ptr; }
    constexpr const Impl* get() const noexcept { return ptr; }
};
namespace std {
    template<class Impl, std::size_t buffer_size, std::size_t align_size = alignof(std::max_align_t), bool noexcept_move = false, bool noexcept_copy = true>
    void swap(pimpl<Impl, buffer_size, align_size, noexcept_move, noexcept_copy>& lhs, pimpl<Impl, buffer_size, align_size, noexcept_move, noexcept_copy>& rhs) { using std::swap; std::swap(*lhs, *rhs); }
    template<class Impl, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    struct hash<pimpl<Impl, buffer_size, align_size, noexcept_move, noexcept_copy>> {
        std::size_t operator()(pimpl<Impl, buffer_size, align_size, noexcept_move, noexcept_copy>& val) { return std::hash<Impl>{}(*val); }
    };
}







/*
//public header
struct fooimpl;
struct foo {
    pimpl<fooimpl, 32> impl;
    foo();
};
void public_api_function(foo& f);

//3p header
#include <iostream>
struct bar {
    int c;
    const char* d;
};
void private_third_party_function(bar& f) { std::cout << "success" << f.c << f.d; }

//private impl
struct fooimpl : bar {};
foo::foo() :impl(bar{ 3, "hi" }) {}

void public_api_function(foo& f) {
    private_third_party_function(*f.impl);
}

int main() {
    foo userFoo;
    public_api_function(userFoo);
}
*/
