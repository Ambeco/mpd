
#include <cassert>
#include "../SmallContainers/pimpl.hpp"

//public header
struct fooimpl;
struct foo {
    mpd::pimpl<fooimpl, 32> impl;
    foo(int v);
    void set(int v);
    int get() const;
};

//private impl
struct fooimpl {
    int v_;
    fooimpl(int v) : v_{ v } {}
    void set(int v) { v_ = v; }
    int get() const { return v_; }
};
foo::foo(int v) :impl(v) {}
void foo::set(int v) { return impl->set(v); }
int foo::get() const { return impl->get(); }


template<class...Ts>
void test_ctor(int expected_value, Ts&&... vals) {
    mpd::pimpl<fooimpl, 32> v(std::forward<Ts>(vals)...);
    assert(v->get() == expected_value);
}
template<class F>
void _test_mutation(int expected_value, F func) {
    mpd::pimpl<fooimpl, 32> v(1);
    func(v);
    assert(expected_value == v->get());
}
#define test_mutation(expected_value, op) \
_test_mutation(expected_value, [=](mpd::pimpl<fooimpl, 32>& v){op;});

void test_pimpl() {
    const mpd::pimpl<fooimpl, 32> cfoo(2);
    mpd::pimpl<fooimpl, 32> mfoo(3);

    test_ctor(2, cfoo);
    test_ctor(4, fooimpl(4));
    test_ctor(4, 4);
    test_mutation(2, v = cfoo);
    test_mutation(4, v = fooimpl(4));
    assert(cfoo->get() == 2);
    assert(mfoo->get() == 3);
    assert((*cfoo).get() == 2);
    assert((*mfoo).get() == 3);
    assert(cfoo.get()->get() == 2);
    assert(mfoo.get()->get() == 3);

}