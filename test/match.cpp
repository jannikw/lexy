// Copyright (C) 2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <lexy/match.hpp>

#include <doctest.h>
#include <lexy/dsl/capture.hpp>
#include <lexy/dsl/literal.hpp>
#include <lexy/input/string_input.hpp>

TEST_CASE("match")
{
    SUBCASE("pattern")
    {
        SUBCASE("match")
        {
            auto input  = lexy::zstring_input("abc");
            auto result = lexy::match(input, LEXY_LIT("abc"));
            CHECK(result);
            CHECK(input.peek() == lexy::default_encoding::eof());
        }
        SUBCASE("no match")
        {
            auto input  = lexy::zstring_input("def");
            auto result = lexy::match(input, LEXY_LIT("abc"));
            CHECK(!result);
            CHECK(input.peek() == 'd');
        }
        SUBCASE("partial match")
        {
            auto input  = lexy::zstring_input("abc123");
            auto result = lexy::match(input, LEXY_LIT("abc"));
            CHECK(result);
            CHECK(input.peek() == '1');
        }
    }
    SUBCASE("rule")
    {
        SUBCASE("match")
        {
            auto input  = lexy::zstring_input("abc");
            auto result = lexy::match(input, capture(LEXY_LIT("abc")));
            CHECK(result);
            CHECK(input.peek() == lexy::default_encoding::eof());
        }
        SUBCASE("no match")
        {
            auto input  = lexy::zstring_input("def");
            auto result = lexy::match(input, capture(LEXY_LIT("abc")));
            CHECK(!result);
            CHECK(input.peek() == 'd');
        }
        SUBCASE("partial match")
        {
            auto input  = lexy::zstring_input("abc123");
            auto result = lexy::match(input, capture(LEXY_LIT("abc")));
            CHECK(result);
            CHECK(input.peek() == '1');
        }
    }
}

