@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef @WIDGET_INCLUDE_GUARD@
#define @WIDGET_INCLUDE_GUARD@
@endif

#include <@WIDGET_BASE_CLASS@>

class @WIDGET_CLASS@ : public @WIDGET_BASE_CLASS@
{
    Q_OBJECT

public:
    @WIDGET_CLASS@(QWidget *parent = 0);
};

@if ! '%{Cpp:PragmaOnce}'
#endif // @WIDGET_INCLUDE_GUARD@
@endif
