// Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef LEXY_DSL_SYMBOL_HPP_INCLUDED
#define LEXY_DSL_SYMBOL_HPP_INCLUDED

#include <lexy/_detail/integer_sequence.hpp>
#include <lexy/_detail/iterator.hpp>
#include <lexy/_detail/nttp_string.hpp>
#include <lexy/_detail/trie.hpp>
#include <lexy/dsl/base.hpp>
#include <lexy/dsl/capture.hpp>
#include <lexy/error.hpp>
#include <lexy/lexeme.hpp>

namespace lexy
{
#define LEXY_SYMBOL(Str) LEXY_NTTP_STRING(::lexy::_detail::type_string, Str)

template <typename T, typename... Strings>
class _symbol_table
{
    static auto _char_type()
    {
        if constexpr (sizeof...(Strings) == 0)
            return;
        else
            return std::common_type_t<typename Strings::char_type...>{};
    }

public:
    using char_type   = decltype(_char_type());
    using key_type    = char_type;
    using mapped_type = T;

    struct value_type
    {
        const char_type*   symbol;
        const mapped_type& value;
    };

    //=== modifiers ===//
    LEXY_CONSTEVAL _symbol_table() : _data{} {}

    template <typename SymbolString, typename... Args>
    LEXY_CONSTEVAL auto map(Args&&... args) const
    {
        using next_table = _symbol_table<T, Strings..., SymbolString>;
        if constexpr (empty())
            return next_table(lexy::_detail::make_index_sequence<0>{}, nullptr, LEXY_FWD(args)...);
        else
            return next_table(lexy::_detail::make_index_sequence<size()>{}, _data,
                              LEXY_FWD(args)...);
    }

#if LEXY_HAS_NTTP
    template <_detail::string_literal SymbolString, typename... Args>
    LEXY_CONSTEVAL auto map(Args&&... args) const
    {
        return map<_detail::to_type_string<_detail::type_string, SymbolString>>(LEXY_FWD(args)...);
    }
#else
#    if (defined(__clang__) && __clang_major__ <= 7)                                               \
        || (defined(__clang__) && defined(__apple_build_version__) && __clang_major__ <= 10)
    template <char C, typename... Args> // Sorry, compiler bug.
#    else
    template <auto C, typename... Args>
#    endif
    LEXY_CONSTEVAL auto map(Args&&... args) const
    {
        return map<_detail::type_string<LEXY_DECAY_DECLTYPE(C), C>>(LEXY_FWD(args)...);
    }
#endif

    //=== access ===//
    static constexpr bool empty() noexcept
    {
        return size() == 0;
    }

    static constexpr std::size_t size() noexcept
    {
        return sizeof...(Strings);
    }

    class iterator
    : public lexy::_detail::bidirectional_iterator_base<iterator, value_type, value_type, void>
    {
    public:
        constexpr iterator() noexcept : _table(nullptr), _idx(0) {}

        constexpr value_type deref() const noexcept
        {
            if constexpr (empty())
            {
                LEXY_PRECONDITION(false);
                return value_type{"", LEXY_DECLVAL(T)};
            }
            else
            {
                LEXY_PRECONDITION(_table);
                constexpr const char_type* strings[] = {Strings::template c_str<char_type>...};
                return value_type{strings[_idx], _table->_data[_idx]};
            }
        }

        constexpr void increment() noexcept
        {
            LEXY_PRECONDITION(_idx != sizeof...(Strings));
            ++_idx;
        }
        constexpr void decrement() noexcept
        {
            LEXY_PRECONDITION(_idx != 0);
            --_idx;
        }

        constexpr bool equal(iterator rhs) const noexcept
        {
            LEXY_PRECONDITION(_table == rhs._table);
            return _idx == rhs._idx;
        }

    private:
        constexpr iterator(const _symbol_table* table, std::size_t idx) noexcept
        : _table(table), _idx(idx)
        {}

        const _symbol_table* _table;
        std::size_t          _idx;

        friend _symbol_table;
    };

    constexpr iterator begin() const noexcept
    {
        return iterator(this, 0);
    }
    constexpr iterator end() const noexcept
    {
        return iterator(this, size());
    }

    struct key_index
    {
        std::size_t _value;

        constexpr key_index() noexcept : _value(std::size_t(-1)) {}
        constexpr explicit key_index(std::size_t idx) noexcept : _value(idx)
        {
            LEXY_PRECONDITION(_value < size());
        }

        constexpr explicit operator bool() const noexcept
        {
            return _value < size();
        }

        friend constexpr bool operator==(key_index lhs, key_index rhs) noexcept
        {
            return lhs._value == rhs._value;
        }
        friend constexpr bool operator!=(key_index lhs, key_index rhs) noexcept
        {
            return lhs._value != rhs._value;
        }
    };

    template <typename Reader>
    constexpr key_index try_parse(Reader& reader) const
    {
        static_assert(!empty(), "symbol table must not be empty");
        constexpr auto& trie = _trie<typename Reader::encoding>::object;

        auto result = _detail::trie_parser<trie>::parse(reader);
        if (result == trie.invalid_value)
            return key_index();
        else
            return key_index(result);
    }

