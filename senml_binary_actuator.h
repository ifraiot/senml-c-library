#ifndef SENMLBINARYACTUATOR
#define SENMLBINARYACTUATOR

#include <senml_binary_record.h>

#define BINARY_ACTUATOR_SIGNATURE void (*callback)(const unsigned char*, int)

class SenMLBinaryActuator: public SenMLBinaryRecord
{
    friend class SenMLCborParser;
public:
    SenMLBinaryActuator(const char* name, BINARY_ACTUATOR_SIGNATURE): SenMLBinaryRecord(name, SENML_UNIT_NONE), callback(callback) {};
    SenMLBinaryActuator(const char* name, SenMLUnit unit, BINARY_ACTUATOR_SIGNATURE): SenMLBinaryRecord(name, unit), callback(callback) {};
    ~SenMLBinaryActuator(){};

protected:

    //called while parsing a senml message, when the parser found the value for an SenMLJsonListener
    virtual void actuate(const char* value, int dataLength, SenMLDataType dataType);

    //called while parsing a senml message, when the parser found the value for an SenMLJsonListener
    //the actual value has already been converted to it's appropriate type
    inline void actuate(const char* value, int length)
    {
        this->set((unsigned char*)value, length);
        this->callback((unsigned char*)value, length);
    };

private:
    BINARY_ACTUATOR_SIGNATURE;
};

#endif // SENMLBINARYACTUATOR