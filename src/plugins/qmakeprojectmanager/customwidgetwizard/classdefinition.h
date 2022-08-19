// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_classdefinition.h"
#include "filenamingparameters.h"
#include "pluginoptions.h"

#include <QTabWidget>

namespace QmakeProjectManager {
namespace Internal {

class ClassDefinition : public QTabWidget
{
    Q_OBJECT

public:
    explicit ClassDefinition(QWidget *parent = nullptr);
    void setClassName(const QString &name);

    FileNamingParameters fileNamingParameters() const { return m_fileNamingParameters; }
    void setFileNamingParameters(const FileNamingParameters &fnp) { m_fileNamingParameters = fnp; }

    PluginOptions::WidgetOptions widgetOptions(const QString &className) const;

    void enableButtons();

private Q_SLOTS:
    void widgetLibraryChanged(const QString &text);
    void widgetHeaderChanged(const QString &text);
    void pluginClassChanged(const QString &text);
    void pluginHeaderChanged(const QString &text);

private:
    Ui::ClassDefinition m_ui;
    FileNamingParameters m_fileNamingParameters;
    bool m_domXmlChanged;
};

}
}
