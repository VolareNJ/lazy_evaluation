#include <iostream>
#include "lazy_evaluation.hpp"

int six()
{

    std::cout << "cache initialised" << std::endl;
    return 6;
}

int add(int a, int b)
{
    std::cout << "memory initialised" << std::endl;
    return a + b;
}

int main()
{
    std::cout << "-----------------------lazy_val" << std::endl;
    lazy::lazy_val<int (*)()> test1(six);
    std::cout << test1 << std::endl;
    std::cout << test1 << std::endl;
    std::cout << test1 << std::endl;
    std::cout << test1 << std::endl;
    std::cout << test1 << std::endl;
    std::cout << test1 << std::endl;

    std::cout << "-----------------------make_lazy" << std::endl;
    auto test2 = lazy::make_lazy(six);
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;

    std::cout << "-----------------------memorised" << std::endl;
    lazy::memorised<int, int, int> test3(add);
    std::cout << test3(1, 2) << std::endl;
    std::cout << test3(1, 2) << std::endl;
    std::cout << test3(1, 2) << std::endl;
    std::cout << test3(3, 4) << std::endl;
    std::cout << test3(3, 4) << std::endl;
    std::cout << test3(3, 4) << std::endl;

    std::cout << "-----------------------make_memorised" << std::endl;
    auto test4 = lazy::make_memorised(add);
    std::cout << test4(1, 2) << std::endl;
    std::cout << test4(1, 2) << std::endl;
    std::cout << test4(1, 2) << std::endl;
    std::cout << test4(3, 4) << std::endl;
    std::cout << test4(3, 4) << std::endl;
    std::cout << test4(3, 4) << std::endl;
}
