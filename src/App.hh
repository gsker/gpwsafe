//
// App.hh
//
//     Created: 24.01.2012
//      Author: A. Sakhnik
//

#pragma once

#include <string>
#include <boost/noncopyable.hpp>

namespace gPWS {

struct iEmitter;

class cApp
    : boost::noncopyable
{
public:
    cApp(char const *program_name);

    int Init(int argc, char *argv[]);

    int Run();

private:
    char const *_program_name;
    std::string _file_name;

    enum eCommand
    {
        _C_LIST = 0
    } _command;

    enum eEmitter
    {
        _E_XCLIP = 0,
        _E_STDOUT
    } _emitter;

    bool _user;
    bool _password;

    char const *_argument;

    int _Run();
    int _Usage(bool fail);
    int _DoList();
    void _PrintIntention(iEmitter const *emitter);
};

} //namespace gPWS;

// vim: set et ts=4 sw=4: