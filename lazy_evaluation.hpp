#include <mutex>
#include <type_traits>
#include <map>
#include <tuple>

namespace lazy
{
    // ------------------------------------------------------------
    // lazy_val: 一次性惰性求值包装器
    // ------------------------------------------------------------
    template <typename func_t>
    class lazy_val
    {
        // 推导计算结果的类型 (C++17 invoke_result_t 亦可)
        using ret_t = typename std::invoke_result<func_t>::type;

    public:
        // 存储计算函数，不立即执行
        lazy_val(func_t computation)
            : m_computation(computation)
        {}

        // 隐式转换到结果类型的常量引用，第一次访问时执行计算
        operator const ret_t& () const
        {
            // std::call_once 保证多线程下也只计算一次
            std::call_once(m_val_flag, [this]
                {
                    m_cache = m_computation();
                });
            return m_cache;
        }

    private:
        func_t m_computation;               // 延迟的计算函数
        mutable ret_t m_cache;              // mutable：在 const 转换函数中可修改
        mutable std::once_flag m_val_flag;  // call_once 的标志
    };

    // 工厂函数：自动推导函数类型，生成 lazy_val 对象
    template <typename func_t>
    lazy_val<func_t> make_lazy(func_t computation)
    {
        return lazy_val<func_t>(computation);
    }


    // ------------------------------------------------------------
    // 标签类型，用于构造函数消歧义（详见 memorised 类）
    // ------------------------------------------------------------
    class null_param
    {};


    // ------------------------------------------------------------
    // memorised: 通用记忆化包装器（支持普通函数和递归函数）
    // ------------------------------------------------------------
    template <typename func_sig, typename func_t>
    class memorised;

    // 特化：匹配 Result(Args...) 形式
    template <typename ret_t, typename... args_t, typename func_t>
    class memorised<ret_t(args_t...), func_t>
    {
        // 缓存的键类型：将参数退化（去除引用、cv 限定符），保证类型一致
        using tup_t = std::tuple<std::decay_t<args_t>...>;

    public:
        /*
         * 模板构造函数：使用转发引用接收可调用对象
         * 为什么需要 null_param 作为第二个参数？
         *  - 避免与拷贝构造函数冲突。
         *  - 若无此参数，当传入一个左值 memorised 对象时，模板构造函数
         *    会被推导为更优匹配（T 推导为 memorised&），导致本应调用
         *    拷贝构造的场合误入此模板，产生悬垂引用或编译错误。
         *  - 加入 null_param 后，调用方必须显式提供 null_param{}，
         *    从而明确意图为“包装新函数”。
         */
        template <typename c_func_t>
        memorised(c_func_t&& func, null_param)
            : m_func(std::forward<c_func_t>(func))  // 使用转发保持值类别
        {}

        // 拷贝构造函数：只复制被包装的函数，不复制缓存
        memorised(const memorised& other)
            : m_func(other.m_func)
        {}

        /*
         * 调用运算符（模板化实现完美转发）
         * - c_args_t&& 是转发引用（万能引用），可以接受左值或右值实参，
         *   并以零拷贝方式传递。
         * - 虽然签名是 Result(Args...)，但允许传入可隐式转换的类型。
         */
        template <typename... c_args_t>
        ret_t operator()(c_args_t&&... args) const
        {
            // 递归互斥锁：因为递归函数内部会再次调用本 operator()，
            // 普通 mutex 会导致同一线程死锁
            std::unique_lock<std::recursive_mutex> lk(m_rmtx);

            // 构造键（make_tuple 会退化类型，与 tup_t 匹配）
            const auto args_tuple = std::make_tuple(args...);  // 类型: tuple<decay_t<Args>...>
            const auto cached = m_cache.find(args_tuple);

            if (cached == m_cache.end())
            {
                // 编译期判断：被包装的函数 f 是否接受 *this 作为第一个参数
                // - 如果是递归函数（如 [](auto& self, int n){...}），则走第一个分支
                // - 如果是普通函数（如 [](int n){...}），则走第二个分支
                // if constexpr 保证另一个分支完全不编译，从而实现一个类兼容两种用法
                if constexpr (std::is_invocable_v<func_t,
                    const memorised&,
                    c_args_t&&...>)
                {
                    // 递归版本：传递 *this 作为第一个参数，函数内部通过它递归调用
                    auto&& ret = m_func(*this, std::forward<c_args_t>(args)...);
                    m_cache[args_tuple] = ret;
                    return ret;
                }
                else
                {
                    // 普通版本：直接调用，不传递额外参数
                    auto&& ret = m_func(std::forward<c_args_t>(args)...);
                    m_cache[args_tuple] = ret;
                    return ret;
                }
            }
            else
            {
                // 缓存命中，直接返回
                return cached->second;
            }
        }

    private:
        mutable std::map<tup_t, ret_t> m_cache;   // mutable: const 方法中可写入
        mutable std::recursive_mutex m_rmtx;      // 递归锁：支持自身递归调用
        func_t m_func;                             // 被包装的函数对象
    };


    // ------------------------------------------------------------
    // 工厂函数：创建 memorised 包装器
    // ------------------------------------------------------------
    template <typename func_sig, typename func_t>
    memorised<func_sig, std::decay_t<func_t>> make_memorised(func_t&& func)
    {
        /*
         * 为什么使用 std::decay_t<func_t> 作为模板参数？
         *  - 当传入左值时 func_t 会被推导为 T&（引用类型），
         *    若直接将引用作为类模板参数，则 m_func 会变成引用成员，
         *    导致对象不拥有函数对象，极易产生悬垂引用。
         *  - decay_t 去除引用和 cv，得到纯净的值类型，
         *    保证 memorised 总是以值语义存储一份可调用对象的副本。
         */
        return memorised<func_sig, std::decay_t<func_t>>(
            std::forward<func_t>(func),
            null_param {}
        );
    }
}