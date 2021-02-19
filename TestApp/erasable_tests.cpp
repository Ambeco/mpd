
#include <iostream>
#include <vector>
#include "../SmallContainers/erasable.hpp"

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
mpd::type_identity<Dog> dog_type;
struct Cat : Animal {
    std::string noise() const { return "meow\n"; }
};
mpd::type_identity<Cat> cat_type;

void test_erasable() {
    std::vector<mpd::erasable<Animal, 56>> animals;
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
