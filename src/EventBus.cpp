#include "EventBus.h"

Cbor* timeoutEvent;

EventBus::EventBus(uint32_t size,uint32_t msgSize) :
    _queue(size), _firstFilter(0),_txd(msgSize),_rxd(msgSize) {
    timeoutEvent=new Cbor(12);
    timeoutEvent->addKeyValue(EB_SRC, H("system"));
    timeoutEvent->addKeyValue(EB_EVENT, H("timeout"));
    publish(H("system"),H("setup"));
}

void EventBus::publish(uint16_t header, Cbor& cbor) {
    Cbor msg(0);
    _queue.putMap(msg);
    msg.addKey(0).add(header);
    msg.append(cbor);
    _queue.putRelease(msg);
}

void EventBus::publish(uint16_t src, uint16_t ev) {
    event(src,ev);
    send();
}

Cbor& EventBus::event(uint16_t src, uint16_t event) {
    if(_txd.length() != 0 ) WARN (" EB.txd not cleared ");
    _txd.clear();
    _txd.addKeyValue(EB_SRC,src);
    _txd.addKeyValue(EB_EVENT,event);
    return _txd;
}

Cbor& EventBus::request(uint16_t dst,uint16_t req,uint16_t src) {
    if(_txd.length() != 0 ) WARN (" EB.txd not cleared ");
    _txd.clear();
    _txd.addKeyValue(EB_DST,dst);
    _txd.addKeyValue(EB_REQUEST,req);
    _txd.addKeyValue(EB_SRC,src);
    return _txd;
}
Cbor& EventBus::reply(uint16_t dst,uint16_t repl,uint16_t src) {
    if(_txd.length() != 0 ) WARN (" EB.txd not cleared ");
    _txd.clear();
    _txd.addKeyValue(EB_DST,dst);
    _txd.addKeyValue(EB_REPLY,repl);
    _txd.addKeyValue(EB_SRC,src);
    return _txd;
}

Cbor& EventBus::reply() {
    if(_txd.length() != 0 ) WARN (" EB.txd not cleared ");
    _txd.clear();
    uint16_t dst,src,repl;
    if ( _rxd.getKeyValue(EB_SRC,dst))
        _txd.addKeyValue(EB_DST,dst);
    if ( _rxd.getKeyValue(EB_REQUEST,repl))
        _txd.addKeyValue(EB_REPLY,repl);
    if ( _rxd.getKeyValue(EB_DST,src))
        _txd.addKeyValue(EB_SRC,src);
    return _txd;
}

void EventBus::send() {
    _queue.put(_txd);
    _txd.clear();
}

void EventBus::publish(Cbor& cbor) {
    Cbor msg(0);
    _queue.putMap(msg);
    msg.append(cbor);
    _queue.putRelease(msg);
}

EventFilter& EventBus::onAny() {
    return addFilter(EventFilter::EF_ANY,0,0);
}
//_______________________________________________________________________________________________
//
EventFilter& EventBus::filter(uint16_t key,uint16_t value) {
    return addFilter(EventFilter::EF_KV,key,value);
}
//_______________________________________________________________________________________________
//
EventFilter& EventBus::onRequest(uint16_t dst) {
    return addFilter(EventFilter::EF_REQUEST,dst,0);

}
//_______________________________________________________________________________________________
//
EventFilter& EventBus::onRequest(uint16_t dst,uint16_t req) {
    return addFilter(EventFilter::EF_REQUEST,dst,req);

}
//_______________________________________________________________________________________________
//
EventFilter& EventBus::onReply(uint16_t dst,uint16_t repl) {
    return addFilter(EventFilter::EF_REPLY,dst,repl);

}
//_______________________________________________________________________________________________
//
EventFilter& EventBus::onEvent(uint16_t src,uint16_t ev) {
    return addFilter(EventFilter::EF_EVENT,src,ev);

}

//_______________________________________________________________________________________________
//
EventFilter& EventBus::onDst(uint16_t dst) {
    return addFilter(EventFilter::EF_KV,EB_DST,dst);

}
//_______________________________________________________________________________________________
//
EventFilter& EventBus::onRemote() {
    return addFilter(EventFilter::EF_REMOTE,0,0);

}
//_______________________________________________________________________________________________
//
EventFilter& EventBus::onSrc(uint16_t src) {
    return addFilter(EventFilter::EF_KV,EB_SRC,src);

}
//_______________________________________________________________________________________________
//
bool EventBus::isEvent(uint16_t src,uint16_t ev) {
    return EventFilter::isEvent(_rxd,src,ev);
}
//_______________________________________________________________________________________________
//
bool EventBus::isReply(uint16_t src,uint16_t req) {
    return EventFilter::isReply(_rxd,src,req);
}
//_______________________________________________________________________________________________
//
bool EventBus::isReplyCorrect(uint16_t src,uint16_t req) {
    return EventFilter::isReplyCorrect(_rxd,src,req);
}
//_______________________________________________________________________________________________
//
bool EventBus::isRequest(uint16_t dst,uint16_t req) {
    return EventFilter::isRequest(_rxd,dst,req);
}


#ifdef __linux__
extern const char* hash2string(uint32_t hash);
void logCbor(Cbor& cbor) {
    Str str(2048);
    cbor.offset(0);
    uint32_t key;
    str.clear();
    Cbor::PackType ct;
    cbor.offset(0);
    while (cbor.hasData()) {
        cbor.get(key);
            if ( hash2string(key))
                str.append('"').append(hash2string(key)).append("\":");
            else
                str.append('"').append(key).append("\":");
        if (key == EB_DST || key == EB_SRC || key == EB_REQUEST || key==EB_REPLY || key==EB_EVENT ) {
            cbor.get(key);
            if ( hash2string(key))
                str.append('"').append(hash2string(key)).append("\"");
            else
                str.append('"').append(key).append("\"");

        } else {
            ct = cbor.tokenToString(str);
            if (ct == Cbor::P_BREAK || ct == Cbor::P_ERROR)
                break;
        }
        if (cbor.hasData())
            str << ",";
    };
    LOGF("--- %s", str.c_str());
}
#endif

extern void usart_send_string(const char *s);

//____________________________________________________________________
//
EventFilter* EventBus::firstFilter() {
    return _firstFilter;
}
//____________________________________________________________________
//

void EventFilter::invokeAllSubscriber(Cbor& cbor) {
    for (Subscriber* sub=firstSubscriber(); sub != 0; sub=sub->next()) {
        if (sub->_actor == 0) {
            sub->_staticHandler(cbor);
        } else {
            if (sub->_methodHandler == 0)
                sub->_actor->onEvent(cbor);
            else
                CALL_MEMBER_FUNC(sub->_actor,sub->_methodHandler)(
                    cbor);
        }
    }
}
//____________________________________________________________________
//
void EventBus::defaultHandler(Actor* actor,Cbor& msg) {
    if ( isRequest(actor->id(),H("status"))) {
        eb.reply()
        .addKeyValue(H("state"),actor->_state)
        .addKeyValue(H("timeout"),actor->_timeout)
        .addKeyValue(H("id"),actor->_id)
        .addKeyValue(H("line"),actor->_ptLine);
        eb.send();
    } else if ( isRequest(actor->id(),H("setup"))) {
    	actor->setup();
        eb.reply()
        .addKeyValue(H("state"),actor->_state)
        .addKeyValue(H("timeout"),actor->_timeout)
        .addKeyValue(H("id"),actor->_id)
        .addKeyValue(H("line"),actor->_ptLine);
        eb.send();
    } else {
    	uint16_t src=0;
    	msg.getKeyValue(EB_SRC,src);
    	eb.reply().addKeyValue(H("error"),EBADMSG)
    			.addKeyValue(H("error_msg"),"unknown event")
    			.addKeyValue(H("Actor"),actor->name())
				.addKeyValue(H("from"),src);
    	eb.send();
    }
}
//____________________________________________________________________
//

void EventBus::eventLoop() {
    while ((_queue.get(_rxd) == 0) ) { // handle all events
        for ( EventFilter* filter=firstFilter(); filter ; filter=filter->next() ) { // handle all matching filters
            if ( filter->match(_rxd))
                filter->invokeAllSubscriber(_rxd);
        }
    }

    for (Actor* actor = Actor::first(); actor; actor = actor->next()) { // handle all actor timeouts

        if (actor->timeout()) {
            _rxd=*timeoutEvent;
            actor->onEvent(_rxd);
        }
    }
}
//____________________________________________________________________
//
EventFilter& EventBus::addFilter( EventFilter::type t,uint16_t object,uint16_t value) {
    if ( _firstFilter == 0 ) {
        _firstFilter=new EventFilter(t,object,value);
        return *_firstFilter;
    } else {
        EventFilter* cursorFilter = findFilter(t,object,value);
        if ( cursorFilter==0) {
            cursorFilter = lastFilter()->_nextFilter = new EventFilter(t,object,value);
        }
        return *cursorFilter;
    }

}
//____________________________________________________________________
//
EventFilter* EventBus::findFilter( EventFilter::type t,uint16_t object,uint16_t value) {
    for( EventFilter* ef=firstFilter(); ef; ef=ef->next()) {
        if ( ef->_type == t && ef->_object==object && ef->_value == value) return ef;
    }
    return 0;
}
//_______________________________________________________________________E V E NT F I L T E R _______________________________________

