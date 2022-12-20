add_qtc_executable(cpaster
  DEPENDS Core Utils Qt::Network
  SOURCES
    argumentscollector.cpp argumentscollector.h
    main.cpp
    ../dpastedotcomprotocol.cpp ../dpastedotcomprotocol.h
    ../pastebindotcomprotocol.cpp ../pastebindotcomprotocol.h
    ../protocol.cpp ../protocol.h
    ../urlopenprotocol.cpp ../urlopenprotocol.h
)
