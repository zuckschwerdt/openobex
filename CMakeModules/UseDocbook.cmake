
include ( ${Xslt_USE_FILE} )

if ( NOT DOCBOOK_XSL_VERSION )
  set ( DOCBOOK_XSL_VERSION current )
endif ( NOT DOCBOOK_XSL_VERSION )
set ( DOCBOOK_XSL_PREFIX "http://docbook.sourceforge.net/release/xsl/${DOCBOOK_XSL_VERSION}"
      CACHE STRING "prefix to locate the docbook-XSL release files" )
mark_as_advanced ( DOCBOOK_XSL_PREFIX )

function ( _DOCBOOK_GET_REFENTRY_IDS inFile outIdList )
  # This assumes that every refentry has an unique id attribute
  file ( READ "${inFile}" XML_FILE_CONTENTS )
  string ( REGEX MATCHALL "<refentry[ ]+[^>]*" XML_REFENTRYTITLE "${XML_FILE_CONTENTS}" )
  foreach ( id ${XML_REFENTRYTITLE} )
    string ( REGEX REPLACE ".*id=\"([^\"]*)\".*" "\\1" id "${id}" )
    list ( APPEND FILES ${id} )
  endforeach ( id )
  set ( ${outIdList} ${FILES} PARENT_SCOPE )
endfunction ( )

function ( _DOCBOOK_GET_MANPAGE_IDS inFile outNameList )
  file ( READ "${inFile}" XML_FILE_CONTENTS )
  string ( REPLACE ";" "" XML_FILE_CONTENTS "${XML_FILE_CONTENTS}" )
  string ( REGEX MATCHALL "<refentry[ ]+.*</refentry>" ENTRIES "${XML_FILE_CONTENTS}" )
  string ( REPLACE "</refentry>" ";" ENTRIES "${ENTRIES}" )
  list ( LENGTH ENTRIES COUNT )
  math ( EXPR COUNT "${COUNT} - 1" )
  foreach ( index RANGE ${COUNT} )
    list ( GET ENTRIES ${index} entry )
    string ( REGEX MATCH "<refname>[^<]*" MANPAGE_NAME "${entry}" )
    string ( REGEX REPLACE "^<refname>" "" MANPAGE_NAME "${MANPAGE_NAME}" )
    string ( REGEX REPLACE "[[:space:]]" "" MANPAGE_NAME "${MANPAGE_NAME}" )

    string ( REGEX MATCH "<manvolnum>[^<]*" MANPAGE_VOLUME "${entry}" )
    string ( REGEX REPLACE "^<manvolnum>" "" MANPAGE_VOLUME "${MANPAGE_VOLUME}" )
    string ( REGEX REPLACE "[[:space:]]" "" MANPAGE_VOLUME "${MANPAGE_VOLUME}" )
    if ( MANPAGE_NAME AND MANPAGE_VOLUME )
      list ( APPEND FILES "${MANPAGE_NAME}.${MANPAGE_VOLUME}" )
    endif ( MANPAGE_NAME AND MANPAGE_VOLUME )
  endforeach ( index )
  set ( ${outNameList} ${FILES} PARENT_SCOPE )
endfunction ( )

function ( _DOCBOOK_MANPAGE inFile outList )
  _docbook_get_manpage_ids ( "${inFile}" FILES )

  xsl_transform (
    "${DOCBOOK_XSL_PREFIX}/manpages/docbook.xsl"
    "${inFile}"
    ${FILES}
  )

  set ( ${outList} ${FILES} PARENT_SCOPE )
endfunction ( )

function ( _DOCBOOK_HTML inFile outList )
  _docbook_get_refentry_ids ( "${inFile}" REFTITLES )
  foreach ( file  ${REFTITLES} )
    list ( APPEND FILES ${file}.html )
  endforeach ( file )
  # This assumes that the entry point is 'reference' with
  # one or more refentries.
  list ( APPEND FILES index.html )

  set ( XSLT_PARAMS
    "use.id.as.filename=1"
  )
  xsl_transform (
    "${DOCBOOK_XSL_PREFIX}/xhtml/chunk.xsl"
    "${inFile}"
    ${FILES}
  )

  set ( ${outList} ${FILES} PARENT_SCOPE )
endfunction ( )

function ( DOCBOOK_GENERATE format inFile outList )
  if ( format STREQUAL "manpage" )
    _docbook_manpage ( "${inFile}" ${outList} )

  elseif ( format STREQUAL "html" )
    _docbook_html ( "${inFile}" ${outList} )

  else ( )
    message ( FATAL_ERROR "Unsupported docbook output format." )

  endif ( )
  set ( ${outList} ${${outList}} PARENT_SCOPE )
endfunction ( )
