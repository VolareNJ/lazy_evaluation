# C++ 函数式编程：惰性求值与通用记忆化

本项目展示了 C++ 中**惰性求值**和**通用记忆化**的实现，支持普通函数与递归函数的自动缓存。  
代码根据《C++函数式编程》书中的示例实践敲入；文件内的详细注释是学习过程中与 **DeepSeek** 反复讨论后总结出的坑点和关键知识点。本 README 也由 DeepSeek 辅助生成。

## 文件结构

- `lazy_evaluation.hpp` — 头文件，包含 `lazy::lazy_val`（惰性求值包装器）和 `lazy::memorised`（通用记忆化包装器）的实现。
- `demo.cpp` — 测试程序，演示了惰性求值和三种记忆化场景（非递归函数、递归 lambda、以及函数模板为何不能用于递归记忆化）。

## 功能

### 1. `lazy_val` — 一次性惰性求值

- 包装一个无参可调用对象，延迟执行，第一次访问结果时才计算。
- 线程安全：使用 `std::call_once` 保证多线程环境下只计算一次。
- 后续访问直接返回缓存的结果（`const` 引用）。

**示例**：
```cpp
auto val = lazy::make_lazy(six);   // 工厂函数自动推导类型
std::cout << val << std::endl;     // 第一次触发计算
std::cout << val << std::endl;     // 直接返回缓存
```

### 2. `memorised` — 通用记忆化包装器

- 记忆化任意可调用对象（函数指针、lambda、函数对象），根据参数缓存计算结果。
- **统一支持普通函数和递归函数**：编译期自动检测被包装的函数是否需要额外传递包装器自身以实现递归缓存。
- 线程安全：使用 `std::recursive_mutex` 保证并发安全，并允许递归函数在持有锁时再次进入包装器。
- 完美转发实参，保留值类别（左值/右值），避免不必要的拷贝。

**非递归示例**：
```cpp
auto memo_add = lazy::make_memorised<int(int, int)>(add);
std::cout << memo_add(1, 2);   // 计算并缓存
std::cout << memo_add(1, 2);   // 命中缓存
```

**递归示例（泛型 lambda）**：
```cpp
auto fib = [](auto& self, unsigned int n) -> unsigned int {
    if (n <= 1) return n;
    return self(n - 1) + self(n - 2);  // 递归走缓存
};
auto memo_fib = lazy::make_memorised<unsigned int(unsigned int)>(fib);
std::cout << memo_fib(15);   // 极速计算，所有子问题只算一次
```

## 编译与使用

**要求**：支持 C++17 或更高版本的编译器（GCC 7+, Clang 5+, MSVC 2017 15.3+）。

**编译**：
```bash
g++ -std=c++17 demo.cpp -o demo
```

**运行**：
```bash
./demo
```

## 关键设计要点

### 为什么递归函数必须用泛型 lambda？

记忆化包装器将自身作为第一个参数注入被包装函数，以便递归时通过缓存入口调用。普通函数（或函数模板）无法用泛型方式接受任意包装器类型，而泛型 lambda 的 `auto& self` 参数可以自动推导类型，完美适配这一需求。详见 `demo.cpp` 中 `fib_obj` 与 `fib_ptr` 的注释。

### 为什么需要 `null_param` 标签？

用于消除包装器构造函数与拷贝构造函数之间的重载歧义：当传入左值包装器对象时，若没有标签参数，模板构造函数会被错误推导为更优匹配，导致非法使用。

### 为什么使用递归互斥锁？

因为递归函数在持有缓存锁期间会再次调用同一个包装器的 `operator()`，如果使用普通互斥锁将导致同一线程死锁。

### 为什么用 `std::decay_t` 处理类型？

- 缓存键：将参数类型退化为值类型，保证查找时键的类型一致，避免因引用或 cv 限定符导致不匹配。
- 函数存储：去除引用和顶层 cv，确保包装器以值语义安全持有可调用对象副本，避免悬垂引用。

### 编译期分支 `if constexpr` + `is_invocable_v`

使用 C++17 的 `if constexpr` 和 `std::is_invocable_v` 在编译期检测被包装函数是否接受 `*this` 作为额外参数，从而自动选择递归或非递归调用路径。未采用的分支完全不会被编译，实现零开销的统一接口。

## 许可

此代码为学习目的编写，可自由使用和修改。  
代码来源：书中示例；注释来源：与 DeepSeek 讨论总结；README 生成：DeepSeek。