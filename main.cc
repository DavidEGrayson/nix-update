#include <sstream>
#include <iostream>
#include <nixexpr.hh>
#include <parser-tab.hh>
#include <eval.hh>
#include <shared.hh>
#include "expr-helpers.hh"

struct StringInfo
{
    const nix::ExprString * expr = nullptr;
    nix::Pos pos;

    const char * cstr() const
    {
        return expr->v.string.s;
    }
};

struct FetchGitApp
{
    const nix::ExprApp * app;
    StringInfo urlString, revString, hashString;
};

// Checks to see if the given expression is an application of a function
// with the given name.  If it is, returns it as a pointer.
// Otherwise, returns a null pointer.
nix::ExprApp * tryInterpretAsApp(nix::Expr * expr, const std::string & name)
{
    // Make sure the expression is a function application.
    nix::ExprApp * app = dynamic_cast<nix::ExprApp *>(expr);
    if (app == nullptr) { return nullptr; }

    // Make sure the function being applied is an ExprVar.  We might
    // also need to support ExprSelect in the future.
    nix::ExprVar * funcNameVar = dynamic_cast<nix::ExprVar *>(app->e1);
    if (funcNameVar == nullptr) { return nullptr; }

    // Make sure the function has the right name.
    const std::string & funcNameStr = funcNameVar->name;
    if (funcNameStr != name) { return nullptr; }

    return app;
}

// Checks the given function application for a string attribute with the given name.
// The attribute value must be a single non-idented literal string.
// Returns information about the string attribute as a StringInfo object.
// Returns true if successful.
bool findStringAttr(
    const nix::ExprApp * app,
    const std::string & name,
    StringInfo & info)
{
    nix::ExprAttrs * attrs = dynamic_cast<nix::ExprAttrs *>(app->e2);
    if (attrs == nullptr) { return false; }

    for (auto & symbolAndAttr : attrs->attrs)
    {
        // Continue looping if this attribute has the wrong name.
        if (name != (const std::string &)symbolAndAttr.first) { continue; }

        // Continue looping if this attribute's value is not a single non-indented string.
        auto * es = dynamic_cast<const nix::ExprString *>(symbolAndAttr.second.e);
        if (es == nullptr) { continue; }
        if (es->v.type != nix::ValueType::tString) { continue; }

        info.expr = es;
        info.pos = symbolAndAttr.second.pos;

        return true;
    }

    return false;
}

std::pair<FetchGitApp, bool> tryInterpretAsFetchGitApp(nix::Expr * expr)
{
    auto result = std::pair<FetchGitApp, bool>(FetchGitApp(), false);
    FetchGitApp & fga = result.first;

    fga.app = tryInterpretAsApp(expr, "fetchgit");
    if (fga.app == nullptr) { return result; }

    if (!findStringAttr(fga.app, "url", fga.urlString)) { return result; }
    if (!findStringAttr(fga.app, "rev", fga.revString)) { return result; }
    if (!findStringAttr(fga.app, "sha256", fga.hashString)) { return result; }

    result.second = true;
    return result;
}

int main(int argc, char ** argv)
{
    return nix::handleExceptions(argv[0], [&]() {
        nix::initNix();
        nix::initGC();

        // TODO: don't hardcode path
        std::string path = "/home/david/nixpkgs/pkgs/development/tools/build-managers/lazy/default.nix";

        nix::Strings searchPath;
        nix::EvalState state(searchPath);
        nix::Expr * mainExpr = state.parseExprFromFile(path);

        std::vector<FetchGitApp> fetchGitApps;
        ExprVisitorFunction finder([&](nix::Expr * e) {
            auto result = tryInterpretAsFetchGitApp(e);
            if (result.second) { fetchGitApps.push_back(result.first); }
            return true;
        });
        ExprDepthFirstSearch search(&finder);
        search.visit(mainExpr);
        for (FetchGitApp & fga : fetchGitApps)
        {
            std::cout << fga.app->pos << std::endl;
            std::cout << *const_cast<nix::ExprApp *>(fga.app) << std::endl;
            std::cout << fga.urlString.cstr() << std::endl;
            std::cout << fga.revString.cstr() << std::endl;
            std::cout << fga.hashString.pos << std::endl;
            std::cout << fga.hashString.cstr() << std::endl;
        }
        return 0;
    });
}
