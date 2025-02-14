// Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef LEXY_DETAIL_CODE_POINT_HPP_INCLUDED
#define LEXY_DETAIL_CODE_POINT_HPP_INCLUDED

#include <lexy/code_point.hpp>
#include <lexy/input/base.hpp>

//=== encoding ===//
namespace lexy::_detail
{
template <typename Encoding>
constexpr std::size_t encode_code_point(code_point cp, typename Encoding::char_type* buffer,
                                        std::size_t size)
{
    if constexpr (std::is_same_v<Encoding, lexy::ascii_encoding>)
    {
        LEXY_PRECONDITION(cp.is_ascii());
        LEXY_PRECONDITION(size >= 1);

        *buffer = char(cp.value());
        return 1;
    }
    else if constexpr (std::is_same_v<Encoding, lexy::utf8_encoding>)
    {
        LEXY_PRECONDITION(cp.is_valid());

        // Taken from http://www.herongyang.com/Unicode/UTF-8-UTF-8-Encoding-Algorithm.html.
        if (cp.is_ascii())
        {
            LEXY_PRECONDITION(size >= 1);

            buffer[0] = LEXY_CHAR8_T(cp.value());
            return 1;
        }
        else if (cp.value() <= 0x07'FF)
        {
            LEXY_PRECONDITION(size >= 2);

            auto first  = (cp.value() >> 6) & 0x1F;
            auto second = (cp.value() >> 0) & 0x3F;

            buffer[0] = LEXY_CHAR8_T(0xC0 | first);
            buffer[1] = LEXY_CHAR8_T(0x80 | second);
            return 2;
        }
        else if (cp.value() <= 0xFF'FF)
        {
            LEXY_PRECONDITION(size >= 3);

            auto first  = (cp.value() >> 12) & 0x0F;
            auto second = (cp.value() >> 6) & 0x3F;
            auto third  = (cp.value() >> 0) & 0x3F;

            buffer[0] = LEXY_CHAR8_T(0xE0 | first);
            buffer[1] = LEXY_CHAR8_T(0x80 | second);
            buffer[2] = LEXY_CHAR8_T(0x80 | third);
            return 3;
        }
        else
        {
            LEXY_PRECONDITION(size >= 4);

            auto first  = (cp.value() >> 18) & 0x07;
            auto second = (cp.value() >> 12) & 0x3F;
            auto third  = (cp.value() >> 6) & 0x3F;
            auto fourth = (cp.value() >> 0) & 0x3F;

            buffer[0] = LEXY_CHAR8_T(0xF0 | first);
            buffer[1] = LEXY_CHAR8_T(0x80 | second);
            buffer[2] = LEXY_CHAR8_T(0x80 | third);
            buffer[3] = LEXY_CHAR8_T(0x80 | fourth);
            return 4;
        }
    }
    else if constexpr (std::is_same_v<Encoding, lexy::utf16_encoding>)
    {
        LEXY_PRECONDITION(cp.is_valid());

        if (cp.is_bmp())
        {
            LEXY_PRECONDITION(size >= 1);

            buffer[0] = char16_t(cp.value());
            return 1;
        }
        else
        {
            // Algorithm implemented from
            // https://en.wikipedia.org/wiki/UTF-16#Code_points_from_U+010000_to_U+10FFFF.
            LEXY_PRECONDITION(size >= 2);

            auto u_prime       = cp.value() - 0x1'0000;
            auto high_ten_bits = u_prime >> 10;
            auto low_ten_bits  = u_prime & 0b0000'0011'1111'1111;

            buffer[0] = char16_t(0xD800 + high_ten_bits);
            buffer[1] = char16_t(0xDC00 + low_ten_bits);
            return 2;
        }
    }
    else if constexpr (std::is_same_v<Encoding, lexy::utf32_encoding>)
    {
        LEXY_PRECONDITION(cp.is_valid());
        LEXY_PRECONDITION(size >= 1);

        *buffer = char32_t(cp.value());
        return 1;
    }
    else
    {
        static_assert(lexy::_detail::error<Encoding>,
                      "cannot encode a code point in this encoding");
        (void)cp;
        (void)buffer;
        (void)size;
        return 0;
    }
}
} // namespace lexy::_detail

//=== parsing ===//
namespace lexy::_detail
{
enum class cp_error
{
    success,
    eof,
    leads_with_trailing,
    missing_trailing,
    surrogate,
    overlong_sequence,
    out_of_range,
    predicate_failure,
};

template <typename Reader>
struct cp_result
{
    lexy::code_point          cp;
    cp_error                  error;
    typename Reader::iterator end;
};

template <typename Reader>
constexpr cp_result<Reader> parse_code_point(Reader reader)
{
    if constexpr (std::is_same_v<typename Reader::encoding, lexy::ascii_encoding>)
    {
        if (reader.peek() == Reader::encoding::eof())
            return {{}, cp_error::eof, reader.position()};

        auto cur = reader.peek();
        reader.bump();

        auto cp = lexy::code_point(static_cast<char32_t>(cur));
        if (cp.is_ascii())
            return {cp, cp_error::success, reader.position()};
        else
            return {cp, cp_error::out_of_range, reader.position()};
    }
    else if constexpr (std::is_same_v<typename Reader::encoding, lexy::utf8_encoding>)
    {
        constexpr auto payload_lead1 = 0b0111'1111;
        constexpr auto payload_lead2 = 0b0001'1111;
        constexpr auto payload_lead3 = 0b0000'1111;
        constexpr auto payload_lead4 = 0b0000'0111;
        constexpr auto payload_cont  = 0b0011'1111;

        constexpr auto pattern_lead1 = 0b0 << 7;
        constexpr auto pattern_lead2 = 0b110 << 5;
        constexpr auto pattern_lead3 = 0b1110 << 4;
        constexpr auto pattern_lead4 = 0b11110 << 3;
        constexpr auto pattern_cont  = 0b10 << 6;

        auto first = reader.peek();
        if ((first & ~payload_lead1) == pattern_lead1)
        {
            // ASCII character.
            reader.bump();
            return {lexy::code_point(first), cp_error::success, reader.position()};
        }
        else if ((first & ~payload_cont) == pattern_cont)
        {
            return {{}, cp_error::leads_with_trailing, reader.position()};
        }
        else if ((first & ~payload_lead2) == pattern_lead2)
        {
            reader.bump();

            auto second = reader.peek();
            if ((second & ~payload_cont) != pattern_cont)
                return {{}, cp_error::missing_trailing, reader.position()};
            reader.bump();

            auto result = char32_t(first & payload_lead2);
            result <<= 6;
            result |= char32_t(second & payload_cont);

            // C0 and C1 are overlong ASCII.
            if (first == 0xC0 || first == 0xC1)
                return {lexy::code_point(result), cp_error::overlong_sequence, reader.position()};
            else
                return {lexy::code_point(result), cp_error::success, reader.position()};
        }
        else if ((first & ~payload_lead3) == pattern_lead3)
        {
            reader.bump();

            auto second = reader.peek();
            if ((second & ~payload_cont) != pattern_cont)
                return {{}, cp_error::missing_trailing, reader.position()};
            reader.bump();

            auto third = reader.peek();
            if ((third & ~payload_cont) != pattern_cont)
                return {{}, cp_error::missing_trailing, reader.position()};
            reader.bump();

            auto result = char32_t(first & payload_lead3);
            result <<= 6;
            result |= char32_t(second & payload_cont);
            result <<= 6;
            result |= char32_t(third & payload_cont);

            auto cp = lexy::code_point(result);
            if (cp.is_surrogate())
                return {cp, cp_error::surrogate, reader.position()};
            else if (first == 0xE0 && second < 0xA0)
                return {cp, cp_error::overlong_sequence, reader.position()};
            else
                return {cp, cp_error::success, reader.position()};
        }
        else if ((first & ~payload_lead4) == pattern_lead4)
        {
            reader.bump();

            auto second = reader.peek();
            if ((second & ~payload_cont) != pattern_cont)
                return {{}, cp_error::missing_trailing, reader.position()};
            reader.bump();

            auto third = reader.peek();
            if ((third & ~payload_cont) != pattern_cont)
                return {{}, cp_error::missing_trailing, reader.position()};
            reader.bump();

            auto fourth = reader.peek();
            if ((fourth & ~payload_cont) != pattern_cont)
                return {{}, cp_error::missing_trailing, reader.position()};
            reader.bump();

            auto result = char32_t(first & payload_lead4);
            result <<= 6;
            result |= char32_t(second & payload_cont);
            result <<= 6;
            result |= char32_t(third & payload_cont);
            result <<= 6;
            result |= char32_t(fourth & payload_cont);

            auto cp = lexy::code_point(result);
            if (!cp.is_valid())
                return {cp, cp_error::out_of_range, reader.position()};
            else if (first == 0xF0 && second < 0x90)
                return {cp, cp_error::overlong_sequence, reader.position()};
            else
                return {cp, cp_error::success, reader.position()};
        }
        else // FE or FF
        {
            return {{}, cp_error::eof, reader.position()};
        }
    }
    else if constexpr (std::is_same_v<typename Reader::encoding, lexy::utf16_encoding>)
    {
        constexpr auto payload1 = 0b0000'0011'1111'1111;
        constexpr auto payload2 = payload1;

        constexpr auto pattern1 = 0b110110 << 10;
        constexpr auto pattern2 = 0b110111 << 10;

        if (reader.peek() == Reader::encoding::eof())
            return {{}, cp_error::eof, reader.position()};

        auto first = char16_t(reader.peek());
        if ((first & ~payload1) == pattern1)
        {
            reader.bump();
            if (reader.peek() == Reader::encoding::eof())
                return {{}, cp_error::missing_trailing, reader.position()};

            auto second = char16_t(reader.peek());
            if ((second & ~payload2) != pattern2)
                return {{}, cp_error::missing_trailing, reader.position()};
            reader.bump();

            // We've got a valid code point.
            auto result = char32_t(first & payload1);
            result <<= 10;
            result |= char32_t(second & payload2);
            result |= 0x10000;
            return {lexy::code_point(result), cp_error::success, reader.position()};
        }
        else if ((first & ~payload2) == pattern2)
        {
            return {{}, cp_error::leads_with_trailing, reader.position()};
        }
        else
        {
            // Single code unit code point; always valid.
            reader.bump();
            return {lexy::code_point(first), cp_error::success, reader.position()};
        }
    }
    else if constexpr (std::is_same_v<typename Reader::encoding, lexy::utf32_encoding>)
    {
        if (reader.peek() == Reader::encoding::eof())
            return {{}, cp_error::eof, reader.position()};

        auto cur = reader.peek();
        reader.bump();

        auto cp = lexy::code_point(cur);
        if (!cp.is_valid())
            return {cp, cp_error::out_of_range, reader.position()};
        else if (cp.is_surrogate())
            return {cp, cp_error::surrogate, reader.position()};
        else
            return {cp, cp_error::success, reader.position()};
    }
    else
    {
        static_assert(lexy::_detail::error<typename Reader::encoding>,
                      "no known code point for this encoding");
        return {};
    }
}

template <typename Reader>
constexpr void recover_code_point(Reader& reader, cp_result<Reader> result)
{
    switch (result.error)
    {
    case cp_error::success:
        LEXY_PRECONDITION(false);
        break;
    case cp_error::eof:
        // We don't need to do anything to "recover" from EOF.
        break;

    case cp_error::leads_with_trailing:
        // Invalid code unit, consume to recover.
        LEXY_PRECONDITION(result.end == reader.position());
        reader.bump();
        break;

    case cp_error::missing_trailing:
    case cp_error::surrogate:
    case cp_error::out_of_range:
    case cp_error::overlong_sequence:
    case cp_error::predicate_failure:
        // Consume all the invalid code units to recover.
        reader.set_position(result.end);
        break;
    }
}
} // namespace lexy::_detail

#endif // LEXY_DETAIL_CODE_POINT_HPP_INCLUDED

