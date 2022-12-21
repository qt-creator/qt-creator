// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filenamingparameters.h"
#include "pluginoptions.h"

#include <QTabWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class QTextEdit;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

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
    FileNamingParameters m_fileNamingParameters;
    bool m_domXmlChanged;

    Utils::PathChooser *m_iconPathChooser;
    QRadioButton *m_libraryRadio;
    QCheckBox *m_skeletonCheck;
    QLabel *m_widgetLibraryLabel;
    QLineEdit *m_widgetLibraryEdit;
    QLabel *m_widgetSourceLabel;
    QLineEdit *m_widgetSourceEdit;
    QLabel *m_widgetBaseClassLabel;
    QLineEdit *m_widgetBaseClassEdit;
    QLabel *m_widgetProjectLabel;
    QLineEdit *m_widgetProjectEdit;
    QLineEdit *m_widgetHeaderEdit;
    QLineEdit *m_pluginClassEdit;
    QLineEdit *m_pluginSourceEdit;
    QLineEdit *m_pluginHeaderEdit;
    QLineEdit *m_groupEdit;
    QLineEdit *m_tooltipEdit;
    QTextEdit *m_whatsthisEdit;
    QCheckBox *m_containerCheck;
    QTextEdit *m_domXmlEdit;
};

}
}
