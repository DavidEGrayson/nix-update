#include "libupdate.hh"

#include <parser-tab.hh>

#include <stdio.h>
#include <sstream>
#include <fstream>
#include <system_error>
#include <cassert>

std::ostream & operator << (std::ostream & str, const ExprStringAndPos & v)
{
    str << "string at " << v.pos << ':' << std::endl;
    str << "  " << v.c_str() << std::endl;
    return str;
}

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

#include <iostream>

// Note: This will add a blank line to the last line.
// Note: I am not sure what would happen in Windows with \r\n line endings.
static void performReplacementsCore(
    const std::vector<StringReplacement> & sortedReplacements,
    std::istream & input, std::ostream & output)
{
    auto ri = sortedReplacements.begin();
    size_t lineNumber = 0;

    while (1)
    {
        // Read a line from the input.
        lineNumber++;
        std::string line;
        std::getline(input, line);
        if (input.fail())
        {
            if (input.eof())
            {
                break;
            }
            throw std::runtime_error("Failed to read line.");
        }

        // Apply all the replacements needed on this line.
        while (ri != sortedReplacements.end() &&
            ri->line == lineNumber)
        {
            // Make sure that the current contents of the line match what we expect.
            if (line.substr(ri->column - 1, ri->oldString.size()) != ri->oldString)
            {
                throw std::runtime_error("File contents mismatch.");
            }

            // Perform the replacement.
            line.replace(ri->column - 1, ri->oldString.size(), ri->newString);

            // Advance to the next replacement in the sorted list.
            ++ri;
        }

        // Write the modified line to the output.
        output << line << std::endl;
        if (output.fail())
        {
            throw std::runtime_error("Failed to write line.");
        }
    }

    if (ri != sortedReplacements.end())
    {
        throw std::runtime_error("File has fewer lines than expected.");
    }
}

void performReplacements(const std::string & path,
    const std::vector<StringReplacement> & replacements)
{
    // Sort the replacements by line number.
    std::vector<StringReplacement> sortedReplacements = replacements;
    std::sort (sortedReplacements.begin(), sortedReplacements.end(),
        [](const StringReplacement & a, const StringReplacement & b) {
            if (a.line == b.line) { return a.column > b.column; }
            return a.line < b.line;
    });

    std::string modifiedFile;

    {
        std::ifstream inputStream(path);
        if (inputStream.fail())
        {
            int ev = errno;
            std::string what = std::string("Failed to open file for input: ") + path;
            throw std::system_error(ev, std::system_category(), what);
        }
        std::ostringstream stringStream;
        performReplacementsCore(sortedReplacements, inputStream, stringStream);
        modifiedFile = stringStream.str();
    }

    {
        std::ofstream outputStream(path);
        if (outputStream.fail())
        {
            int ev = errno;
            std::string what = std::string("Failed to open file for writing: ") + path;
            throw std::system_error(ev, std::system_category(), what);
        }
        outputStream << modifiedFile;
        if (outputStream.fail())
        {
            throw std::runtime_error("Failed to write to file.");
        }
    }
}

