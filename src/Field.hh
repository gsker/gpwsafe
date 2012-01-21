//
// Field.hh
//
//     Created: 20.01.2012
//      Author: A. Sakhnik
//

#pragma once

#include "Defs.hh"

#include <stdint.h>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace gPWS {

struct sField
    : boost::noncopyable
{
    typedef boost::shared_ptr<sField> PtrT;

    uint32_t length;
    uint8_t type;
    StringX value;

    void *operator new(size_t n);
    void operator delete(void *p, size_t n);
};

} //namespace gPWS;

// vim: set et ts=4 sw=4:
