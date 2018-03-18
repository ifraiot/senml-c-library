#include <senml_pack.h>
#include <senml_base.h>
#include <senml_helpers.h>
#include <senml_json_parser.h>
#include <senml_cbor_parser.h>
#include <senml_JsonStreamingParser.h>
#include <math.h>
#include <cbor.h>
#include <senml_logging.h>



//todo: inline these:
void SenMLPack::setBaseName(const char* name)
{
    this->_bn = name;
}

const char* SenMLPack::getBaseName()
{
    return this->_bn.c_str();
}

void SenMLPack::setBaseUnit(SenMLUnit unit)
{
    this->_bu = unit;
}


void SenMLPack::setBaseTime(double time)
{
    double prev = this->_bt;
    this->_bt = time;                               //set before asking children -> could be better to do it afterwards?
    SenMLBase *item = this->_start;
    while(item){
        item->adjustToBaseTime(prev, time);
        item = item->getNext();
    }
}


void SenMLPack::setLast(SenMLBase* value)
{
    if(value == this)                           //if we become the last item in the list, then the list is empty.
        this->_end = NULL;
    else
        this->_end = value;
}


bool SenMLPack::add(SenMLBase* item)
{
    if(item->getNext() != NULL){
        log_debug("item already added to list");
        return false;
    }

    SenMLBase* last = this->_end;
    if(last){
        last->setNext(item);
        item->setPrev(last);
    }
    else{
        this->_start = item;
        item->setPrev(this);
    }
    this->_end = item;
    return true;
}

bool SenMLPack::clear()
{
    SenMLBase *item = this->_start;
    while(item){
        if(item->isPack())                                          //if it's a pack element, it also needs to clear out it's children.
            ((SenMLPack*)item)->clear();
        item->setPrev(NULL);
        SenMLBase *next = item->getNext();
        item->setNext(NULL);
        item = next;
    }
    this->setNext(NULL);
    this->setPrev(NULL);
    this->_end = NULL;
    this->_start = NULL;
    return true;
}

void SenMLPack::fromJson(Stream *source, SenMLStreamMethod format)
{
    JsonStreamingParser parser;
    SenMLJsonListener listener(this);
    
    parser.setListener(&listener);
    char data;
    if(format == SENML_RAW) {
        #ifdef __MBED__
            data = source->getc();
        #else
            data = source->read();
        #endif
    }
    else{
        data = readHexChar(source);
    }
        
    while(data != -1){
        parser.parse(data); 
        if(format == SENML_RAW){
            #ifdef __MBED__
                data = source->getc();
            #else
                data = source->read();
            #endif
        }   
        else
            data = readHexChar(source);
    }
    // when we get here, all the data is stored in the document and callbacks have been called.
}

void SenMLPack::fromJson(const char *source)
{
    JsonStreamingParser parser;
    SenMLJsonListener listener(this);
    
    parser.setListener(&listener);
    for(int i = 0; source[i] != 0; i++){
        parser.parse(source[i]); 
    }
    // when we get here, all the data is stored in the document and callbacks have been called.
}

void SenMLPack::fromCbor(Stream* source, SenMLStreamMethod format)
{
    SenMLCborParser parser(this, source, format);
    parser.parse();
}


void SenMLPack::toJson(Stream *dest, SenMLStreamMethod format)
{
    StreamContext renderTo;                                              //set up the global record that configures the rendering. This saves us some bytes on the stack and in code by not having to pass along the values as function arguments.
    renderTo.stream = dest;
    renderTo.format = format;
    _streamCtx = &renderTo;

    printText("[", 1);
    this->contentToJson();
    printText("]", 1);
}


void SenMLPack::fieldsToJson()
{
    printText("\"bn\":\"", 6);
    printText(this->_bn.c_str(), this->_bn.length());
    printText("\"", 1);
    if(this->_bu){
        printText(",\"bu\":\"", 7);
        printUnit(this->_bu);
        printText("\"", 1);
    }
    if(!isnan(this->_bt)){
        printText(",\"bt\":", 6);
        printDouble(this->_bt, 16);
    }
}

void SenMLPack::contentToJson()
{
    printText("{", 1);
    this->fieldsToJson();
    SenMLBase *next = this->_start;
    if(next && next->isPack() == false){                        //we can only inline the first record. If the first item is a Pack (child device), then don't inline it.
        printText(",", 1);
        next->fieldsToJson();
        next = next->getNext();
    }
    printText("}", 1);
    while(next){
        printText(",", 1);
        next->contentToJson();
        next = next->getNext();
    }
}


void SenMLPack::toCbor(Stream *dest, SenMLStreamMethod format)
{
    StreamContext renderTo;                                              //set up the global record that configures the rendering. This saves us some bytes on the stack and in code by not having to pass along the values as function arguments.
    renderTo.stream = dest;
    renderTo.format = format;
    _streamCtx = &renderTo;

    cbor_serialize_array(this->getArrayLength());
    this->contentToCbor();
}


void SenMLPack::contentToCbor()
{
    cbor_serialize_map(this->getFieldLength());

    this->fieldsToCbor();
    SenMLBase *next = this->_start;
    if(next && next->isPack() == false){                        //we can only inline the first record. If the first item is a Pack (child device), then don't inline it.
        next->fieldsToCbor();
        next = next->getNext();
    }

    while(next){
        if(next->isPack() == false)
            next->contentToCbor();
        else
            ((SenMLPack*)next)->contentToCbor();
        next = next->getNext();
    }
}

int SenMLPack::getArrayLength()
{
    int result = 0;                             //init to 0 cause if there is a record, the first will become part of the first element in the array, if we were to init to 1, we would have 1 record too many.
    SenMLBase *next = this->_start;
    while(next){                                //we can only inline the first record. If the first item is a Pack (child device), then don't inline it.
        result += next->getArrayLength();       //custom record implementations may wrap multiple records.
        next = next->getNext();
    }
    if(result == 0)                             //if there are no items in this pack, then we still render 1 array element, that of the pack itself.
        result = 1;
    return result;
}


int SenMLPack::getFieldLength()
{
    int result = 1;                             //always render the base name
    if(this->_bu) result++;
    if(!isnan(this->_bt)) result++;

    SenMLBase *next = this->_start;
    if(next && next->isPack() == false){        //we can only inline the first record. If the first item is a Pack (child device), then don't inline it.
        result += next->getFieldLength();
        next = next->getNext();
    }
    return result;
}

void SenMLPack::fieldsToCbor()
{
    cbor_serialize_int(SENML_CBOR_BN_LABEL);
    cbor_serialize_unicode_string(this->_bn.c_str());
    if(this->_bu){
        cbor_serialize_int(SENML_CBOR_BU_LABEL);
        cbor_serialize_unicode_string(senml_units_names[this->_bu]);
    }
    if(!isnan(this->_bt)){
        cbor_serialize_int(SENML_CBOR_BT_LABEL);
        cbor_serialize_double(this->_bt);
    }
}