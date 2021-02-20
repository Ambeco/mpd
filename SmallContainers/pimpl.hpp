#include <memory>

namespace mpd {
    //allows you to have Impl as a member without the definition available in the header
    template<class Impl, std::size_t buffer_size, std::size_t align_size = alignof(std::max_align_t), bool noexcept_move = false, bool noexcept_copy = true>
    class pimpl {
        std::aligned_storage_t<buffer_size, align_size> rawbuffer;
        Impl* ptr;
        Impl* buffer() { return reinterpret_cast<Impl*>(&rawbuffer); }
    public:
        constexpr pimpl(const pimpl& other) noexcept(noexcept_copy) :ptr(new(buffer())Impl(*other)) {}
        constexpr pimpl(pimpl&& other) noexcept(noexcept_move) :ptr(new(buffer())Impl(std::move(*other))) {}
        template<class...Ts>
        constexpr pimpl(Ts&&...vs) noexcept(std::is_nothrow_constructible_v<Impl, Ts...>) :ptr(new(buffer())Impl(std::forward<Ts>(vs)...)) {}
        constexpr pimpl& operator=(const pimpl& other) noexcept(noexcept_copy) { *ptr = *other; }
        constexpr pimpl& operator=(pimpl&& other) noexcept(noexcept_move) { *ptr = std::move(*other); }
        ~pimpl() { ptr->~Impl(); }
        constexpr Impl* operator->() noexcept { return ptr; }
        constexpr const Impl* operator->() const noexcept { return ptr; }
        constexpr Impl& operator*() noexcept { return *ptr; }
        constexpr const Impl& operator*() const noexcept { return *ptr; }
        constexpr Impl* get() noexcept { return ptr; }
        constexpr const Impl* get() const noexcept { return ptr; }
    };
}
namespace std {
    template<class Impl, std::size_t buffer_size, std::size_t align_size = alignof(std::max_align_t), bool noexcept_move = false, bool noexcept_copy = true>
    void swap(mpd::pimpl<Impl, buffer_size, align_size, noexcept_move, noexcept_copy>& lhs, mpd::pimpl<Impl, buffer_size, align_size, noexcept_move, noexcept_copy>& rhs) { using std::swap; std::swap(*lhs, *rhs); }
    template<class Impl, std::size_t buffer_size, std::size_t align_size, bool noexcept_move, bool noexcept_copy>
    struct hash<mpd::pimpl<Impl, buffer_size, align_size, noexcept_move, noexcept_copy>> {
        std::size_t operator()(mpd::pimpl<Impl, buffer_size, align_size, noexcept_move, noexcept_copy>& val) { return std::hash<Impl>{}(*val); }
    };
}