EventFilter::EventFilter(EventFilter::type type, uint16_t object,uint16_t value) : _firstSubscriber(0),_nextFilter(0) {
    _type=type;
    _object=object;
    _value=value;
}
//_______________________________________________________________________________________________
//
bool EventFilter::match(Cbor& cbor) {
    if ( _type == EF_ANY ) return true;
    if ( _type == EF_EVENT ) {
        return isEvent(cbor,_object,_value);
    } else if ( _type == EF_REPLY ) {
        return isReply(cbor,_object,_value);
    } else if ( _type == EF_REQUEST ) {
        return isRequest(cbor,_object,_value);
    } else if ( _type == EF_KV ) {
        uint16_t v;
        if ( cbor.getKeyValue(_object,v)) {
            if ( (_value==v || _value==0 )   )  return true;
        } else {
            return false;
        }
    } else if ( _type==EF_REMOTE ) {
        uint16_t dst;
        if ( cbor.getKeyValue(EB_DST,dst) && (Actor::findById(dst)==0))
            return true;
    }
    return false;
}
//_______________________________________________________________________________________________
//
bool EventFilter::isEvent(Cbor& cbor ,uint16_t src,uint16_t ev) {
    uint16_t _src,_event;
    if (cbor.getKeyValue(EB_EVENT,_event) && cbor.getKeyValue(EB_SRC,_src)) {
        if ( (_event==ev || _event==0 || ev==0)  && (_src==src || _src==0 || src==0) ) return true;
    }
    return false;
}
//_______________________________________________________________________________________________
//
bool EventFilter::isReply(Cbor& cbor ,uint16_t src,uint16_t req) {
    uint16_t _src,_req;
    if (cbor.getKeyValue(EB_REPLY,_req) && cbor.getKeyValue(EB_SRC,_src)) {
        if ( (_req==req || _req==0 || req==0)  && (_src==src || _src==0 || src==0) )  return true;
    }
    return false;
}
//_______________________________________________________________________________________________
//
bool EventFilter::isReplyCorrect(Cbor& cbor ,uint16_t src,uint16_t req) {
    uint16_t _src,_req;
    uint32_t error;
    if (cbor.getKeyValue(EB_REPLY,_req) && cbor.getKeyValue(EB_SRC,_src) && cbor.getKeyValue(EB_ERROR,error)) {
        if ( (_req==req || _req==0 || req==0)  && (_src==src || _src==0 || src==0) && (error==0))  return true;
    }
    return false;
}
//_______________________________________________________________________________________________
//
bool EventFilter::isRequest(Cbor& cbor ,uint16_t dst,uint16_t req) {
    uint16_t _dst,_req;
    if (cbor.getKeyValue(EB_REQUEST,_req) && cbor.getKeyValue(EB_DST,_dst)) {
        if ( (_req==req || _req==0 || req==0)  && (_dst==dst || _dst==0 || dst==0) )  return true;
    }
    return false;
}
//________________________________________________________ S U B S C R I B E R _______________________________________
//

Subscriber* EventFilter::addSubscriber() {
    Subscriber* cursorSubscriber;

    if ( _firstSubscriber==0) {
        cursorSubscriber = _firstSubscriber=new Subscriber();
    } else {
        cursorSubscriber = lastSubscriber()->_nextSubscriber = new Subscriber();
    }
    return cursorSubscriber;
}
//_______________________________________________________________________________________________
//

//____________________________________________________________________
//
void EventFilter::subscribe(Actor* instance,
                            MethodHandler handler) {
    Subscriber* sub = addSubscriber();
    sub->_actor = instance;
    sub->_methodHandler = handler;
}
//____________________________________________________________________
//
void EventFilter::subscribe( StaticHandler handler) {
    Subscriber* sub = addSubscriber();
    sub->_staticHandler = handler;
    sub->_actor = 0;
}
//____________________________________________________________________
//
void EventFilter::subscribe(Actor* instance) {
    subscribe(instance, 0);
}

EventFilter* EventFilter::next() {
    return _nextFilter;
}

//____________________________________________________________________
//
EventFilter* EventBus::lastFilter() {
    for(EventFilter* ef=firstFilter(); ef!=0; ef=ef->next()) {
        if ( ef->_nextFilter==0 ) return ef;
    }
    ASSERT(false); // shouldn't arrive here
    return 0;
}
//____________________________________________________________________
//
Subscriber* EventFilter::firstSubscriber() {
    return _firstSubscriber;
}
Subscriber* Subscriber::next() {
    return _nextSubscriber;
}
//____________________________________________________________________
//
Subscriber* EventFilter::lastSubscriber() {
    for(Subscriber* sub=firstSubscriber(); sub; sub=sub->next()) {
        if ( sub->next() ==0 ) return sub;
    }
    ASSERT(false); // shouldn't come here
    return 0;
}


//____________________________________________________________________
//


Subscriber::Subscriber() : _nextSubscriber(0) {
}
