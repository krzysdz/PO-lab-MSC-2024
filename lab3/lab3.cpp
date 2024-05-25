#include "generators.hpp"
#include "test_utils.hpp"
#include <format>

class GeneratorTests {
    static auto get_base() { return std::make_shared<GeneratorBaza>(); }
    static void test_base()
    {
        std::cerr << "Base generator: ";
        it_should_not_throw([]() {
            for (double v : { 0.0, 1.5, 13.2, -7.3 }) {
                GeneratorBaza b{ v };
                for (int t : { 1, 15, 20, 123456 }) {
                    auto r = b.symuluj(t);
                    if (r != v)
                        throw std::logic_error{ std::format(
                            "Generator({0}).symuluj({1}) returned {2} instead of {0}", v, t, r) };
                }
            }
        });
    }

public:
    static void run_all() { test_base(); }
};

int main()
{
    GeneratorTests::run_all();
    return 0;
}
