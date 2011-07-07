/**************************************************************************

**

**  This  file  is  part  of  Qt  Creator

**

**  Copyright  (c)  2011  Nokia  Corporation  and/or  its  subsidiary(-ies).

**

**  Contact:  Nokia  Corporation  (info@nokia.com)

**

**  No  Commercial  Usage

**

**  This  file  contains  pre-release  code  and  may  not  be  distributed.

**  You  may  use  this  file  in  accordance  with  the  terms  and  conditions

**  contained  in  the  Technology  Preview  License  Agreement  accompanying

**  this  package.

**

**  GNU  Lesser  General  Public  License  Usage

**

**  Alternatively,  this  file  may  be  used  under  the  terms  of  the  GNU  Lesser

**  General  Public  License  version  2.1  as  published  by  the  Free  Software

**  Foundation  and  appearing  in  the  file  LICENSE.LGPL  included  in  the

**  packaging  of  this  file.   Please  review  the  following  information  to

**  ensure  the  GNU  Lesser  General  Public  License  version  2.1  requirements

**  will  be  met:  http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.

**

**  In  addition,  as  a  special  exception,  Nokia  gives  you  certain  additional

**  rights.   These  rights  are  described  in  the  Nokia  Qt  LGPL  Exception

**  version  1.1,  included  in  the  file  LGPL_EXCEPTION.txt  in  this  package.

**

**  If  you  have  questions  regarding  the  use  of  this  file,  please  contact

**  Nokia  at  info@nokia.com.

**

**************************************************************************/

#include "anchorchangesnodeinstance.h"

namespace QmlDesigner {

namespace Internal {

AnchorChangesNodeInstance::AnchorChangesNodeInstance(QDeclarativeAnchorChanges *anchorChangesObject) :
        ObjectNodeInstance(anchorChangesObject)
{
}

AnchorChangesNodeInstance::Pointer AnchorChangesNodeInstance::create(QObject *object)
{
    QDeclarativeAnchorChanges *anchorChangesObject = qobject_cast<QDeclarativeAnchorChanges*>(object);

    Q_ASSERT(anchorChangesObject);

    Pointer instance(new AnchorChangesNodeInstance(anchorChangesObject));

    instance->populateResetValueHash();

    return instance;
}

void AnchorChangesNodeInstance::setPropertyVariant(const QString &/*name*/, const QVariant &/*value*/)
{
}

void AnchorChangesNodeInstance::setPropertyBinding(const QString &/*name*/, const QString &/*expression*/)
{
}

QVariant AnchorChangesNodeInstance::property(const QString &/*name*/) const
{
    return QVariant();
}

void AnchorChangesNodeInstance::resetProperty(const QString &/*name*/)
{
}


void AnchorChangesNodeInstance::reparent(const ServerNodeInstance &/*oldParentInstance*/, const QString &/*oldParentProperty*/, const ServerNodeInstance &/*newParentInstance*/, const QString &/*newParentProperty*/)
{
}

QDeclarativeAnchorChanges *AnchorChangesNodeInstance::changesObject() const
{
    Q_ASSERT(qobject_cast<QDeclarativeAnchorChanges*>(object()));
    return static_cast<QDeclarativeAnchorChanges*>(object());
}

} // namespace Internal

} // namespace QmlDesigner
