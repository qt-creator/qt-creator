#include "%{ProjectName}.h"

#include <qul/application.h>
#include <qul/qul.h>

#include <cstdio>
#include <FreeRTOS.h>
#include <task.h>

#ifndef QUL_STACK_SIZE
#error QUL_STACK_SIZE must be defined.
#endif

static void Qul_Thread(void *argument);

int main()
{
    Qul::initPlatform();

    if (xTaskCreate(Qul_Thread, "QulExec", QUL_STACK_SIZE, 0, 4, 0) != pdPASS) {
        std::printf("Task creation failed!.\\r\\n");
        configASSERT(false);
    }

    vTaskStartScheduler();

    // Should not reach this point
    configASSERT(false);
    return 0;
}

static void Qul_Thread(void *argument)
{
    Qul::Application app;
    static %{ProjectName} item;
    app.setRootItem(&item);
    app.exec();
}
