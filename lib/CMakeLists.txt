#
# Main library version and shared object version
#
set ( OPENOBEX_LIB_NAME      "openobex" )
foreach ( i VERSION VERSION_MAJOR VERSION_MINOR VERSION_PATCH )
  set ( OPENOBEX_LIB_${i}   "${${i}}" )
endforeach ( i )
set ( OPENOBEX_LIB_SOVERSION "1" )


set ( SOURCES
  obex.c
  obex_client.c
  obex_connect.c
  obex_header.c
  obex_main.c
  obex_object.c
  obex_server.c
  obex_transport.c
  databuffer.c
  inobex.c
)

set ( HEADERS
  obex_client.h
  obex_connect.h
  obex_header.h
  obex_main.h
  obex_object.h
  obex_server.h
  obex_transport.h
  databuffer.h
  inobex.h
)

option ( ENABLE_DEBUG_MESSAGES "Enables debug messages printed during runtime" )
if ( ENABLE_DEBUG_MESSAGES )
  add_definitions ( -DOBEX_DEBUG )
  if ( NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" )
     option ( DEBUG_USE_SYSLOG "Use SysLog facility instead of stderr for debug messages" )
  endif ( NOT CMAKE_SYSTEM_NAME STREQUAL "Windows" )
  if ( DEBUG_USE_SYSLOG )     
    add_definitions ( -DOBEX_SYSLOG )
  endif ( DEBUG_USE_SYSLOG )     
endif ( ENABLE_DEBUG_MESSAGES )

option ( ENABLE_DUMP_MESSAGES "Enables dumping of data" )
if ( ENABLE_DUMP_MESSAGES )
  add_definitions ( -DOBEX_DUMP )
endif ( ENABLE_DUMP_MESSAGES )

if ( OPENOBEX_IRDA )
  list ( APPEND SOURCES
    irobex.c
  )
  list ( APPEND HEADERS
    irobex.h
    irda.h
    irda_wrap.h
  )
endif ( OPENOBEX_IRDA )

if ( OPENOBEX_BLUETOOTH )
  list ( APPEND SOURCES
    btobex.c
  )
  list ( APPEND HEADERS
    btobex.h
    bluez_compat.h
  )
endif ( OPENOBEX_BLUETOOTH )

if ( OPENOBEX_USB )
  list ( APPEND ${OPENOBEX_LIB_NAME}_LIBRARIES
    ${LibUSB_LIBRARIES}
  )
  include_directories ( BEFORE SYSTEM ${LibUSB_INCLUDE_DIRS} )
  list ( APPEND SOURCES
    usbobex.c
  )
  list ( APPEND HEADERS
    usbobex.h
  )
endif ( OPENOBEX_USB )

if ( COMPILER_SUPPORT_VISIBILITY )
  set ( ${OPENOBEX_LIB_NAME}_DEFINITIONS
    "${${OPENOBEX_LIB_NAME}_DEFINITIONS} -DHAVE_VISIBILITY ${COMPILER_FLAG_VISIBILITY}"
  )
endif ( COMPILER_SUPPORT_VISIBILITY )
if ( COMPILER_SUPPORT_NOUNDEFINED )
  set ( ${OPENOBEX_LIB_NAME}_LDFLAGS
    "${${OPENOBEX_LIB_NAME}_LDFLAGS} ${LINKER_FLAG_NOUNDEFINED}"
  )
endif ( COMPILER_SUPPORT_NOUNDEFINED )

if ( WIN32 )
  if ( CMAKE_COMPILER_IS_GNUCC )
    set ( ${OPENOBEX_LIB_NAME}_LDFLAGS
      "${${OPENOBEX_LIB_NAME}_LDFLAGS} -Wl,--disable-stdcall-fixup -Wl,--add-stdcall-alias"
    )
  endif ( CMAKE_COMPILER_IS_GNUCC )

  list ( APPEND ${OPENOBEX_LIB_NAME}_LIBRARIES
    ws2_32
  )

  if ( CMAKE_RC_COMPILER )
    set ( OPENOBEX_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/openobex.rc" )
    configure_file (
      "${CMAKE_CURRENT_SOURCE_DIR}/openobex.rc.in"
      "${OPENOBEX_RC_FILE}"
      @ONLY
    )
  endif ( CMAKE_RC_COMPILER )

  if ( MSVC )
    set ( OPENOBEX_DEF_FILE "${CMAKE_CURRENT_BINARY_DIR}/openobex.def" )
    file ( WRITE "${OPENOBEX_DEF_FILE}" "VERSION ${OPENOBEX_LIB_VERSION_MAJOR}.${OPENOBEX_LIB_VERSION_MINOR}\n" )
    file ( APPEND "${OPENOBEX_DEF_FILE}" "EXPORTS\n" )
    file ( READ "${CMAKE_CURRENT_SOURCE_DIR}/obex.sym" OPENOBEX_SYMBOLS )
    file ( APPEND "${OPENOBEX_DEF_FILE}" "${OPENOBEX_SYMBOLS}\n" )
  endif ( MSVC )
endif ( WIN32 )

if ( CYGWIN )
  #also define _WIN32 under CygWin
  add_definitions( -D_WIN32)
endif ( CYGWIN )

# Add the openobex library target
add_library ( ${OPENOBEX_LIB_NAME}
  ${SOURCES}
  ${HEADERS}
  ${OPENOBEX_RC_FILE}
  ${OPENOBEX_DEF_FILE}
)
set_target_properties ( ${OPENOBEX_LIB_NAME} PROPERTIES
  VERSION       ${OPENOBEX_LIB_VERSION}
  SOVERSION     ${OPENOBEX_LIB_SOVERSION}
  DEFINE_SYMBOL OPENOBEX_EXPORTS
  COMPILE_FLAGS "${${OPENOBEX_LIB_NAME}_DEFINITIONS}"
  LINK_FLAGS    "${${OPENOBEX_LIB_NAME}_LDFLAGS}"
)
target_link_libraries ( ${OPENOBEX_LIB_NAME}
  ${${OPENOBEX_LIB_NAME}_LIBRARIES}
)

install ( TARGETS ${OPENOBEX_LIB_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  COMPONENT library
)

#
# Copy the .pc file to install it only if the lib gets installed
#
add_custom_command ( TARGET ${OPENOBEX_LIB_NAME}
  COMMAND ${CMAKE_COMMAND}
  ARGS    -E copy_if_different ${PROJECT_BINARY_DIR}/${OPENOBEX_LIB_NAME}.pc
          ${CMAKE_CURRENT_BINARY_DIR}/${OPENOBEX_LIB_NAME}.pc
  VERBATIM
)
install ( FILES ${CMAKE_CURRENT_BINARY_DIR}/${OPENOBEX_LIB_NAME}.pc
  DESTINATION ${PKGCONFIG_INSTALL_DIR}
  OPTIONAL
)


#  include ( GetPrerequisites )
#  get_prerequisites ( ${OPENOBEX_LIB_NAME} CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS 0 1 )
include ( InstallRequiredSystemLibraries )

