option ( USE_MAINTAINER_MODE "Enable some stuff only relevant to developers" OFF )
if ( USE_MAINTAINER_MODE )
  if ( CMAKE_COMPILER_IS_GNUCC )
    set ( MAINTAINER_MODE_WARN_FLAGS
      all
      extra
      no-unused-parameter
      no-missing-field-initializers
      declaration-after-statement
      missing-declarations
      redundant-decls
      cast-align
      error
    )
    foreach ( flag ${MAINTAINER_MODE_WARN_FLAGS} )
      set ( cflag "-W${flag}" )
      set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}" )
      set ( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} ${cflag}" )
      set ( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} ${cflag}" )
      set ( CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS} ${cflag}" )
      set ( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} ${cflag}" )      
    endforeach ( flag )

  elseif ( MSVC )
    set ( MAINTAINER_MODE_WARN_FLAGS
      W3
      WX
    )
    foreach ( flag ${MAINTAINER_MODE_WARN_FLAGS} )
      set ( cflag "/${flag}" )
      set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${cflag}" )
      set ( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} ${cflag}" )
      set ( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} ${cflag}" )
      set ( CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS} ${cflag}" )
      set ( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS} ${cflag}" )      
    endforeach ( flag )
  endif ( CMAKE_COMPILER_IS_GNUCC )
endif ( USE_MAINTAINER_MODE )
