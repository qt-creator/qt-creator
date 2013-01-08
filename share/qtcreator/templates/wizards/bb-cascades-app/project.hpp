#ifndef %ProjectName%_HPP_
#define %ProjectName%_HPP_

#include <QObject>

namespace bb { namespace cascades { class Application; }}

class %ProjectName% : public QObject
{
    Q_OBJECT
public:
    %ProjectName%(bb::cascades::Application *app);
    virtual ~%ProjectName%() {}
};

#endif /* %ProjectName%_HPP_ */
