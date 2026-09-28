#include <zephyr/kernel.h>

int main(void)
{
    const char *board = CONFIG_BOARD_TARGET;

    printk("Hello World from %s\\n", board);
    return 0;
}
