set ( OPENOBEX_HEADERS
  openobex/obex.h
  openobex/obex_const.h
)

install ( FILES ${OPENOBEX_HEADERS}
  DESTINATION include/openobex
  COMPONENT development
)
