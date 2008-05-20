
add_custom_target ( doc ALL )

set ( SOURCES
  "${CMAKE_SOURCE_DIR}/lib/obex.c"
  "${CMAKE_SOURCE_DIR}/include/openobex/obex.h"
  "${CMAKE_SOURCE_DIR}/include/openobex/obex_const.h"
  "${CMAKE_SOURCE_DIR}/doc/openobex.coverpage"
)

find_package( Doxygen )
if ( DOXYGEN_FOUND )
  string ( REPLACE ";" " " DOC_SOURCE_FILES "${SOURCES}" )
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

      add_custom_target ( ${type_low}
        DEPENDS "${DOC_${type}_OUTPUT_FILE_ABS}"
      )
    endif ( DOC_${type}_OUTPUT_FILE )
  endforeach ( type )

  set_directory_properties ( PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "html;latex"
  )

  add_dependencies ( doc html )
  install ( DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html"
    DESTINATION share/doc/openobex
    COMPONENT documentation
  )
endif ( DOXYGEN_FOUND )
