// Copyright (C) 2020-2021 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef LEXY_ACTION_PARSE_AS_TREE_HPP_INCLUDED
#define LEXY_ACTION_PARSE_AS_TREE_HPP_INCLUDED

#include <lexy/action/base.hpp>
#include <lexy/action/validate.hpp>
#include <lexy/parse_tree.hpp>

namespace lexy
{
template <typename Tree, typename Input, typename ErrorCallback>
class parse_tree_handler
{
public:
    explicit parse_tree_handler(Tree& tree, const Input& input, const ErrorCallback& cb)
    : _tree(&tree), _depth(0), _validate(input, cb)
    {}

    template <typename Production>
    class event_handler
    {
        using iterator = typename lexy::input_reader<Input>::iterator;

    public:
        void on(parse_tree_handler& handler, parse_events::production_start ev, iterator pos)
        {
            if (handler._depth++ == 0)
                handler._builder.emplace(LEXY_MOV(*handler._tree), Production{});
            else
                _marker = handler._builder->start_production(Production{});

            _validate.on(handler._validate, ev, pos);
        }

        void on(parse_tree_handler& handler, parse_events::production_finish ev, iterator pos)
        {
            if (--handler._depth == 0)
                *handler._tree = LEXY_MOV(*handler._builder).finish();
            else
                handler._builder->finish_production(LEXY_MOV(_marker));

            _validate.on(handler._validate, ev, pos);
        }

        void on(parse_tree_handler& handler, parse_events::production_cancel ev, iterator pos)
        {
            if (--handler._depth == 0)
                handler._tree->clear();
            else
            {
                // Cancelling the production removes all nodes from the tree.
                // To ensure that the parse tree remains lossless, we add everything consumed by it
                // as an error token.
                handler._builder->cancel_production(LEXY_MOV(_marker));
                handler._builder->token(lexy::error_token_kind, _validate.production_begin(), pos);
            }

            _validate.on(handler._validate, ev, pos);
        }

        template <typename TokenKind>
        void on(parse_tree_handler& handler, parse_events::token ev, TokenKind kind, iterator begin,
                iterator end)
        {
            handler._builder->token(kind, begin, end);
            _validate.on(handler._validate, ev, kind, begin, end);
        }

        template <typename Error>
        void on(parse_tree_handler& handler, parse_events::error ev, Error&& error)
        {
            _validate.on(handler._validate, ev, LEXY_FWD(error));
        }

        template <typename Event, typename... Args>
        void on(parse_tree_handler& handler, Event ev, Args&&... args)
        {
            _validate.on(handler._validate, ev, LEXY_FWD(args)...);
        }

    private:
        typename Tree::builder::marker _marker;
        typename validate_handler<Input, ErrorCallback>::template event_handler<Production>
            _validate;
    };

    template <typename Production, typename State>
    using value_callback = _detail::void_value_callback;

    constexpr auto get_result_void(bool rule_parse_result) &&
    {
        return LEXY_MOV(_validate).get_result_void(rule_parse_result);
    }

private:
    lexy::_detail::lazy_init<typename Tree::builder> _builder;
    Tree*                                            _tree;
    int                                              _depth;

    validate_handler<Input, ErrorCallback> _validate;
};

template <typename Production, typename TokenKind, typename MemoryResource, typename Input,
          typename ErrorCallback>
auto parse_as_tree(parse_tree<lexy::input_reader<Input>, TokenKind, MemoryResource>& tree,
                   const Input& input, const ErrorCallback& callback)
    -> validate_result<ErrorCallback>
{
    auto handler = parse_tree_handler(tree, input, LEXY_MOV(callback));
    auto reader  = input.reader();
    return lexy::do_action<Production>(LEXY_MOV(handler), no_parse_state, reader);
}

template <typename Production, typename TokenKind, typename MemoryResource, typename Input,
          typename State, typename ErrorCallback>
auto parse_as_tree(parse_tree<lexy::input_reader<Input>, TokenKind, MemoryResource>& tree,
                   const Input& input, const State& state, const ErrorCallback& callback)
    -> validate_result<ErrorCallback>
{
    auto handler = parse_tree_handler(tree, input, LEXY_MOV(callback));
    auto reader  = input.reader();
    return lexy::do_action<Production>(LEXY_MOV(handler), &state, reader);
}
} // namespace lexy

#endif // LEXY_ACTION_PARSE_AS_TREE_HPP_INCLUDED

