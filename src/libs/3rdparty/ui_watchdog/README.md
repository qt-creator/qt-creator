# UI Watchdog

Header-only tool that monitors the main thread and breaks the program whenever
its event loop hasn't run for `MAX_TIME_BLOCKED` (default 300ms).

Currently it only breaks the program when running on Windows, but actions to perform
can be customized next to the "Add custom action here" comment.

## Example Usage

```cpp
    #include "uiwatchdog.h"

    (...)

    UiWatchdog dog;
    dog.start();

    return app.exec();
```
