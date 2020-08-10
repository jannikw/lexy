// Copyright (C) 2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <lexy/dsl/sequence.hpp>

#include "verify.hpp"
#include <lexy/dsl/capture.hpp>
#include <lexy/dsl/label.hpp>

TEST_CASE("rule: sequence")
{
    constexpr auto rule
        = LEXY_LIT("a") + lexy::dsl::label<struct lab> + LEXY_LIT("b") + capture(LEXY_LIT("c"));
    CHECK(lexy::is_rule<decltype(rule)>);
    CHECK(!lexy::is_pattern<decltype(rule)>);

    struct callback
    {
        const char* str;

        constexpr int success(const char* cur, lexy::label<lab>, lexy::lexeme<test_input> lex)
        {
            assert(str + 3 == cur);
            assert(lex.string_view() == "c");
            return 0;
        }

        constexpr int error(test_error<lexy::expected_literal> e)
        {
            if (e.string() == "a")
                return -1;
            else if (e.string() == "b")
                return -2;
            else if (e.string() == "c")
                return -3;

            assert(false);
        }
    };

    constexpr auto empty = rule_matches<callback>(rule, "");
    CHECK(empty == -1);
    constexpr auto a = rule_matches<callback>(rule, "a");
    CHECK(a == -2);
    constexpr auto ab = rule_matches<callback>(rule, "ab");
    CHECK(ab == -3);

    constexpr auto abc = rule_matches<callback>(rule, "abc");
    CHECK(abc == 0);
}

