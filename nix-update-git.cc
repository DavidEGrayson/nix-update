// Standard headers
#include <sstream>
#include <iostream>
#include <memory>

// libnixexpr headers
#include <nixexpr.hh>
#include <parser-tab.hh>
#include <eval.hh>
#include <shared.hh>
#include <json-to-value.hh>

// headers from this project
#include "expr-helpers.hh"

// TODO: add support for --deepClone and other options to fetchGit/nix-prefetch-git

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
};

struct StringReplacement
{
    uint32_t line, column;
    std::string oldString;
    std::string newString;

    StringReplacement(const ExprStringAndPos & e, const std::string & newString)
    {
        this->line = e.pos.line;
        this->column = e.pos.column;
        this->oldString = e.string();
        this->newString = newString;
    }
};

std::ostream & operator << (std::ostream & str, const ExprStringAndPos & v)
{
    str << "string at " << v.pos << ':' << std::endl;
    str << "  " << v.c_str() << std::endl;
    return str;
}

struct FetchGitApp
{
    const nix::ExprApp * app;
    ExprStringAndPos urlString, revString, hashString;
    std::string newRev;
    std::string newHash;

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
// Returns information about the string attribute as a ExprStringAndPos object.
// Returns true if successful.
std::pair<ExprStringAndPos, bool> findStringFromApp(
    const nix::ExprApp * app,
    const std::string & name)
{
    std::pair<ExprStringAndPos, bool> result;

    nix::ExprAttrs * attrs = dynamic_cast<nix::ExprAttrs *>(app->e2);
    if (attrs == nullptr) { return result; }

    for (auto & symbolAndAttr : attrs->attrs)
    {
        // Continue looping if this attribute has the wrong name.
        if (name != (const std::string &)symbolAndAttr.first) { continue; }

        // Continue looping if this attribute's value is not a single non-indented string.
        auto * es = dynamic_cast<const nix::ExprString *>(symbolAndAttr.second.e);
        if (es == nullptr) { continue; }
        if (es->v.type != nix::ValueType::tString) { continue; }

        // nix::ExprString does not know its position.  To get the
        // position of tee string, we assume that the string starts on
        // the same line as the attribute definition and there is
        // exactly one space on each side of the equal sign.
        nix::Pos pos = symbolAndAttr.second.pos;
        pos.column += name.length() + 3;

        result.first.expr = es;
        result.first.pos = pos;
        result.second = true;
        return result;
    }

    return result;
}

std::pair<std::string, bool> findStringFromBindings(nix::Bindings & bindings,
    const std::string & name)
{
    std::pair<std::string, bool> result;

    for (size_t i = 0; i < bindings.size(); i++)
    {
        nix::Attr & attr = bindings[i];

        // Continue looping if this attribute has the wrong name.
        if (name != (const std::string &)attr.name) { continue; }

        // Make sure this attribute's value is a string.
        if (attr.value->type != nix::ValueType::tString)
        {
            throw std::runtime_error("expected a string, got something else");
        }

        result.first = std::string(attr.value->string.s);
        result.second = true;
        return result;
    }

    return result;
}

std::pair<FetchGitApp, bool> tryInterpretAsFetchGitApp(nix::Expr * expr)
{
    std::pair<FetchGitApp, bool> result;
    FetchGitApp & fga = result.first;

    fga.app = tryInterpretAsApp(expr, "fetchgit");
    if (fga.app == nullptr) { return result; }

    auto strResult = findStringFromApp(fga.app, "url");
    if (!strResult.second) { return result; }
    fga.urlString = strResult.first;

    strResult = findStringFromApp(fga.app, "rev");
    if (!strResult.second) { return result; }
    fga.revString = strResult.first;

    strResult = findStringFromApp(fga.app, "sha256");
    if (!strResult.second) { return result; }
    fga.hashString = strResult.first;

    result.second = true;
    return result;
}

/** Runs a shell command using popen.  The command's standard output is
 * returned, while the standard error goes to the standard error of this
 * process.  Errors are converted into exceptions. */
std::string runShellCommand(const std::string & cmd)
{
    FILE * fp = popen(cmd.c_str(), "r");
    if (fp == nullptr)
    {
        int ev = errno;
        std::string what = std::string("Failed to start command: ") + cmd;
        throw std::system_error(ev, std::system_category(), what);
    }

    std::string output;
    while (1)
    {
        if (feof(fp)) { break; }

        if (ferror(fp))
        {
            pclose(fp);
            std::string what = "Failed to read from pipe";
            throw std::runtime_error(what);
        }

        char buffer[128];
        if (fgets(buffer, sizeof(buffer), fp) != nullptr)
        {
            output.append(buffer);
        }
    }

    int ret = pclose(fp);
    if (ret == -1)
    {
        int ev = errno;
        std::string what = "pclose failed";
        throw std::system_error(ev, std::system_category(), what);
    }
    else if (ret != 0)
    {
        std::string what = std::string("Command `") + cmd + "` failed";
        if (WEXITSTATUS(ret))
        {
            what += " with error code " + std::to_string(WEXITSTATUS(ret));
        }
        throw std::runtime_error(what);
    }

    return output;
}

// Use nix-prefetch-git to get updated info about the upstream repository.
// (Requires internet access.)
void getLatestGitInfo(FetchGitApp & fga, nix::EvalState & state)
{
    // Fetch the info from the git repository.
    std::string cmd = std::string("nix-prefetch-git ") + fga.urlString.string();
    std::string json = runShellCommand(cmd);

    // Parse the JSON returned from nix-prefetch-git using nix's JSON parser.
    nix::Value value;
    parseJSON(state, json, value);
    if (value.type != nix::ValueType::tAttrs)
    {
        throw std::runtime_error("JSON from nix-prefetch-git is not a hash.");
    }

    // Make sure the url from the JSON response is what we are expecting.
    auto result = findStringFromBindings(*value.attrs, "url");
    if (!result.second)
    {
        throw std::runtime_error("JSON from nix-prefetch-git is missing the key 'url'.");
    }
    if (result.first != fga.urlString.string())
    {
        throw std::runtime_error("JSON from nix-prefetch-git has a url that does " \
            "not match what we expected.");
    }

    // Get the rev and sha256 values and store them in fga.
    result = findStringFromBindings(*value.attrs, "rev");
    if (!result.second)
    {
        throw std::runtime_error("JSON from nix-prefetch-git is missing the key 'rev'.");
    }
    fga.newRev = result.first;
    result = findStringFromBindings(*value.attrs, "sha256");
    if (!result.second)
    {
        throw std::runtime_error("JSON from nix-prefetch-git is missing the key 'sha256'.");
    }
    fga.newHash = result.first;
}

std::vector<StringReplacement> getStringReplacements(const FetchGitApp & app)
{
    std::vector<StringReplacement> r;
    r.push_back(StringReplacement(app.revString, app.newRev));
    r.push_back(StringReplacement(app.hashString, app.newHash));
    return r;
}

int main(int argc, char ** argv)
{
    return nix::handleExceptions(argv[0], [&]() {
        nix::initNix();
        nix::initGC();

        // TODO: don't hardcode path
        std::string path = "/home/david/nixpkgs/pkgs/development/tools/build-managers/lazy/default.nix";

        // Open the .nix file and parse it.
        nix::Strings searchPath;
        nix::EvalState state(searchPath);
        nix::Expr * mainExpr = state.parseExprFromFile(path);

        // Traverse the parsed representation of the file and gather
        // information about all calls (applications) of fetchgit.
        std::vector<FetchGitApp> fetchGitApps;
        ExprVisitorFunction finder([&](nix::Expr * e) {
            auto result = tryInterpretAsFetchGitApp(e);
            if (result.second) { fetchGitApps.push_back(result.first); }
            return true;
        });
        ExprDepthFirstSearch(&finder).visit(mainExpr);

        // Get updated info about the upstream repository.  (Requires internet access.)
        for (FetchGitApp & fga : fetchGitApps)
        {
            getLatestGitInfo(fga, state);
        }

        // Get the info about what replacements need to be made in the file.
        std::vector<StringReplacement> replacements;
        for (FetchGitApp & fga : fetchGitApps)
        {
            for (const StringReplacement & sr : getStringReplacements(fga))
            {
                if (sr.newString != sr.oldString)
                {
                    replacements.push_back(sr);
                }
            }
        }

        // TODO: write the updated info to the file

        // Print debugging info.
        if (0)
        {
            for (const FetchGitApp & fga : fetchGitApps)
            {
                std::cout << fga.app->pos << std::endl;
                std::cout << *const_cast<nix::ExprApp *>(fga.app) << std::endl;
                std::cout << fga.urlString << std::endl;
                std::cout << fga.revString << std::endl;
                std::cout << fga.hashString << std::endl;
                std::cout << fga.newRev << std::endl;
                std::cout << fga.newHash << std::endl;
            }
        }
        if (0)
        {
            for (const StringReplacement & sr : replacements)
            {
                std::cout << sr.line << ':' << sr.column << ' ' << \
                    sr.oldString << '!' << sr.newString << std::endl;
            }
        }
        return 0;
    });
}
