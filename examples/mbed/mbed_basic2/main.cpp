#include "mbed.h"
#include <kpn_senml.h>

Serial pc(USBTX, USBRX);
SenMLPack doc("deviId");
SenMLFloatRecord temperature("temp", SENML_UNIT_DEGREES_CELSIUS);
SenMLBoolRecord doorPos("doorPos");
SenMLStringRecord strVal("str val");

int main() {
    senMLSetLogger(&pc);
    doc.setBaseName("basename");
    doc.setBaseUnit(SENML_UNIT_EVENT_RATE_PER_SECOND);
    doc.setBaseTime(1.0);
    doc.add(&temperature);
    doc.add(&doorPos);
    doc.add(&strVal);
    pc.printf("start \n\r");

    while(1) {
        temperature.set(20.5000112345, 2.0);
        doorPos.set(true);
        strVal.set("test");
        doc.toJson(&pc);
        pc.printf("\n\r");
        wait(1);
    }
}
