// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designercontext.h"
#include "formeditor.h"

#include <QDesignerFormEditorInterface>
#include <QDesignerIntegration>

#include <QWidget>
#include <QDebug>

enum { debug = 0 };

namespace Designer {
namespace Internal {

DesignerContext::DesignerContext(const Core::Context &context,
                                 QWidget *widget, QObject *parent)
  : Core::IContext(parent)
{
    setContext(context);
    setWidget(widget);
}

void DesignerContext::contextHelp(const HelpCallback &callback) const
{
    const QDesignerFormEditorInterface *core = designerEditor();
    callback(core->integration()->contextHelpId());
}

} // namespace Internal
} // namespace Designer
