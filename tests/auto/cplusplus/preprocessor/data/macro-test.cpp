#define USE(MY_USE) (defined MY_USE_##MY_USE && MY_USE_##MY_USE)

#define MY_USE_FEATURE1 1
#define MY_USE_FEATURE2 0

#if USE(FEATURE1)
void thisFunctionIsEnabled();
#endif

#if USE(FEATURE2)
void thisFunctionIsDisabled();
#endif

#if USE(FEATURE3)
void thisFunctionIsAlsoDisabled();
#endif

#define USE2(MY_USE) (defined MY_USE_##MY_USE)

#if USE2(FEATURE1)
void thisFunctionIsEnabled2();
#endif

#if USE2(FEATURE3)
void thisFunctionIsDisabled2();
#endif

#define USE3(MY_USE) (MY_USE_##MY_USE)

#if USE3(FEATURE1)
void thisFunctionIsEnabled3();
#endif

#if USE3(FEATURE2)
void thisFunctionIsDisabled3();
#endif
