/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#ifndef TESTTREEVIEW_H
#define TESTTREEVIEW_H

#include <utils/navigationtreeview.h>

namespace Core {
class IContext;
}

namespace Autotest {
namespace Internal {

class TestTreeView : public Utils::NavigationTreeView
{
    Q_OBJECT

public:
    TestTreeView(QWidget *parent = 0);

    void selectAll();
    void deselectAll();

private:
    void changeCheckStateAll(const Qt::CheckState checkState);
    Core::IContext *m_context;
};

} // namespace Internal
} // namespace Autotest

#endif // TESTTREEVIEW_H
