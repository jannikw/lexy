// Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef LEXY_CALLBACK_CONTAINER_HPP_INCLUDED
#define LEXY_CALLBACK_CONTAINER_HPP_INCLUDED

#include <lexy/callback/base.hpp>

namespace lexy
{
struct nullopt;

template <typename Container>
using _detect_reserve = decltype(LEXY_DECLVAL(Container&).reserve(std::size_t()));
template <typename Container>
constexpr auto _has_reserve = _detail::is_detected<_detect_reserve, Container>;

template <typename Container>
struct _list_sink
{
    Container _result;

    using return_type = Container;

    template <typename C = Container, typename U>
    auto operator()(U&& obj) -> decltype(LEXY_DECLVAL(C&).push_back(LEXY_FWD(obj)))
    {
        return _result.push_back(LEXY_FWD(obj));
    }

    template <typename C = Container, typename... Args>
    auto operator()(Args&&... args) -> decltype(LEXY_DECLVAL(C&).emplace_back(LEXY_FWD(args)...))
    {
        return _result.emplace_back(LEXY_FWD(args)...);
    }

    Container&& finish() &&
    {
        return LEXY_MOV(_result);
    }
};

template <typename Container, typename AllocFn>
struct _list_alloc
{
    AllocFn _alloc;

    using return_type = Container;

    template <typename State>
    struct _with_state
    {
        const State&   _state;
        const AllocFn& _alloc;

        constexpr Container operator()(Container&& container) const
        {
            return LEXY_MOV(container);
        }
        constexpr Container operator()(nullopt&&) const
        {
            return Container(_detail::invoke(_alloc, _state));
        }

        template <typename... Args>
        constexpr auto operator()(Args&&... args) const
            -> LEXY_DECAY_DECLTYPE((LEXY_DECLVAL(Container&).push_back(LEXY_FWD(args)), ...),
                                   LEXY_DECLVAL(Container))
        {
            Container result(_detail::invoke(_alloc, _state));
            if constexpr (_has_reserve<Container>)
                result.reserve(sizeof...(args));
            (result.emplace_back(LEXY_FWD(args)), ...);
            return result;
        }
    };

    template <typename State>
    constexpr auto operator[](const State& state) const
    {
        return _with_state<State>{state, _alloc};
    }

    template <typename State>
    constexpr auto sink(const State& state) const
    {
        return _list_sink<Container>{Container(_detail::invoke(_alloc, state))};
    }
};

template <typename Container>
struct _list
{
    using return_type = Container;

    constexpr Container operator()(Container&& container) const
    {
        return LEXY_MOV(container);
    }
    constexpr Container operator()(nullopt&&) const
    {
        return Container();
    }

    template <typename... Args>
    constexpr auto operator()(Args&&... args) const
        -> LEXY_DECAY_DECLTYPE((LEXY_DECLVAL(Container&).push_back(LEXY_FWD(args)), ...),
                               LEXY_DECLVAL(Container))
    {
        Container result;
        if constexpr (_has_reserve<Container>)
            result.reserve(sizeof...(args));
        (result.emplace_back(LEXY_FWD(args)), ...);
        return result;
    }
    template <typename C = Container, typename... Args>
    constexpr auto operator()(const typename C::allocator_type& allocator, Args&&... args) const
        -> decltype((LEXY_DECLVAL(C&).push_back(LEXY_FWD(args)), ...), C(allocator))
    {
        Container result(allocator);
        if constexpr (_has_reserve<Container>)
            result.reserve(sizeof...(args));
        (result.emplace_back(LEXY_FWD(args)), ...);
        return result;
    }

    constexpr auto sink() const
    {
        return _list_sink<Container>{Container()};
    }
    template <typename C = Container>
    constexpr auto sink(const typename C::allocator_type& allocator) const
    {
        return _list_sink<Container>{Container(allocator)};
    }

    template <typename AllocFn>
    constexpr auto allocator(AllocFn alloc_fn) const
    {
        return _list_alloc<Container, AllocFn>{alloc_fn};
    }
    constexpr auto allocator() const
    {
        return allocator([](const auto& alloc) { return alloc; });
    }
};

/// A callback with sink that creates a list of things (e.g. a `std::vector`, `std::list`, etc.).
/// It repeatedly calls `push_back()` and `emplace_back()`.
template <typename Container>
constexpr auto as_list = _list<Container>{};
} // namespace lexy

namespace lexy
{
template <typename Container>
struct _collection_sink
{
    Container _result;

    using return_type = Container;

    template <typename C = Container, typename U>
    auto operator()(U&& obj) -> decltype(LEXY_DECLVAL(C&).insert(LEXY_FWD(obj)))
    {
        return _result.insert(LEXY_FWD(obj));
    }

    template <typename C = Container, typename... Args>
    auto operator()(Args&&... args) -> decltype(LEXY_DECLVAL(C&).emplace(LEXY_FWD(args)...))
    {
        return _result.emplace(LEXY_FWD(args)...);
    }

    Container&& finish() &&
    {
        return LEXY_MOV(_result);
    }
};

template <typename Container, typename AllocFn>
struct _collection_alloc
{
    AllocFn _alloc;

    using return_type = Container;

    template <typename State>
    struct _with_state
    {
        const State&   _state;
        const AllocFn& _alloc;

        constexpr Container operator()(Container&& container) const
        {
            return LEXY_MOV(container);
        }
        constexpr Container operator()(nullopt&&) const
        {
            return Container(_detail::invoke(_alloc, _state));
        }

        template <typename... Args>
        constexpr auto operator()(Args&&... args) const
            -> LEXY_DECAY_DECLTYPE((LEXY_DECLVAL(Container&).insert(LEXY_FWD(args)), ...),
                                   LEXY_DECLVAL(Container))
        {
            Container result(_detail::invoke(_alloc, _state));
            if constexpr (_has_reserve<Container>)
                result.reserve(sizeof...(args));
            (result.emplace(LEXY_FWD(args)), ...);
            return result;
        }
    };

    template <typename State>
    constexpr auto operator[](const State& state) const
    {
        return _with_state<State>{state, _alloc};
    }

    template <typename State>
    constexpr auto sink(const State& state) const
    {
        return _collection_sink<Container>{Container(_detail::invoke(_alloc, state))};
    }
};

template <typename Container>
struct _collection
{
    using return_type = Container;

    constexpr Container operator()(Container&& container) const
    {
        return LEXY_MOV(container);
    }
    constexpr Container operator()(nullopt&&) const
    {
        return Container();
    }

    template <typename... Args>
    constexpr auto operator()(Args&&... args) const
        -> LEXY_DECAY_DECLTYPE((LEXY_DECLVAL(Container&).insert(LEXY_FWD(args)), ...),
                               LEXY_DECLVAL(Container))
    {
        Container result;
        if constexpr (_has_reserve<Container>)
            result.reserve(sizeof...(args));
        (result.emplace(LEXY_FWD(args)), ...);
        return result;
    }

    template <typename C = Container, typename... Args>
    constexpr auto operator()(const typename C::allocator_type& allocator, Args&&... args) const
        -> decltype((LEXY_DECLVAL(C&).insert(LEXY_FWD(args)), ...), C(allocator))
    {
        Container result(allocator);
        if constexpr (_has_reserve<Container>)
            result.reserve(sizeof...(args));
        (result.emplace(LEXY_FWD(args)), ...);
        return result;
    }

    constexpr auto sink() const
    {
        return _collection_sink<Container>{Container()};
    }
    template <typename C = Container>
    constexpr auto sink(const typename C::allocator_type& allocator) const
    {
        return _collection_sink<Container>{Container(allocator)};
    }

    template <typename AllocFn>
    constexpr auto allocator(AllocFn alloc_fn) const
    {
        return _collection_alloc<Container, AllocFn>{alloc_fn};
    }
    constexpr auto allocator() const
    {
        return allocator([](const auto& alloc) { return alloc; });
    }
};

/// A callback with sink that creates an unordered collection of things (e.g. a `std::set`,
/// `std::unordered_map`, etc.). It repeatedly calls `insert()` and `emplace()`.
template <typename T>
constexpr auto as_collection = _collection<T>{};
} // namespace lexy

namespace lexy
{
template <typename Container, typename Callback>
class _collect_sink
{
public:
    constexpr explicit _collect_sink(Callback callback) : _callback(LEXY_MOV(callback)) {}
    template <typename C = Container>
    constexpr explicit _collect_sink(Callback callback, const typename C::allocator_type& allocator)
    : _result(allocator), _callback(LEXY_MOV(callback))
    {}

    using return_type = Container;

    template <typename... Args>
    constexpr auto operator()(Args&&... args)
        -> decltype(void(LEXY_DECLVAL(Callback)(LEXY_FWD(args)...)))
    {
        _result.push_back(_callback(LEXY_FWD(args)...));
    }

    constexpr auto finish() &&
    {
        return LEXY_MOV(_result);
    }

private:
    Container                  _result;
    LEXY_EMPTY_MEMBER Callback _callback;
};
template <typename Callback>
class _collect_sink<void, Callback>
{
public:
    constexpr explicit _collect_sink(Callback callback) : _count(0), _callback(LEXY_MOV(callback))
    {}

    using return_type = std::size_t;

    template <typename... Args>
    constexpr auto operator()(Args&&... args)
        -> decltype(void(LEXY_DECLVAL(Callback)(LEXY_FWD(args)...)))
    {
        _callback(LEXY_FWD(args)...);
        ++_count;
    }

    constexpr auto finish() &&
    {
        return _count;
    }

private:
    std::size_t                _count;
    LEXY_EMPTY_MEMBER Callback _callback;
};

template <typename Container, typename Callback>
class _collect
{
public:
    constexpr explicit _collect(Callback callback) : _callback(LEXY_MOV(callback)) {}

    constexpr auto sink() const
    {
        return _collect_sink<Container, Callback>(_callback);
    }
    template <typename C = Container>
    constexpr auto sink(const typename C::allocator_type& allocator) const
    {
        return _collect_sink<Container, Callback>(_callback, allocator);
    }

private:
    LEXY_EMPTY_MEMBER Callback _callback;
};

/// Returns a sink that invokes the void-returning callback multiple times, resulting in the number
/// of times it was invoked.
template <typename Callback>
constexpr auto collect(Callback&& callback)
{
    using callback_t = std::decay_t<Callback>;
    static_assert(std::is_void_v<typename callback_t::return_type>,
                  "need to specify a container to collect into for non-void callbacks");
    return _collect<void, callback_t>(LEXY_FWD(callback));
}

/// Returns a sink that invokes the callback multiple times, storing each result in the container.
template <typename Container, typename Callback>
constexpr auto collect(Callback&& callback)
{
    using callback_t = std::decay_t<Callback>;
    static_assert(!std::is_void_v<typename callback_t::return_type>,
                  "cannot collect a void callback into a container");
    return _collect<Container, callback_t>(LEXY_FWD(callback));
}
} // namespace lexy

#endif // LEXY_CALLBACK_CONTAINER_HPP_INCLUDED

