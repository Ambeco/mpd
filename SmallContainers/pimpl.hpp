#include <memory>

namespace mpd {
    //allows you to have Impl as a member without the definition available in the header
    template<class Impl, std::size_t buffer_size, std::size_t align_size = alignof(std::max_align_t), bool noexcept_move = false, bool noexcept_copy = true>
    class pimpl {
        std::aligned_storage_t<buffer_size, align_size> rawbuffer;
#if __cplusplus >= 201704L
        Impl* buffer() { return std::launder(reinterpret_cast<Impl*>(&rawbuffer)); }
        const Impl* buffer() const { return std::launder(reinterpret_cast<const Impl*>(&rawbuffer)); }
#else
        Impl* buffer() { return reinterpret_cast<Impl*>(&rawbuffer); }
        const Impl* buffer() const { return reinterpret_cast<const Impl*>(&rawbuffer); }
#endif
    public:
        constexpr pimpl(const pimpl& other) noexcept(noexcept_copy) { new(buffer())Impl(*other); }
        constexpr pimpl(pimpl&& other) noexcept(noexcept_move) { new(buffer())Impl(std::move(*other)); }
        template<class...Ts>
        constexpr pimpl(Ts&&...vs) noexcept(std::is_nothrow_constructible_v<Impl, Ts...>) { new(buffer())Impl(std::forward<Ts>(vs)...); }
        constexpr pimpl& operator=(const pimpl& other) noexcept(noexcept_copy) { *buffer() = *other; return *this; }
        constexpr pimpl& operator=(pimpl&& other) noexcept(noexcept_move) { *buffer() = std::move(*other); return *this;}
        ~pimpl() { buffer()->~Impl(); static_assert(sizeof(Impl)<sizeof(rawbuffer), "buffer_size is too small"); }
        constexpr Impl* operator->() noexcept { return buffer(); }
        constexpr const Impl* operator->() const noexcept { return buffer(); }
        constexpr Impl& operator*() noexcept { return *buffer(); }
        constexpr const Impl& operator*() const noexcept { return *buffer(); }
        constexpr Impl* get() noexcept { return buffer(); }
        constexpr const Impl* get() const noexcept { return buffer(); }
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
