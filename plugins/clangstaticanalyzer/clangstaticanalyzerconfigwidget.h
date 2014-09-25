/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#ifndef CLANGSTATICANALYZERCONFIGWIDGET_H
#define CLANGSTATICANALYZERCONFIGWIDGET_H

#include "clangstaticanalyzersettings.h"

#include <QWidget>

namespace ClangStaticAnalyzer {
namespace Internal {

namespace Ui { class ClangStaticAnalyzerConfigWidget; }

class ClangStaticAnalyzerConfigWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ClangStaticAnalyzerConfigWidget(ClangStaticAnalyzerSettings *settings,
                                             QWidget *parent = 0);
    ~ClangStaticAnalyzerConfigWidget();

private:
    Ui::ClangStaticAnalyzerConfigWidget *m_ui;
    ClangStaticAnalyzerSettings *m_settings;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERCONFIGWIDGET_H
