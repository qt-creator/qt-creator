%{Cpp:LicenseTemplate}\

@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{JS: Cpp.headerGuard('setup.h')}
#define %{JS: Cpp.headerGuard('setup.h')}
@endif

#include <QObject>
#include <QQmlEngine>

class Setup : public QObject
{
    Q_OBJECT
public:
    Setup() = default;

public slots:
    void applicationAvailable();
    void qmlEngineAvailable(QQmlEngine *engine);
    void cleanupTestCase();
};

@if 'not %{Cpp:PragmaOnce}'
#endif // %{JS: Cpp.headerGuard('setup.h')}
@endif
