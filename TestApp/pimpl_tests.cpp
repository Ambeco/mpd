
#include "../SmallContainers/pimpl.hpp"

//public header
struct fooimpl;
struct foo {
    mpd::pimpl<fooimpl, 32> impl;
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
struct fooimpl : bar {
    fooimpl(int c, const char* d) : bar{ c, d } {}
};
foo::foo() :impl(3, "hi") {}

void public_api_function(foo& f) {
    private_third_party_function(*f.impl);
}

void test_pimpl() {
    foo userFoo;
    public_api_function(userFoo);

}