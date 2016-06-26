#include "libupdate.hh"

#include <system_error>
#include <stdio.h>

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
