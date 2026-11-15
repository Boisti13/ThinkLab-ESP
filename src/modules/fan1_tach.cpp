#include "fan1_tach.h"
#include "tach.h"

#if USE_FAN1
static Tach s_tach1;
void fan1TachBegin() { s_tach1.begin(FAN1_TACH_PIN, FAN1_PPR, FAN1_TACH_TIMEOUT_MS); }
void fan1TachTick()  { s_tach1.tick(); }
int  fan1TachGetRPM() { return s_tach1.rpm(); }
#endif
