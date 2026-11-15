#include "fan2_tach.h"
#include "tach.h"

#if USE_FAN2_TACH
static Tach s_tach2;
void fan2TachBegin() { s_tach2.begin(FAN2_TACH_PIN, FAN2_PPR, FAN2_TACH_TIMEOUT_MS); }
void fan2TachTick()  { s_tach2.tick(); }
int  fan2TachGetRPM() { return s_tach2.rpm(); }
#endif
