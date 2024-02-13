%{Cpp:LicenseTemplate}\
#pragma once

#include <QCoreApplication>

namespace %{PluginName} {

struct Tr
{
    Q_DECLARE_TR_FUNCTIONS(QtC::%{PluginName})
};

} // namespace %{PluginName}
