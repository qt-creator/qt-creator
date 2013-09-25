#ifndef %ProjectName:h%_HPP_
#define %ProjectName:h%_HPP_

#include <QObject>

namespace bb { namespace cascades { class Application; }}

class %ProjectName:s% : public QObject
{
    Q_OBJECT
public:
    %ProjectName:s%(bb::cascades::Application *app);
    virtual ~%ProjectName:s%() {}
};

#endif /* %ProjectName:h%_HPP_ */
