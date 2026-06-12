#include <iostream>
#include "lazy_evaluation.hpp"

// 普通计算函数（无递归，不涉及自引用）
int six()
{
    std::cout << "cache initialised" << std::endl;
    return 6;
}

// 普通二元函数，用于非递归记忆化测试
int add(int a, int b)
{
    std::cout << "memory initialised" << std::endl;
    return a + b;
}

// -----------------------------------------------------------
// 递归函数用于记忆化：必须使用泛型 lambda
// -----------------------------------------------------------
// 记忆化包装器要求被包装的函数能够接受“包装器自身”作为第一个参数，
// 以便递归时走缓存。泛型 lambda 可以通过 auto& 参数自动推导包装器类型。
auto fib_obj = [](auto& self, unsigned int n) -> unsigned int
{
    std::cout << "fib_obj n = " << n << " calculated" << std::endl;
    if (n == 0) return 0;
    if (n == 1) return 1;
    // self 是记忆化包装器，调用 self 会进入缓存机制
    return self(n - 1) + self(n - 2);
};

// -----------------------------------------------------------
// 尝试用普通函数实现递归记忆化 —— 会失败！
// -----------------------------------------------------------
// fib_ptr 看起来和上面的 lambda 几乎一样，但它是“函数模板”
// （C++20 起，带 auto 参数的函数声明等价于函数模板）。
//
// 为什么它不能用于 make_memorised？
// 1. 函数模板不是对象，不能作为参数传递：
//    make_memorised(fib_ptr) 会因无法推导模板参数而失败。
// 2. 即使通过 fib_ptr<...> 显式实例化，也要指定第一个参数 self 的具体类型。
//    而这个类型是 memorised<...> 的具体实例，依赖于包装的函数对象自身 ——
//    这就成了“先有鸡还是先有蛋”的死循环。
// 3. 就算硬编码类型进行实例化，函数签名也会绑死特定的包装器类型，
//    失去通用性，且代码会极其丑陋和脆弱。
//
// 因此，递归记忆化场景下，必须使用泛型 lambda（或手写的函数对象类），
// 而不能用普通函数或函数模板。
unsigned int fib_ptr(auto& self, unsigned int n)
{
    std::cout << "fib_ptr n = " << n << " calculated" << std::endl;
    if (n == 0) return 0;
    if (n == 1) return 1;
    return self(n - 1) + self(n - 2);
}


int main()
{
    // ==============================================
    // 1. 惰性求值测试（lazy_val）
    // ==============================================
    std::cout << "-----------------------lazy_val" << std::endl;
    lazy::lazy_val<int (*)()> test1(six);
    std::cout << test1 << std::endl;
    std::cout << test1 << std::endl;
    std::cout << test1 << std::endl;  // 重复访问不会重新计算
    std::cout << test1 << std::endl;
    std::cout << test1 << std::endl;
    std::cout << test1 << std::endl;

    std::cout << "-----------------------make_lazy" << std::endl;
    auto test2 = lazy::make_lazy(six);   // 工厂函数，自动推导类型
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;
    std::cout << test2 << std::endl;

    // ==============================================
    // 2. 非递归记忆化测试（普通函数 add）
    // ==============================================
    std::cout << "-----------------------memorised" << std::endl;
    // 手动构造（需指定签名和函数指针类型）
    lazy::memorised<int(int, int), int (*)(int, int)> test3(add, {});
    std::cout << test3(1, 2) << std::endl;
    std::cout << test3(1, 2) << std::endl;  // 命中缓存，不重复计算
    std::cout << test3(1, 2) << std::endl;
    std::cout << test3(3, 4) << std::endl;
    std::cout << test3(3, 4) << std::endl;
    std::cout << test3(3, 4) << std::endl;

    std::cout << "-----------------------make_memorised" << std::endl;
    // 用工厂函数更简洁（只需指定函数签名）
    auto test4 = lazy::make_memorised<int(int, int)>(add);
    std::cout << test4(1, 2) << std::endl;
    std::cout << test4(1, 2) << std::endl;
    std::cout << test4(1, 2) << std::endl;
    std::cout << test4(3, 4) << std::endl;
    std::cout << test4(3, 4) << std::endl;
    std::cout << test4(3, 4) << std::endl;

    // ==============================================
    // 3. 递归记忆化测试（必须用泛型 lambda）
    // ==============================================
    std::cout << "-----------------------make_memorised_fib_obj" << std::endl;
    // fib_obj 是泛型 lambda，self 会自动推导为 memorised 对象
    auto test5 = lazy::make_memorised<unsigned int(unsigned int)>(fib_obj);
    std::cout << test5(15) << std::endl;
    std::cout << test5(15) << std::endl;  // 第二次调用全部命中缓存，无计算输出

    std::cout << "-----------------------make_memorised_fib_ptr" << std::endl;
    std::cout << "Not compilable: see comments in source code" << std::endl;
    // 下面的代码无法编译，原因详见 fib_ptr 定义处的注释：
    // auto test6 = lazy::make_memorised<unsigned int(unsigned int)>(fib_ptr);

    return 0;
}