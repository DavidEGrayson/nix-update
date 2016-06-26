#include "standard.hh"

// TODO: add support for --deepClone and other options to fetchGit/nix-prefetch-git

const char * help =
    "Usage: nix-update-git [OPTION]... NIXFILE\n"
    "Updates calls to fetchgit in the NIXFILE to fetch latest upstream version.\n"
    "\n"
    "Options:\n"
    "  -h, --help    Show this help screen\n"
    // TODO: "  --version     Show version number\n"
    "  -q, --quiet   Suppress non-error output\n";

struct NixUpdateGitOptions
{
    bool showHelp = false;
    bool showVersion = false;
    bool quiet = false;
    std::string path;
};

struct FetchGitApp
{
    const nix::ExprApp * app;
    ExprStringAndPos urlString, revString, hashString;
    std::string newRev;
    std::string newHash;
};

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

// Use nix-prefetch-git to get updated info about the upstream repository.
// (Requires internet access.)
void getLatestGitInfo(FetchGitApp & fga, nix::EvalState & state, bool quiet)
{
    // Prevent security problems when assembling the shell command below.
    if (fga.urlString.string().find('\'') != std::string::npos)
    {
        throw std::runtime_error("Git repository name has a single quote in it.");
    }

    // Fetch the info from the git repository.
    std::string cmd = "nix-prefetch-git ";
    cmd += std::string("\'") + fga.urlString.string() + std::string("\'");
    if (quiet) { cmd += " 2>/dev/null"; }

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
        throw std::runtime_error("JSON from nix-prefetch-git has a url that does "
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
    r.push_back(app.revString.replacement(app.newRev));
    r.push_back(app.hashString.replacement(app.newHash));
    return r;
}

NixUpdateGitOptions parseArgs(int argc, char ** argv)
{
    NixUpdateGitOptions options;

    if (argc == 1)
    {
        options.showHelp = true;
        return options;
    }

    nix::parseCmdLine(argc, argv, [&](nix::Strings::iterator & arg,
            const nix::Strings::iterator & end)
    {
        if (*arg == "--help" || *arg == "-h")
        {
            options.showHelp = true;
        }
        else if (*arg == "--version")
        {
            options.showVersion = true;
        }
        else if (*arg == "--quiet" || *arg == "-q")
        {
            options.quiet = true;
        }
        else if (*arg != "" && arg->at(0) == '-')
        {
            return false;
        }
        else
        {
            if (!options.path.empty())
            {
                throw std::runtime_error("Only one file path argument is allowed.");
            }
            options.path = *arg;
        }
        return true;
    });

    if (options.path.empty() && !options.showHelp && !options.showVersion)
    {
        throw std::runtime_error("No files were specified.");
    }

    return options;
}

int nixUpdateGit(const NixUpdateGitOptions & options)
{
    // Open the .nix file and parse it.
    nix::Strings searchPath;
    nix::EvalState state(searchPath);
    nix::Expr * mainExpr = state.parseExprFromFile(options.path);

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
        getLatestGitInfo(fga, state, options.quiet);
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

    // Update the .nix file if needed.
    if (replacements.size() > 0)
    {
        performReplacements(options.path, replacements);
        if (!options.quiet)
        {
            std::cerr << "Updated: " << options.path << std::endl;
        }
    }
    else
    {
        if (!options.quiet)
        {
            std::cerr << "Already up-to-date: " << options.path << std::endl;
        }
    }

    return 0;
}

int mainWithExceptions(int argc, char ** argv)
{
    NixUpdateGitOptions options = parseArgs(argc, argv);
    if (options.showHelp)
    {
        std::cout << help;
        return 0;
    }

    if (options.showVersion)
    {
        // TODO: show version
        return 0;
    }

    return nixUpdateGit(options);
}

int main(int argc, char ** argv)
{
    return nix::handleExceptions(argv[0], [&]() {
        nix::initNix();
        nix::initGC();
        mainWithExceptions(argc, argv);
    });
}
