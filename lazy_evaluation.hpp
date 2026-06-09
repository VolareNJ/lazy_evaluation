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

    /*
    template<typename func_t>
    class lazy_val_alt
    {
        using ret_t = std::invoke_result<func_t>::type;
    private:
        mutable bool m_cache_initialised;
        mutable std::mutex m_mtx;
        mutable ret_t m_cache;
        func_t m_computation;

    public:
        lazy_val_alt(func_t computation)
            : m_computation(computation)
            , m_cache_initialised(false)
        {}

        bool is_initialised() const
        {
            return m_cache_initialised?true:false;
        }

        operator const ret_t& () const
        {
            if(!m_cache_initialised)
            {
                std::unique_lock<std::mutex> lk(m_mtx);
                m_cache = m_computation();
                m_cache_initialised = true;
            }
            return m_cache;
        }

    };
    */

    template <typename ret_t, typename... args_t>
    class memorised
    {
        memorised(ret_t (*func)(args_t...)) : m_func(func) {}

        ret_t operator()(args_t... args)
        {
            const std::tuple<args_t...> args_tuple = std::make_tuple(args...);
            const auto cached = m_cache.find(args_tuple);

            if (cached == m_cache.end())
            {
                ret_t ret = func(args...);
                m_cache[args_tuple] = ret;
                return ret;
            }
            else
            {
                return cached->second();
            }
        }

    private:
        std::map<std::tuple<args_t...>, ret_t> m_cache;
        ret_t (*m_func)(args_t...);
    };
}