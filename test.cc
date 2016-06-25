#include <sstream>
#include <iostream>
#include <nixexpr.hh>
#include <parser-tab.hh>
#include <eval.hh>
#include <shared.hh>
#include "expr-helpers.hh"

struct FetchGitApp
{
    nix::ExprApp * app;
    nix::ExprString * urlStringExpr;
    nix::ExprString * revStringExpr;
    nix::ExprString * hashStringExpr;
};

// Checks to see if the given expression is an application of a function
// with the given name.  If it is, returns it as a pointer.
// Otherwise, returns a null pointer.
nix::ExprApp * tryInterpretAsApp(nix::Expr * expr, const char * name)
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

std::pair<FetchGitApp, bool> tryInterpretAsFetchGitApp(nix::Expr * expr)
{
    auto result = std::pair<FetchGitApp, bool>(FetchGitApp(), false);
    FetchGitApp & fga = result.first;

    fga.app = tryInterpretAsApp(expr, "fetchgit");
    if (fga.app == nullptr) { return result; }

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
            std::cout << *fga.app << std::endl;
        }
        return 0;
    });
}
