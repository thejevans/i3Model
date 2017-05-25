#ifndef DigitalRW_Direct
#define DigitalRW_Direct

#include <Arduino.h>

inline void digitalWriteDirect(int pin, boolean val){
 if(val) g_APinDescription[pin].pPort -> PIO_SODR = g_APinDescription[pin].ulPin;
 else    g_APinDescription[pin].pPort -> PIO_CODR = g_APinDescription[pin].ulPin;
}

inline void digitalWriteDirectHigh(int pin){
 g_APinDescription[pin].pPort -> PIO_SODR = g_APinDescription[pin].ulPin;
}

inline void digitalWriteDirectLow(int pin){
g_APinDescription[pin].pPort -> PIO_CODR = g_APinDescription[pin].ulPin;
}

inline int digitalReadDirect(int pin){
 return !!(g_APinDescription[pin].pPort -> PIO_PDSR & g_APinDescription[pin].ulPin);
}

#endif