    constexpr const T& operator[](key_index idx) const noexcept
    {
        LEXY_PRECONDITION(idx);
        return _data[idx._value];
    }

private:
    template <typename Encoding>
    struct _trie
    {
        static constexpr auto object = lexy::_detail::trie<Encoding, Strings...>;
    };

    template <std::size_t... Idx, typename... Args>
    constexpr explicit _symbol_table(lexy::_detail::index_sequence<Idx...>, const T* data,
                                     Args&&... args)
    // New data is appended at the end.
    : _data{data[Idx]..., T(LEXY_FWD(args)...)}
    {}

    std::conditional_t<empty(), char, T> _data[empty() ? 1 : size()];

    template <typename, typename...>
    friend class _symbol_table;
};

template <typename T>
constexpr auto symbol_table = _symbol_table<T>{};
} // namespace lexy

namespace lexy
{
struct unknown_symbol
{
    static LEXY_CONSTEVAL auto name()
    {
        return "unknown symbol";
    }
};
} // namespace lexy

namespace lexyd
{
template <typename Leading, typename Trailing>
struct _idp;
template <typename Leading, typename Trailing, typename... Reserved>
struct _id;

template <const auto& Table, typename Token, typename Tag>
struct _sym : branch_base
{
    template <typename Reader>
    struct bp
    {
        typename Reader::iterator end;
        typename LEXY_DECAY_DECLTYPE(Table)::key_index symbol;

        template <typename ControlBlock>
        constexpr bool try_parse(ControlBlock&, const Reader& reader)
        {
            // Try and parse the token.
            lexy::token_parser_for<Token, Reader> parser(reader);
            if (!parser.try_parse(reader))
                return false;
            end = parser.end;

            // Check whether this is a symbol.
            auto content = lexy::partial_reader(reader, end);
            symbol       = Table.try_parse(content);

            // We need to consume everything.
            if (content.position() != end)
                return false;

            // Only succeed if it is a symbol.
            return static_cast<bool>(symbol);
        }

        template <typename Context>
        constexpr void cancel(Context&)
        {}

        template <typename NextParser, typename Context, typename... Args>
        LEXY_PARSER_FUNC bool finish(Context& context, Reader& reader, Args&&... args)
        {
            // We need to consume and report the token.
            context.on(_ev::token{}, Token{}, reader.position(), end);
            reader.set_position(end);

            // And continue parsing with the symbol value after whitespace skipping.
            using continuation = lexy::whitespace_parser<Context, NextParser>;
            return continuation::parse(context, reader, LEXY_FWD(args)..., Table[symbol]);
        }
    };

    template <typename NextParser>
    struct p
    {
        template <typename... PrevArgs>
        struct _cont
        {
            template <typename Context, typename Reader>
            LEXY_PARSER_FUNC static bool parse(Context& context, Reader& reader, PrevArgs&&... args,
                                               lexy::lexeme<Reader> lexeme)
            {
                // Check whether the captured lexeme is a symbol.
                auto content = lexy::partial_reader(reader, lexeme.begin(), lexeme.end());
                auto symbol  = Table.try_parse(content);
                if (!symbol || content.position() != lexeme.end())
                {
                    // Unknown symbol.
                    using tag = lexy::_detail::type_or<Tag, lexy::unknown_symbol>;
                    auto err  = lexy::error<Reader, tag>(lexeme.begin(), lexeme.end());
                    context.on(_ev::error{}, err);
                    return false;
                }

                // Continue parsing with the symbol value.
                return NextParser::parse(context, reader, LEXY_FWD(args)..., Table[symbol]);
            }
        };

        template <typename Context, typename Reader, typename... Args>
        LEXY_PARSER_FUNC static bool parse(Context& context, Reader& reader, Args&&... args)
        {
            // Capture the token and continue with special continuation.
            return lexy::parser_for<_capt<Token>, _cont<Args...>>::parse(context, reader,
                                                                         LEXY_FWD(args)...);
        }
    };

    //=== dsl ===//
    template <typename ErrorTag>
    static constexpr _sym<Table, Token, ErrorTag> error = {};
};

// Optimization for identifiers: instead of parsing an entire identifier (which requires checking
// every character against the char class), parse a symbol and check whether the next character
// would continue the identifier. This is the same optimization that is done for keywords.
template <const auto& Table, typename L, typename T, typename Tag>
struct _sym<Table, _idp<L, T>, Tag> : branch_base
{
    template <typename Reader>
    struct bp
    {
        typename LEXY_DECAY_DECLTYPE(Table)::key_index symbol;
        typename Reader::iterator end;

        template <typename ControlBlock>
        constexpr bool try_parse(const ControlBlock*, Reader reader)
        {
            // Try to parse a symbol.
            symbol = Table.try_parse(reader);
            if (!symbol)
                return false;
            end = reader.position();

            // We had a symbol, but it must not be the prefix of a valid identifier.
            return !lexy::try_match_token(T{}, reader);
        }

