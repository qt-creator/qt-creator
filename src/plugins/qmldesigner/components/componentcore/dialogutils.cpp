// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <model.h>

#include <coreplugin/messagebox.h>

namespace QmlDesigner {

namespace DialogUtils {

void showWarningForInvalidId(const QString &id)
{
    constexpr char text[] = R"(
The ID <b>'%1'</b> is invalid.

Make sure the ID is:
<ul>
<li>Unique within the QML file.</li>
<li>Beginning with a lowercase letter.</li>
<li>Without any blank space or symbol.</li>
<li>Not a reserved QML keyword. </li>
</ul>
)";

    Core::AsynchronousMessageBox::warning(Model::tr("Invalid Id"),
                                          Model::tr(text).arg(id));
}

} // namespace DialogUtils

} //QmlDesigner
