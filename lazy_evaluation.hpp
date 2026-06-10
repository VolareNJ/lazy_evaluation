#include <mutex>
#include <type_traits>
#include <map>
#include <tuple>

namespace lazy
{
    template <typename func_t>
    class lazy_val
    {

        using ret_t = std::invoke_result<func_t>::type;

    public:
        lazy_val(func_t computation)
            : m_computation(computation)
        {
        }

        operator const ret_t &() const
        {
            std::call_once(m_val_flag, [this]
                           { m_cache = m_computation(); });
            return m_cache;
        }

    private:
        func_t m_computation;
        mutable ret_t m_cache;
        mutable std::once_flag m_val_flag;
    };

    template <typename func_t>
    lazy_val<func_t> make_lazy(func_t computation)
    {
        return lazy_val<func_t>(computation);
    }

    template <typename ret_t, typename... args_t>
    class memorised
    {
    public:
        memorised(ret_t (*func)(args_t...)) : m_func(func) {}

        ret_t operator()(args_t... args)
        {
            const std::tuple<args_t...> args_tuple = std::make_tuple(args...);
            const auto cached = m_cache.find(args_tuple);

            if (cached == m_cache.end())
            {
                ret_t ret = m_func(args...);
                m_cache[args_tuple] = ret;
                return ret;
            }
            else
            {
                return cached->second;
            }
        }

    private:
        std::map<std::tuple<args_t...>, ret_t> m_cache;
        ret_t (*m_func)(args_t...);
    };

    template <typename ret_t, typename... args_t>
    memorised<ret_t, args_t...> make_memorised(ret_t (*func)(args_t... args))
    {
        return memorised<ret_t, args_t...>(func);
    }
}
