#
# call as XSLT_TRANSFORM(<.xsl file URL> <input file> <output files...>)
#
function ( XSL_TRANSFORM xslurl infile )
  if ( XSLT_PROCESSOR STREQUAL "xsltproc" )
    if ( XSLTPROC )
      set ( XSL_TRANSFORM_COMMAND
	${XSLTPROC}
	--param use.id.as.filename 1
	${xslurl} ${infile}
      )
    else ( XSLTPROC )
      message ( FATAL_ERROR "xsltproc not found" )
    endif ( XSLTPROC )

  elseif ( XSLT_PROCESSOR STREQUAL "saxon" )
    if ( SAXON_COMMAND )
      set ( XSL_TRANSFORM_COMMAND
	${SAXON_COMMAND}
	${infile} ${xslurl}
	use.id.as.filename=1
      )
    else ( SAXON_COMMAND )
      message ( FATAL_ERROR "Saxon-6.5.x not found" )
    endif ( SAXON_COMMAND )

  elseif ( XSLT_PROCESSOR STREQUAL "xalan2" )
    # Xalan places the output in the source file's directory :-(
    get_filename_component ( infile_name "${infile}" NAME )
    add_custom_command (
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${infile_name}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${infile}" "${CMAKE_CURRENT_BINARY_DIR}/${infile_name}"
      DEPENDS ${infile}
      VERBATIM
    )
    set ( infile "${CMAKE_CURRENT_BINARY_DIR}/${infile_name}" )
    if ( XALAN2_COMMAND )
      set ( XSL_TRANSFORM_COMMAND
	${XALAN2_COMMAND}
	-param use.id.as.filename 1
	-in ${infile} -xsl ${xslurl}
      )
    else ( XALAN2_COMMAND )
      message ( FATAL_ERROR " Xalan 2.x not found" )
    endif ( XALAN2_COMMAND )

  else ( XSLT_PROCESSOR STREQUAL "xsltproc" )
    message ( FATAL_ERROR "Unsupported XSLT processor" )
  endif ( XSLT_PROCESSOR STREQUAL "xsltproc" )

  add_custom_command (
    OUTPUT ${ARGN}
    COMMAND ${XSL_TRANSFORM_COMMAND}
    DEPENDS ${infile}
    VERBATIM
  )
  set ( XSL_TRANSFORM_COMMAND )
endfunction ( XSL_TRANSFORM )