        template <typename Context>
        constexpr void cancel(Context&)
        {}

        template <typename NextParser, typename Context, typename... Args>
        LEXY_PARSER_FUNC bool finish(Context& context, Reader& reader, Args&&... args)
        {
            // We need to consume and report the identifier pattern.
            context.on(_ev::token{}, _idp<L, T>{}, reader.position(), end);
            reader.set_position(end);

            // And continue parsing with the symbol value after whitespace skipping.
            using continuation = lexy::whitespace_parser<Context, NextParser>;
            return continuation::parse(context, reader, LEXY_FWD(args)..., Table[symbol]);
        }
    };

    template <typename NextParser>
    struct p
    {
        template <typename Context, typename Reader, typename... Args>
        LEXY_PARSER_FUNC static bool parse(Context& context, Reader& reader, Args&&... args)
        {
            auto begin = reader.position();

            // Try to parse a symbol that is not the prefix of an identifier.
            auto symbol_reader = reader;
            auto symbol        = Table.try_parse(symbol_reader);
            if (!symbol || lexy::try_match_token(T{}, symbol_reader))
            {
                // Unknown symbol or not an identifier.
                // Parse the identifier pattern normally, and see if that fails.
                using id_parser = lexy::parser_for<_idp<L, T>, lexy::pattern_parser<>>;
                if (!id_parser::parse(context, reader))
                    // It did fail, so it reported an error and we're done here.
                    return false;

                // We're having a valid identifier but unknown symbol.
                using tag = lexy::_detail::type_or<Tag, lexy::unknown_symbol>;
                auto err  = lexy::error<Reader, tag>(begin, reader.position());
                context.on(_ev::error{}, err);

                return false;
            }
            else
            {
                // We need to consume and report the identifier pattern.
                auto end = symbol_reader.position();
                context.on(_ev::token{}, _idp<L, T>{}, begin, end);
                reader.set_position(end);

                // And continue parsing with the symbol value after whitespace skipping.
                using continuation = lexy::whitespace_parser<Context, NextParser>;
                return continuation::parse(context, reader, LEXY_FWD(args)..., Table[symbol]);
            }
        }
    };

    //=== dsl ===//
    template <typename ErrorTag>
    static constexpr _sym<Table, _idp<L, T>, ErrorTag> error = {};
};

template <const auto& Table, typename Tag>
struct _sym<Table, void, Tag> : branch_base
{
    template <typename Reader>
    struct bp
    {
        typename LEXY_DECAY_DECLTYPE(Table)::key_index symbol;
        typename Reader::iterator end;

        template <typename ControlBlock>
        constexpr bool try_parse(const ControlBlock*, Reader reader)
        {
            // Try to parse a symbol.
            symbol = Table.try_parse(reader);
            end    = reader.position();

            // Only succeed if it is a symbol.
            return static_cast<bool>(symbol);
        }

        template <typename Context>
        constexpr void cancel(Context&)
        {}

        template <typename NextParser, typename Context, typename... Args>
        LEXY_PARSER_FUNC bool finish(Context& context, Reader& reader, Args&&... args)
        {
            // We need to consume and report the token.
            context.on(_ev::token{}, lexy::identifier_token_kind, reader.position(), end);
            reader.set_position(end);

            // And continue parsing with the symbol value after whitespace skipping.
            using continuation = lexy::whitespace_parser<Context, NextParser>;
            return continuation::parse(context, reader, LEXY_FWD(args)..., Table[symbol]);
        }
    };

    template <typename NextParser>
    struct p
    {
        template <typename Context, typename Reader, typename... Args>
        LEXY_PARSER_FUNC static bool parse(Context& context, Reader& reader, Args&&... args)
        {
            bp<Reader> impl{};
            if (impl.try_parse(context.control_block, reader))
                return impl.template finish<NextParser>(context, reader, LEXY_FWD(args)...);
            impl.cancel(context);

            // Unknown symbol.
            using tag = lexy::_detail::type_or<Tag, lexy::unknown_symbol>;
            auto err  = lexy::error<Reader, tag>(reader.position());
            context.on(_ev::error{}, err);

            return false;
        }
    };

    //=== dsl ===//
    template <typename ErrorTag>
    static constexpr _sym<Table, void, ErrorTag> error = {};
};

template <const auto& Table>
struct _sym_dsl : _sym<Table, void, void>
{
    template <typename Token>
    constexpr auto operator()(Token) const
    {
        static_assert(lexy::is_token_rule<Token>);
        return _sym<Table, Token, void>{};
    }
    template <typename L, typename T, typename... R>
    constexpr auto operator()(_id<L, T, R...> id) const
    {
        static_assert(sizeof...(R) == 0,
                      "symbol() must not be used in the presence of reserved identifiers");
        return _sym<Table, decltype(id.pattern()), void>{};
    }
};

/// Parses optional rule, then matches the resulting lexeme against the symbol table.
template <const auto& Table>
constexpr auto symbol = _sym_dsl<Table>{};
} // namespace lexyd

#endif // LEXY_DSL_SYMBOL_HPP_INCLUDED

