
add_custom_target ( doc )

set ( SOURCES
  "${CMAKE_SOURCE_DIR}/lib/obex.c"
  "${CMAKE_SOURCE_DIR}/include/openobex/obex.h"
  "${CMAKE_SOURCE_DIR}/include/openobex/obex_const.h"
  "${CMAKE_SOURCE_DIR}/doc/openobex.coverpage"
)

find_package( Doxygen )
if ( DOXYGEN_FOUND )
  string ( REPLACE ";" " " DOC_SOURCE_FILES "${SOURCES}" )
  set ( DOC_VERSION ${SHORT_VERSION} )
  set ( DOC_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} )
  set ( DOC_OUTPUT_TYPES
    HTML
    LATEX
  )
  set ( DOC_HTML_OUTPUT_FILE "index.html" )
  set ( DOC_LATEX_OUTPUT_FILE "refman.tex" )

  foreach ( type ${DOC_OUTPUT_TYPES} )
    foreach ( i ${DOC_OUTPUT_TYPES} )
      set ( DOC_${i}_OUTPUT "NO" )
    endforeach ( i )
    set ( DOC_${type}_OUTPUT "YES" )
    configure_file (
      ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
      ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile_${type}
      @ONLY
    )
    if ( DOC_${type}_OUTPUT_FILE )
      string ( TOLOWER "${type}" type_low )
      set ( DOC_${type}_OUTPUT_FILE_ABS "${CMAKE_CURRENT_BINARY_DIR}/${type_low}/${DOC_${type}_OUTPUT_FILE}" )

      add_custom_command ( OUTPUT "${DOC_${type}_OUTPUT_FILE_ABS}"
          COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile_${type}"
          DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in" ${SOURCES}
          VERBATIM
      )

      add_custom_target ( apidoc_${type_low} DEPENDS ${DOC_${type}_OUTPUT_FILE_ABS} )
    endif ( DOC_${type}_OUTPUT_FILE )
  endforeach ( type )

  set_directory_properties ( PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "html;latex"
  )
  add_dependencies ( openobex apidoc_html )
  add_dependencies ( doc apidoc_html )

  install ( DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html"
    DESTINATION ${DOCUMENTATION_INSTALL_DIR}
    COMPONENT documentation
  )
endif ( DOXYGEN_FOUND )

find_package ( Xslt )
if ( NOT XSLT_PROCESSOR )
  if ( XSLTPROC )
    set ( XSLT_PROCESSOR "xsltproc" )
  elseif ( SAXON_COMMAND )
    set ( XSLT_PROCESSOR "saxon" )
  elseif ( XALAN2_COMMAND )
    set ( XSLT_PROCESSOR "xalan2" )
  endif ( XSLTPROC )
endif ( NOT XSLT_PROCESSOR )
set ( XSLT_PROCESSOR "${XSLT_PROCESSOR}"
      CACHE STRING "XSLT processor select (xsltproc, xalan2 or saxon)" )
include ( ${Xslt_USE_FILE} )

if ( XSLT_PROCESSOR )
  if ( UNIX )
    option (BUILD_APPS_HELP_MAN "Build manpage application help files" ON)
    option (BUILD_APPS_HELP_HTML "Build HTML application help files" OFF)
  else ( UNIX )
    option (BUILD_APPS_HELP_HTML "Build HTML application help files" ON)
  endif ( UNIX )

  SET ( DOCBOOK_XSL_PREFIX "http://docbook.sourceforge.net/release/xsl/current"
        CACHE STRING "prefix to locate the docbook-XSL release files" )
  mark_as_advanced ( DOCBOOK_XSL_PREFIX )

  file ( READ "${CMAKE_CURRENT_SOURCE_DIR}/openobex-apps.xml" XML_FILE_CONTENTS )
  string ( REGEX MATCHALL "<refentrytitle>[^<]*" XML_REFENTRYTITLE "${XML_FILE_CONTENTS}" )
  foreach ( file ${XML_REFENTRYTITLE} )
    string ( REGEX REPLACE "^<refentrytitle>" "" file "${file}" )
    string ( REGEX REPLACE "[[:space:]]" "" file "${file}" )
    list ( APPEND XSLT_FILES ${file} )
  endforeach ( file )
  set ( XML_FILE_CONTENTS )
  set ( XML_REFENTRYTITLE )

  if ( BUILD_APPS_HELP_MAN )
    foreach ( file  ${XSLT_FILES} )
      list ( APPEND XSLT_FILES_MAN ${file}.1 )
    endforeach ( file )
    xsl_transform (
      "${DOCBOOK_XSL_PREFIX}/manpages/docbook.xsl"
      "${CMAKE_CURRENT_SOURCE_DIR}/openobex-apps.xml"
      ${XSLT_FILES_MAN}
    )
    foreach ( file ${XSLT_FILES_MAN} )
      list ( APPEND OPENOBEX_HELP_FILES ${file} )
      get_filename_component ( fileExt ${file} EXT )
      string ( REGEX REPLACE "^[.]" "" fileExt ${fileExt} )
      install (
	FILES ${CMAKE_CURRENT_BINARY_DIR}/${file}
	DESTINATION ${MANPAGE_INSTALL_DIR}/man${fileExt}
	PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ
                    WORLD_READ
        COMPONENT documentation
        OPTIONAL
      )
    endforeach ( file )
  endif ( BUILD_APPS_HELP_MAN )

  if ( BUILD_APPS_HELP_HTML )
    foreach ( file  ${XSLT_FILES} )
      list ( APPEND XSLT_FILES_HTML ${file}.html )
    endforeach ( file )
    list ( APPEND XSLT_FILES_HTML
      index.html
    )
    xsl_transform (
      "${DOCBOOK_XSL_PREFIX}/xhtml/chunk.xsl"
      "${CMAKE_CURRENT_SOURCE_DIR}/openobex-apps.xml"
      ${XSLT_FILES_HTML}
    )
    foreach ( file ${XSLT_FILES_HTML} )
      list ( APPEND OPENOBEX_HELP_FILES ${file} )
      INSTALL (
	FILES ${CMAKE_CURRENT_BINARY_DIR}/${file}
        DESTINATION ${DOCUMENTATION_INSTALL_DIR}
        PERMISSIONS OWNER_READ OWNER_WRITE
                    GROUP_READ
                    WORLD_READ
        COMPONENT documentation
        OPTIONAL
      )
    endforeach ( file )
  endif ( BUILD_APPS_HELP_HTML )

  add_custom_target ( openobex-apps-doc DEPENDS ${OPENOBEX_HELP_FILES} )
  add_dependencies ( openobex-apps openobex-apps-doc )
  add_dependencies ( doc openobex-apps-doc )
endif ( XSLT_PROCESSOR )
