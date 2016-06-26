#pragma once

#include <nixexpr.hh>

#include <string>

struct StringReplacement
{
    uint32_t line, column;
    std::string oldString;
    std::string newString;
};

/** Stores information about a parsed string literal: its ExprString
 * object and its position in this file.  */
struct ExprStringAndPos
{
    const nix::ExprString * expr = nullptr;

    nix::Pos pos;

    const char * c_str() const
    {
        if (expr == nullptr) { return ""; }
        if (expr->v.type != nix::ValueType::tString) { return ""; }
        return expr->v.string.s;
    }

    std::string string() const
    {
        return c_str();
    }

    StringReplacement replacement(const std::string & newString) const
    {
        StringReplacement sr;
        sr.line = pos.line;
        sr.column = pos.column;
        sr.oldString = string();
        sr.newString = newString;
        return sr;
    }

};

std::ostream & operator << (std::ostream & str, const ExprStringAndPos & v);

nix::ExprApp * tryInterpretAsApp(nix::Expr * expr, const std::string & name);

std::pair<ExprStringAndPos, bool> findStringFromApp(
    const nix::ExprApp * app,
    const std::string & name);

std::pair<std::string, bool> findStringFromBindings(nix::Bindings & bindings,
    const std::string & name);

std::string runShellCommand(const std::string & cmd);
