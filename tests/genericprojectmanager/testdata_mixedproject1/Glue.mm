#include "Glue.h"

@class NSApp;

void Glue::it::together()
{
    [[NSApp dockTile] setContentView:nil];
}
