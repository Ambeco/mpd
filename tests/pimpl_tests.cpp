
#include <cassert>
#include "utilities/pimpl.hpp"

//public header
struct fooimpl;
struct foo {
    mpd::pimpl<fooimpl, 32> impl;
    foo(int v);
    foo(const foo& rhs);
    foo(foo&& rhs);
    foo& operator=(const foo& rhs);
    foo& operator=(foo&& rhs);
    void set(int v);
    int get() const;
};


template<class...Ts>
void test_ctor(int expected_value, Ts&&... vals) {
    foo v(std::forward<Ts>(vals)...);
    assert(v.get() == expected_value);
}
template<class F>
void _test_mutation(int expected_value, F func) {
    foo v(1);
    func(v);
    assert(expected_value == v.get());
}
#define test_mutation(expected_value, op) \
_test_mutation(expected_value, [=](foo& v){op;});

void test_pimpl() {
    const foo cfoo(2);
    foo mfoo(3);

    test_ctor(2, cfoo);
    test_ctor(4, foo(4));
    test_ctor(4, 4);
    test_mutation(2, v = cfoo);
    test_mutation(4, v = foo(4));
    assert(cfoo.get() == 2);
    assert(mfoo.get() == 3);
}

//private impl
struct fooimpl {
    int v_;
    fooimpl(int v) : v_{v} {}
    void set(int v) { v_ = v; }
    int get() const { return v_; }
};
foo::foo(int v) :impl(v) {}
foo::foo(const foo& rhs) : impl(rhs.impl) {}
foo::foo(foo&& rhs) : impl(std::move(rhs.impl)) {}
foo& foo::operator=(const foo& rhs) { impl = rhs.impl; return *this; }
foo& foo::operator=(foo&& rhs) { impl = std::move(rhs.impl); return *this; }
void foo::set(int v) { return impl->set(v); }
int foo::get() const { return impl->get(); }

void test_pimpl2() {
    const foo cfoo(2);
    foo mfoo(3);

    assert(cfoo.impl->get() == 2);
    assert(mfoo.impl->get() == 3);
    assert((*cfoo.impl).get() == 2);
    assert((*mfoo.impl).get() == 3);
}