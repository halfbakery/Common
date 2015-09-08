/*
 * Handler.cpp
 *
 *  Created on: 20-dec.-2014
 *      Author: lieven2
 */
#define __STDC_LIMIT_MACROS
#include "Handler.h"
#include "Sys.h"
#include "Msg.h"
Handler* Handler::_firstChild = 0;
Handler::Handler()
{
    _timeout = UINT64_MAX;
    _name = "UNDEFINED";
    _next = 0;
//      _firstChild = 0;
    restart();
    reg(this);
}

Handler::Handler(const char* name)
{
    _timeout = UINT64_MAX;
    _name = name;
    _next = 0;
//      _firstChild = 0;
    restart();
    reg(this);
}

void Handler::timeout(uint32_t msec)
{
    _timeout = Sys::millis() + msec;
}

bool Handler::timeout()
{
    return _timeout < Sys::millis();
}

uint64_t Handler::getTimeout()
{
    return _timeout;
}

const char* Handler::getName(){
return _name;
}
//_________________________________________________________________________________________________
//
//       HANDLER LIST
//_________________________________________________________________________________________________
Handler* Handler::first()
{
    return _firstChild;
}

Handler* Handler::next()
{
    return _next;
}

void Handler::reg(Handler* hdlr)
{
    if (_firstChild == 0)
        _firstChild = hdlr;
    else
    {
        Handler* cursor = _firstChild;
        while (cursor->_next != 0)
        {
            cursor = cursor->_next;
        }
        cursor->_next = hdlr;
    }
}

//_________________________________________________________________________________________________
//
//       LISTENER LIST
//_________________________________________________________________________________________________
Handler* hdlr;
void Handler::dispatchToChilds(Msg& msg)
{
    Handler* hdlr;
    for (hdlr = first(); hdlr != 0; hdlr = hdlr->next())
    {
        if (hdlr->isRunning())
            hdlr->dispatch(msg);
    }
}
