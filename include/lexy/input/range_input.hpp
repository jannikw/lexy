// Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef LEXY_INPUT_RANGE_INPUT_HPP_INCLUDED
#define LEXY_INPUT_RANGE_INPUT_HPP_INCLUDED

#include <lexy/error.hpp>
#include <lexy/input/base.hpp>
#include <lexy/lexeme.hpp>

namespace lexy
{
template <typename Encoding, typename Iterator, typename Sentinel = Iterator>
class range_input
{
public:
    using encoding  = Encoding;
    using char_type = typename encoding::char_type;

    using iterator = Iterator;
    using sentinel = Sentinel;

    //=== constructors ===//
    constexpr range_input() noexcept : _begin(), _end() {}

    constexpr range_input(iterator begin, sentinel end) noexcept : _begin(begin), _end(end) {}

    //=== access ===//
    constexpr iterator begin() const noexcept
    {
        return _begin;
    }

    constexpr sentinel end() const noexcept
    {
        return _end;
    }

    //=== reader ===//
    constexpr auto reader() const& noexcept
    {
        return _range_reader<Encoding>(_begin, _end);
    }

private:
    Iterator                   _begin;
    LEXY_EMPTY_MEMBER Sentinel _end;
};

template <typename Iterator, typename Sentinel>
range_input(Iterator begin, Sentinel end)
    -> range_input<deduce_encoding<LEXY_DECAY_DECLTYPE(*begin)>, Iterator, Sentinel>;
} // namespace lexy

#endif // LEXY_INPUT_RANGE_INPUT_HPP_INCLUDED

