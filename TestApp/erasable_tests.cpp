
#include <iostream>
#include <vector>
#include <sstream>
#include "../SmallContainers/erasable.hpp"

struct Animal {
    virtual std::string noise() const = 0;
    virtual ~Animal() {};
    virtual bool operator==(const Animal& rhs) const = 0;
};
std::ostream& operator<<(std::ostream& stream, const Animal& animal) { return stream << animal.noise(); }
struct Dog : Animal {
    std::string name;
    Dog(std::string name_) : name(std::move(name_)) {}
    std::string noise() const { return "bark"; }
    bool operator==(const Animal& rhs) const { return dynamic_cast<const Dog*>(&rhs); }
};
mpd::type_identity<Dog> dog_type;
struct Cat : Animal {
    std::string noise() const { return "meow"; }
    bool operator==(const Animal& rhs) const { return dynamic_cast<const Cat*>(&rhs); }
};
mpd::type_identity<Cat> cat_type;
using eanimal = mpd::erasable<Animal, 56>;

template<class expectedT, class...Ts>
void test_ctor(const expectedT& expected_value, Ts&&... vals) {
    eanimal animal(std::forward<Ts>(vals)...);
    assert(expected_value == animal);
}
template<class expectedT, class F>
void _test_empty_mutation(const expectedT& expected_value, F func) {
    eanimal animal;
    func(animal);
    assert(expected_value == animal);
}
#define test_empty_mutation(expected_value, op) \
_test_empty_mutation(expected_value, [=](eanimal& v){op;});

template<class expectedT, class F>
void _test_dog_mutation(const expectedT& expected_value, F func) {
    eanimal animal(dog_type, "fido");
    func(animal);
    assert(expected_value == animal);
}
#define test_dog_mutation(expected_value, op) \
_test_dog_mutation(expected_value, [=](eanimal& v){op;});
#define test_both_mutation(expected_value, op) \
_test_empty_mutation(expected_value, [=](eanimal& v){op;}); \
_test_dog_mutation(expected_value, [=](eanimal& v){op;});

void test_erasable() {
    std::stringstream ss;
    const Dog cdog{ "fido" };
    const Cat ccat{};
    Dog mdog{ "fido" };
    Cat mcat{};
    eanimal emempty{ };
    eanimal emdog{ cdog };
    eanimal emcat{ ccat };
    const eanimal ecempty{ };
    const eanimal ecdog{ cdog };
    const eanimal eccat{ ccat };

    test_ctor(ecempty);
#if __cplusplus >= 201704L
    test_ctor(ecempty, std::nullopt);
#endif
    test_ctor(ecempty, nullptr);
    test_ctor(ecempty, ecempty);
    test_ctor(eccat, eccat);
    test_ctor(ecempty, eanimal{});
    test_ctor(eccat, eanimal{ Cat{} });
    test_ctor(ecdog, dog_type, "fido");
    test_ctor(eccat, cat_type);
    test_ctor(eccat, cat_type, Cat{});
    test_ctor(eccat, cat_type, ccat);
    test_ctor(ecdog, Dog{ "fido" });
    test_ctor(eccat, Cat{});
    test_ctor(eccat, ccat);
    test_both_mutation(ecempty, v = ecempty);
    test_both_mutation(eccat, v = eccat);
    test_both_mutation(ecempty, v = eanimal{});
    test_both_mutation(eccat, v = eanimal{ Cat{} });
    test_both_mutation(eccat, v = eccat);
    test_both_mutation(eccat, v = Cat{});
#if __cplusplus >= 201704L
    test_both_mutation(ecempty, v = std::nullopt);
#endif
    test_both_mutation(ecempty, v = nullptr);
    assert(emcat->noise() == "meow");
    assert(eccat->noise() == "meow");
    assert((*emcat).noise() == "meow");
    assert((*eccat).noise() == "meow");
    assert((bool)ecempty == false);
    assert((bool)eccat == true);
    assert(eanimal{}.get() == nullptr);
    assert(emcat.get()->noise() == "meow");
    assert(ecempty.get() == nullptr);
    assert(eccat.get()->noise() == "meow");
    assert(ecempty.has_value() == false);
    assert(eccat.has_value() == true);
    assert(emcat.value().noise() == "meow");
    assert(eccat.value().noise() == "meow");
    assert(emempty.value_or(mcat).noise() == "meow");
    assert(emcat.value_or(mcat).noise() == "meow");
    assert(ecempty.value_or(ccat).noise() == "meow");
    assert(eccat.value_or(ccat).noise() == "meow");
    test_both_mutation(ecempty, eanimal o{ }; v.swap(o));
    test_both_mutation(eccat, eanimal o{ Cat{} }; v.swap(o));
    test_empty_mutation(ecempty, eanimal o{ }; o.swap(v));
    test_empty_mutation(eccat, eanimal o{ Cat{} }; o.swap(v));
    test_dog_mutation(ecempty, eanimal o{ }; o.swap(v));
    test_dog_mutation(eccat, eanimal o{ Cat{} }; o.swap(v));
    test_both_mutation(ecempty, v.reset());
    test_both_mutation(cdog, v.emplace(dog_type, "fido"));
    test_both_mutation(ccat, v.emplace(cat_type));
    test_both_mutation(ccat, v.emplace(cat_type, Cat{}));
    test_both_mutation(ccat, v.emplace(cat_type, ccat));
    test_both_mutation(ccat, v.emplace(Cat{}));
    test_both_mutation(ccat, v.emplace(ccat));
    ss << ccat;
    assert(ss.str() == "meow");
}
