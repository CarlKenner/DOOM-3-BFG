cmake_minimum_required(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(Doom3BFGVR)

if( COMMAND cmake_policy )
	if( POLICY CMP0011 )
		cmake_policy( SET CMP0011 NEW )
	endif( POLICY CMP0011 )
	if( POLICY CMP0054 )
		cmake_policy( SET CMP0054 NEW )
	endif( POLICY CMP0054 )
	if( POLICY CMP0003 )
		cmake_policy( SET CMP0003 NEW )
	endif( POLICY CMP0003 )
endif( COMMAND cmake_policy )

if( CMAKE_SIZEOF_VOID_P MATCHES "8" )
	set( X64 64 )
	set( X32_64 64 )
	set( WIN32_WIN64 win64 )
	set( WIN32_X64 x64 )
else( CMAKE_SIZEOF_VOID_P MATCHES "8" )
	set( X32_64 32 )
	set( WIN32_WIN64 win32 )
	set( WIN32_X64 win32 )
endif( CMAKE_SIZEOF_VOID_P MATCHES "8" )

list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../doomclassic )
include( CreateLaunchers )
include( FindPackageHandleStandardArgs )

option( NO_GENERATOR_EXPRESSIONS "Disable generator expressions (for building pk3s with IDEs)." OFF )

# Simplify pk3 building, add_pk3(filename srcdirectory)
function( add_pk3 PK3_NAME PK3_DIR )
	# Generate target name. Just use "pk3" for main pk3 target.
	string( REPLACE "." "_" PK3_TARGET ${PK3_NAME} )
	if( ${PK3_TARGET} STREQUAL "zdoom_pk3" )
		set( PK3_TARGET "pk3" )
	endif( ${PK3_TARGET} STREQUAL "zdoom_pk3" )

	if( NOT NO_GENERATOR_EXPRESSIONS AND NOT ZDOOM_OUTPUT_OLDSTYLE )
		add_custom_command( OUTPUT ${ZDOOM_OUTPUT_DIR}/${PK3_NAME}
			COMMAND zipdir -udf ${ZDOOM_OUTPUT_DIR}/${PK3_NAME} ${PK3_DIR}
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ZDOOM_OUTPUT_DIR}/${PK3_NAME} $<TARGET_FILE_DIR:Doom3BFGVR>/${PK3_NAME}
			DEPENDS zipdir )
	else( NOT NO_GENERATOR_EXPRESSIONS AND NOT ZDOOM_OUTPUT_OLDSTYLE )
		add_custom_command( OUTPUT ${ZDOOM_OUTPUT_DIR}/${PK3_NAME}
			COMMAND zipdir -udf ${ZDOOM_OUTPUT_DIR}/${PK3_NAME} ${PK3_DIR}
			DEPENDS zipdir )
	endif( NOT NO_GENERATOR_EXPRESSIONS AND NOT ZDOOM_OUTPUT_OLDSTYLE )

	if( NOT NO_GENERATOR_EXPRESSIONS )
		# Touch the zipdir executable here so that the pk3s are forced to
		# rebuild each time since their dependecy has "changed."
		add_custom_target( ${PK3_TARGET} ALL
			COMMAND ${CMAKE_COMMAND} -E touch $<TARGET_FILE:zipdir>
			DEPENDS ${ZDOOM_OUTPUT_DIR}/${PK3_NAME} )
	endif( NOT NO_GENERATOR_EXPRESSIONS )

	install(FILES ${ZDOOM_OUTPUT_DIR}/${PK3_NAME} DESTINATION ".")
endfunction( add_pk3 )

# Macro for building libraries without debugging information
macro( make_release_only )
	set( CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_RELEASE} )
	set( CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELEASE} )
	string( REPLACE "/MT " "/MTd " CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_RELEASE} )
	set( CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_RELEASE} )
	set( CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELEASE} )
	string( REPLACE "/MT " "/MTd " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_RELEASE} )
endmacro( make_release_only )

option(USE_MFC_TOOLS
		"Compile the built-in MFC based GZ3Doom tools" OFF)
		
option(MONOLITH
		"Embed GZ3Doom game logic into main executable" ON)
		

option( NO_FMOD "Disable FMODEx sound support" OFF )
option( NO_OPENAL "Disable OpenAL sound support" OFF )

set( ZDOOM_OUTPUT_DIR ${CMAKE_BINARY_DIR} CACHE PATH "Directory where zdoom.pk3 and the executable will be created." )
set( ZDOOM_EXE_NAME "GZ3Doom" CACHE FILEPATH "Name of the executable to create" )

if( MSVC )
	# Eliminate unreferenced functions and data
	# Perform identical COMDAT folding
	set( REL_LINKER_FLAGS "/opt:ref /opt:icf /nodefaultlib:msvcrt /TSAWARE /LARGEADDRESSAWARE" )
endif( MSVC )	

set( BZIP2_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/bzip2" )
set( BZIP2_LIBRARIES bz2 )
set( BZIP2_LIBRARY bz2 )
set( ZLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib;${CMAKE_CURRENT_SOURCE_DIR} )
#set( ZLIB_LIBRARIES z )
#set( ZLIB_LIBRARY z )
set( LZMA_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/lzma/C" )
set( GME_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/game-music-emu" )
set( GME_LIBRARIES gme )

option(SDL2
		"Use SDL2 instead of SDL1.2" ON)
		
option(OPENAL
		"Use OpenAL soft instead of XAudio2" OFF)

option(FFMPEG
		"Use FMPEG to render Bink videos" ON)

option(ONATIVE
		"Optimize for the host CPU" OFF)
		
option(WINRT
		"Build for Windows RT" OFF)
		
option(USE_SYSTEM_ZLIB
                "Use the system zlib instead of the bundled one" OFF)

option(USE_SYSTEM_LIBPNG
                "Use the system libpng instead of the bundled one" OFF)

option(USE_SYSTEM_LIBJPEG
                "Use the system libjpeg instead of the bundled one" OFF)

option(USE_SYSTEM_LIBGLEW
                "Use the system libglew instead of the bundled one" OFF)

if(UNIX)
	set(OPENAL TRUE)
endif()
		
if(MSVC)
	#message(STATUS CMAKE_ROOT: ${CMAKE_ROOT})
	
	#SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/../bin/${WIN32_WIN64})
	
	#message(STATUS EXECUTABLE_OUTPUT_PATH: ${EXECUTABLE_OUTPUT_PATH})
	#message(STATUS PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR})
	#message(STATUS CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR})
else()
	message(STATUS CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE})
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	add_definitions(-pipe)
	#add_definitions(-Wall)
	add_definitions(-Werror=format-security)
	add_definitions(-Werror=format)
	add_definitions(-mmmx -msse -msse2)
	if(WIN32)
		# require msvcr70.dll or newer for _aligned_malloc etc
		# I think it is from Visual C++ .NET 2002, so it should be available on any remotely modern system.
		add_definitions(-D__MSVCRT_VERSION__=0x0700)
		
		find_package(DirectX REQUIRED)
		include_directories(libs/mingw-hacks)
		include_directories(${DirectX_INCLUDE_DIR})
		add_definitions(-DUSE_XINPUT)
	endif()	
	
	#if(NOT ANDROID)
		add_definitions(-DUSE_EXCEPTIONS)
	#endif()
	
	# the warnings are used for every profile anyway, so put them in a variable
	set(my_warn_flags "-Wno-pragmas -Wno-unused-variable -Wno-unused-but-set-variable -Wno-switch -Wno-unused-value -Winvalid-pch -Wno-multichar")
	
	if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
		# append clang-specific settings for warnings (the second one make sure clang doesn't complain
		# about unknown -W flags, like -Wno-unused-but-set-variable)
		set(my_warn_flags "${my_warn_flags} -Wno-local-type-template-args -Wno-unknown-warning-option -Wno-inline-new-delete")
	endif()
	
	if(NOT CMAKE_CROSSCOMPILING AND ONATIVE)
		add_definitions(-march=native)
	endif()

	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG -O0 -ggdb -fno-strict-aliasing ${my_warn_flags}")
	#set(CMAKE_C_FLAGS_DEBUGALL "${CMAKE_C_FLAGS_DEBUGALL} -g -ggdb -D_DEBUG -fno-strict-aliasing ${my_warn_flags}")
	#set(CMAKE_C_FLAGS_PROFILE "${CMAKE_C_FLAGS_PROFILE} -g -ggdb -D_DEBUG -O1 -fno-omit-frame-pointer -fno-strict-aliasing ${my_warn_flags}")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -ffast-math -fno-unsafe-math-optimizations -fomit-frame-pointer -fno-strict-aliasing ${my_warn_flags}")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g -O3 -ffast-math -fno-unsafe-math-optimizations -fomit-frame-pointer -fno-strict-aliasing ${my_warn_flags}")
	set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -ffast-math -fno-unsafe-math-optimizations -fomit-frame-pointer -fno-strict-aliasing ${my_warn_flags}")
	
	set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
	#set(CMAKE_CXX_FLAGS_DEBUGALL ${CMAKE_C_FLAGS_DEBUGALL})
	#set(CMAKE_CXX_FLAGS_PROFILE ${CMAKE_C_FLAGS_PROFILE})
	set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
	set(CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})

elseif(MSVC)
	
	# Multiprocessor support for compiling
	# /MP
	
	# /MT  use multithreaded, static runtime library
	#      (=> binary won't need MSVCP100D.dll or similar)
	# /MTd the same with debug symbols
	# /MD and /MDd - the same with dynamic runtime lib
	#     (=> MSVCP100D.dll or similar is needed to run binary)

	# Inline Function Expansion: Any Suitable (/Ob2)
	# "/Ob2",
			
	# enable Intrinsic Functions
	# "/Oi",
			
	# Omit Frame Pointers
	# "/Oy",
	
	# if(MSVC_VERSION EQUAL 1700)
		# #message(STATUS "$ENV{LIBPATH}")
		
		# find_path(Windows_winmd_DIR NAMES Windows.winmd
			# HINTS
			# C:/Program Files (x86)/Windows Kits/8.0/References/CommonConfiguration/Neutral
			# PATH_SUFFIXES "Neutral")
			
		# message(STATUS "${Windows_winmd_DIR}")
		
		# #set(WINRT_OPTIONS "/ZW /D USE_WINRT /AI \"C:\Program Files (x86)\Windows Kits\8.0\References\CommonConfiguration\Neutral\"")
		# #set(WINRT_OPTIONS "/ZW /D USE_WINRT /AI \"$ENV{LIBPATH}\" ")
		# set(WINRT_OPTIONS "/ZW /D USE_WINRT")
	
		# set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP ${WINRT_OPTIONS}")
		# set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy ${WINRT_OPTIONS}")
		# set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi ${WINRT_OPTIONS}")
		# set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy ${WINRT_OPTIONS}")
		
		# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP ${WINRT_OPTIONS}")
		# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy ${WINRT_OPTIONS}")
		# set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP ${WINRT_OPTIONS}")
		# set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy ${WINRT_OPTIONS}")
	# else()
		set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D _DEBUG /MP /MTd")
		set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MP /Oi /Oy /MT")
		set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /MP /Oi /MTd")
		set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} /MP /Oi /Oy /MT")
		
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP /MTd")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MP /Oi /Oy /MT")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MP /MTd")
		set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MP /Oi /Oy /MT")
		
		# RB: without /SAFESEH:NO we can't link against ffmpeg libs and VS2013 or we have to build our own
		# libs for newer compilers
		set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${REL_LINKER_FLAGS} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL} ${REL_LINKER_FLAGS} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${REL_LINKER_FLAGS} /SAFESEH:NO")
		set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} ${REL_LINKER_FLAGS} /SAFESEH:NO")
	# endif()
	

	add_definitions(-DWIN32 
					-D_WINDOWS
					-D_CRT_SECURE_NO_DEPRECATE
					-D_CRT_NONSTDC_NO_DEPRECATE
					-D_CRT_SECURE_NO_WARNINGS
                                        -D_MBCS
                                       #-DUSE_OPENAL
										-DNO_OPENAL=1
					-DUSE_EXCEPTIONS)
        ## Check for Version ##
        if( WINRT ) # Windows RT
                add_definitions(-DUSE_WINRT)
        endif()
					
	if(NOT CMAKE_CL_64)
		add_definitions(-D_USE_32BIT_TIME_T)
	endif()
					
	if(USE_MFC_TOOLS)
		add_definitions(-D_AFXDLL)
		
		# 0: Use Standard Windows Libraries
		# 1: Use MFC in a Static Library
		# 2: Use MFC in a Shared DLL 
		set(CMAKE_MFC_FLAG 2)
	endif()
endif()

#if(STANDALONE)
#	add_definitions(-DSTANDALONE)
#endif()
add_definitions(-DHAVE_OCULUS_API)

if (USE_SYSTEM_ZLIB)
  find_package(ZLIB REQUIRED)
endif(USE_SYSTEM_ZLIB)

if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    set(ZLIB_LIBRARY ${ZLIB_LIBRARIES})
else (ZLIB_FOUND)
    include_directories("libs/zlib")
    set(ZLIB_LIBRARY "" )
endif (ZLIB_FOUND)

if(USE_SYSTEM_LIBPNG)
find_package(PNG REQUIRED)
endif (USE_SYSTEM_LIBPNG)

if (PNG_FOUND)
    include_directories(${PNG_INCLUDE_DIRS})
    set(PNG_LIBRARY ${PNG_LIBRARIES})
else (PNG_FOUND)
    include_directories("libs/png")
    set(PNG_LIBRARY "" )
endif (PNG_FOUND)


if(USE_SYSTEM_LIBJPEG)
find_package(JPEG REQUIRED)
endif(USE_SYSTEM_LIBJPEG)

if (JPEG_FOUND)
    include_directories(${JPEG_INCLUDE_DIRS})
    set(JPEG_LIBRARY ${JPEG_LIBRARIES})
    add_definitions(-DUSE_NEWER_JPEG)
else (JPEG_FOUND)
    include_directories("libs/jpeg-9b")
    set(JPEG_LIBRARY "" )
    add_definitions(-DUSE_NEWER_JPEG)
endif (JPEG_FOUND)

if (USE_SYSTEM_LIBGLEW)
find_package(GLEW)
endif(USE_SYSTEM_LIBGLEW)

if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    set(GLEW_LIBRARY ${GLEW_LIBRARIES})
else (GLEW_FOUND)
    include_directories("libs/glew/include")
    set(GLEW_LIBRARY "" )
    add_definitions(-DGLEW_STATIC)
endif (GLEW_FOUND)

if(WIN32)
	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openvr/lib/${WIN32_WIN64})
	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/fmod/lib)
endif(WIN32)

if(UNIX)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openvr/lib/linux32)
	else()
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openvr/lib/linux64)
	endif()
endif(UNIX)

if(APPLE)
	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openvr/lib/osx32)
endif(APPLE)

set(OPENVR_LIBRARY "openvr_api")
include_directories("libs/openvr/headers")
if(WIN32)
set(FMOD_LIBRARY "fmodex${X64}_vc")
endif(WIN32)
include_directories("libs/fmod/inc")

add_subdirectory(idlib)

file(GLOB AAS_INCLUDES aas/*.h)
file(GLOB AAS_SOURCES aas/*.cpp)

file(GLOB_RECURSE CM_INCLUDES cm/*.h)
file(GLOB_RECURSE CM_SOURCES cm/*.cpp)

file(GLOB FRAMEWORK_INCLUDES framework/*.h)
file(GLOB FRAMEWORK_SOURCES framework/*.cpp)

file(GLOB FRAMEWORK_ASYNC_INCLUDES framework/async/*.h)
file(GLOB FRAMEWORK_ASYNC_SOURCES framework/async/*.cpp)

file(GLOB RENDERER_INCLUDES renderer/*.h)
	# renderer/AutoRender.h
	# renderer/AutoRenderBink.h
	# renderer/BinaryImage.h
	# renderer/BinaryImageData.h
	# renderer/BoundsTrack.h
	# renderer/BufferObject.h
	# renderer/Cinematic.h
	# renderer/Font.h
	# renderer/GLMatrix.h
	# renderer/GLState.h
	# renderer/GuiModel.h
	# renderer/Image.h
	# renderer/ImageOpts.h
	# renderer/Interaction.h
	# renderer/Material.h
	# renderer/Model.h
	# renderer/ModelDecal.h
	# renderer/ModelManager.h
	# renderer/ModelOverlay.h
	# renderer/Model_ase.h
	# #renderer/Model_ColladaHelper.h
	# #renderer/Model_ColladaParser.h
	# renderer/Model_local.h
	# renderer/Model_lwo.h
	# renderer/Model_ma.h
	# renderer/Model_md3.h
	# renderer/RenderLog.h
	# renderer/RenderProgs.h
	# renderer/RenderSystem.h
	# renderer/RenderTexture.h
	# renderer/RenderWorld.h
	# renderer/RenderWorld_local.h
	# renderer/ResolutionScale.h
	# renderer/ScreenRect.h
	# renderer/simplex.h
	# renderer/tr_local.h
	# renderer/VertexCache.h)


file(GLOB RENDERER_SOURCES renderer/*.cpp)
	# renderer/AutoRender.cpp
	# renderer/Cinematic.cpp
	# renderer/Framebuffer.cpp
	# renderer/GLShader.cpp
	# renderer/GuiModel.cpp
	# renderer/Image_files.cpp
	# renderer/Image_init.cpp
	# renderer/Image_load.cpp
	# renderer/Image_process.cpp
	# renderer/Image_program.cpp
	# renderer/Interaction.cpp
	# renderer/Material.cpp
	# renderer/Model.cpp
	# renderer/ModelDecal.cpp
	# renderer/ModelManager.cpp
	# renderer/ModelOverlay.cpp
	# renderer/Model_ase.cpp
	# renderer/Model_beam.cpp
	# #renderer/Model_ColladaParser.cpp
	# renderer/Model_liquid.cpp
	# renderer/Model_lwo.cpp
	# renderer/Model_ma.cpp
	# renderer/Model_md3.cpp
	# renderer/Model_md5.cpp
	# renderer/Model_prt.cpp
	# renderer/Model_sprite.cpp
	# renderer/RenderEntity.cpp
	# renderer/RenderSystem.cpp
	# renderer/RenderSystem_init.cpp
	# renderer/RenderWorld.cpp
	# renderer/RenderWorld_demo.cpp
	# renderer/RenderWorld_load.cpp
	# renderer/RenderWorld_portals.cpp
	# renderer/VertexCache.cpp
	# renderer/draw_arb.cpp
	# renderer/draw_arb2.cpp
	# renderer/draw_glsl.cpp
	# renderer/draw_common.cpp
	# renderer/draw_exp.cpp
	# renderer/draw_nv10.cpp
	# renderer/draw_nv20.cpp
	# renderer/draw_r200.cpp
	# renderer/tr_backend.cpp
	# renderer/tr_deform.cpp
	# renderer/tr_font.cpp
	# renderer/tr_guisurf.cpp
	# renderer/tr_light.cpp
	# renderer/tr_lightrun.cpp
	# renderer/tr_main.cpp
	# renderer/tr_math.cpp
	# renderer/tr_orderIndexes.cpp
	# renderer/tr_polytope.cpp
	# renderer/tr_render.cpp
	# renderer/tr_rendertools.cpp
	# renderer/tr_shadowbounds.cpp
	# renderer/tr_stencilshadow.cpp
	# renderer/tr_subview.cpp
	# renderer/tr_trace.cpp
	# renderer/tr_trisurf.cpp
	# renderer/tr_turboshadow.cpp)
	
file(GLOB RENDERER_COLOR_INCLUDES renderer/Color/*.h)
file(GLOB RENDERER_COLOR_SOURCES renderer/Color/*.cpp)
file(GLOB RENDERER_DXT_INCLUDES renderer/DXT/*.h)
file(GLOB RENDERER_DXT_SOURCES renderer/DXT/*.cpp)
file(GLOB RENDERER_JOBS_INCLUDES renderer/jobs/*.h)
file(GLOB RENDERER_JOBS_SOURCES renderer/jobs/*.cpp)
file(GLOB RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES renderer/jobs/dynamicshadowvolume/*.h)
file(GLOB RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES renderer/jobs/dynamicshadowvolume/*.cpp)
file(GLOB RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES renderer/jobs/prelightshadowvolume/*.h)
file(GLOB RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES renderer/jobs/prelightshadowvolume/*.cpp)
file(GLOB RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES renderer/jobs/staticshadowvolume/*.h)
file(GLOB RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES renderer/jobs/staticshadowvolume/*.cpp)
file(GLOB RENDERER_OPENGL_INCLUDES renderer/OpenGL/*.h)
file(GLOB RENDERER_OPENGL_SOURCES renderer/OpenGL/*.cpp)

file(GLOB IRRXML_INCLUDES libs/irrxml/src/*.h)
file(GLOB IRRXML_SOURCES libs/irrxml/src/*.cpp)

if (NOT JPEG_FOUND)
    set(JPEG_INCLUDES
#        libs/jpeg-9b/jchuff.h
        libs/jpeg-9b/jconfig.h
        libs/jpeg-9b/jdct.h
#        libs/jpeg-9b/jdhuff.h
        libs/jpeg-9b/jerror.h
        libs/jpeg-9b/jinclude.h
        libs/jpeg-9b/jmemsys.h
        libs/jpeg-9b/jmorecfg.h
        libs/jpeg-9b/jpegint.h
        libs/jpeg-9b/jpeglib.h
        libs/jpeg-9b/jversion.h)

    set(JPEG_SOURCES
        libs/jpeg-9b/jaricom.cpp
        libs/jpeg-9b/jcapimin.cpp
        libs/jpeg-9b/jcapistd.cpp
        libs/jpeg-9b/jcarith.cpp
        libs/jpeg-9b/jccoefct.cpp
        libs/jpeg-9b/jccolor.cpp
        libs/jpeg-9b/jcdctmgr.cpp
        libs/jpeg-9b/jchuff.cpp
        libs/jpeg-9b/jcinit.cpp
        libs/jpeg-9b/jcmainct.cpp
        libs/jpeg-9b/jcmarker.cpp
        libs/jpeg-9b/jcmaster.cpp
        libs/jpeg-9b/jcomapi.cpp
        libs/jpeg-9b/jcparam.cpp
#        libs/jpeg-9b/jcphuff.cpp
        libs/jpeg-9b/jcprepct.cpp
        libs/jpeg-9b/jcsample.cpp
        libs/jpeg-9b/jctrans.cpp
        libs/jpeg-9b/jdapimin.cpp
        libs/jpeg-9b/jdapistd.cpp
        libs/jpeg-9b/jdarith.cpp
        libs/jpeg-9b/jdatadst.cpp
        libs/jpeg-9b/jdatasrc.cpp
        libs/jpeg-9b/jdcoefct.cpp
        libs/jpeg-9b/jdcolor.cpp
        libs/jpeg-9b/jddctmgr.cpp
        libs/jpeg-9b/jdhuff.cpp
        libs/jpeg-9b/jdinput.cpp
        libs/jpeg-9b/jdmainct.cpp
        libs/jpeg-9b/jdmarker.cpp
        libs/jpeg-9b/jdmaster.cpp
        libs/jpeg-9b/jdmerge.cpp
#        libs/jpeg-9b/jdphuff.cpp
        libs/jpeg-9b/jdpostct.cpp
        libs/jpeg-9b/jdsample.cpp
        libs/jpeg-9b/jdtrans.cpp
        libs/jpeg-9b/jerror.cpp
        libs/jpeg-9b/jfdctflt.cpp
        libs/jpeg-9b/jfdctfst.cpp
        libs/jpeg-9b/jfdctint.cpp
        libs/jpeg-9b/jidctflt.cpp
        libs/jpeg-9b/jidctfst.cpp
        libs/jpeg-9b/jidctint.cpp
#        libs/jpeg-9b/jidctred.cpp
        #libs/jpeg-9b/jload.cpp
        libs/jpeg-9b/jmemmgr.cpp
        libs/jpeg-9b/jmemnobs.cpp
        #libs/jpeg-9b/jpegtran.cpp
        libs/jpeg-9b/jquant1.cpp
        libs/jpeg-9b/jquant2.cpp
        libs/jpeg-9b/jutils.cpp)
else (NOT JPEG_FOUND)
   set(JPEG_INCLUDES "")
   set(JPEG_SOURCES "")
endif (NOT JPEG_FOUND)

if (NOT PNG_FOUND)
    file(GLOB PNG_INCLUDES libs/png/*.h)
    file(GLOB PNG_SOURCES libs/png/*.c)
else (NOT PNG_FOUND)
    set(PNG_INCLUDES "")
    set(PNG_SOURCES "")
endif (NOT PNG_FOUND)

if (NOT ZLIB_FOUND)
file(GLOB ZLIB_INCLUDES libs/zlib/*.h)
file(GLOB ZLIB_SOURCES libs/zlib/*.c)
else (NOT ZLIB_FOUND)
  set(ZLIB_INCLUDES "")
  set(ZLIB_SOURCES "")
endif (NOT ZLIB_FOUND)


file(GLOB MINIZIP_INCLUDES libs/zlib/minizip/*.h)
file(GLOB MINIZIP_SOURCES libs/zlib/minizip/*.c libs/zlib/minizip/*.cpp)
	
if (NOT GLEW_FOUND)
    set(GLEW_INCLUDES libs/glew/include/GL/glew.h)
    set(GLEW_SOURCES libs/glew/src/glew.c)

    if(WIN32)
        set(GLEW_INCLUDES ${GLEW_INCLUDES} libs/glew/include/GL/wglew.h)
    else(WIN32)
        set(GLEW_INCLUDES ${GLEW_INCLUDES} libs/glew/include/GL/glxew.h)
    endif(WIN32)
else (NOT GLEW_FOUND)
    set(GLEW_INCLUDES "")
    set(GLEW_SOURCES "")
endif (NOT GLEW_FOUND)
	
set(FREETYPE_SOURCES
	libs/freetype/src/autofit/autofit.c
	libs/freetype/src/bdf/bdf.c
	libs/freetype/src/cff/cff.c
	libs/freetype/src/base/ftbase.c
	libs/freetype/src/base/ftbitmap.c
	libs/freetype/src/cache/ftcache.c
	libs/freetype/src/base/ftdebug.c
	libs/freetype/src/base/ftgasp.c
	libs/freetype/src/base/ftglyph.c
	libs/freetype/src/gzip/ftgzip.c
	libs/freetype/src/base/ftinit.c
	libs/freetype/src/lzw/ftlzw.c
	libs/freetype/src/base/ftstroke.c
	libs/freetype/src/base/ftsystem.c
	libs/freetype/src/smooth/smooth.c
	libs/freetype/src/base/ftbbox.c
	libs/freetype/src/base/ftmm.c
	libs/freetype/src/base/ftpfr.c
	libs/freetype/src/base/ftsynth.c
	libs/freetype/src/base/fttype1.c
	libs/freetype/src/base/ftwinfnt.c
	libs/freetype/src/pcf/pcf.c
	libs/freetype/src/pfr/pfr.c
	libs/freetype/src/psaux/psaux.c
	libs/freetype/src/pshinter/pshinter.c
	libs/freetype/src/psnames/psmodule.c
	libs/freetype/src/raster/raster.c
	libs/freetype/src/sfnt/sfnt.c
	libs/freetype/src/truetype/truetype.c
	libs/freetype/src/type1/type1.c
	libs/freetype/src/cid/type1cid.c
	libs/freetype/src/type42/type42.c
	libs/freetype/src/winfonts/winfnt.c)
	
	
set(SOUND_INCLUDES
	sound/snd_local.h
	sound/sound.h
	sound/SoundVoice.h
	sound/WaveFile.h)
	
set(SOUND_SOURCES
	#sound/snd_cache.cpp
	#sound/snd_decoder.cpp
	#sound/snd_efxfile.cpp
	sound/snd_emitter.cpp
	sound/snd_shader.cpp
	sound/snd_system.cpp
	sound/snd_world.cpp
	sound/SoundVoice.cpp
	sound/WaveFile.cpp
	)
	
set(XAUDIO2_INCLUDES
	sound/XAudio2/XA2_SoundHardware.h
	sound/XAudio2/XA2_SoundSample.h
	sound/XAudio2/XA2_SoundVoice.h)
	
set(XAUDIO2_SOURCES
	sound/XAudio2/XA2_SoundHardware.cpp
	sound/XAudio2/XA2_SoundSample.cpp
	sound/XAudio2/XA2_SoundVoice.cpp)
	
set(OPENAL_INCLUDES
	sound/OpenAL/AL_SoundHardware.h
	sound/OpenAL/AL_SoundSample.h
	sound/OpenAL/AL_SoundVoice.h)
	
set(OPENAL_SOURCES
	sound/OpenAL/AL_SoundHardware.cpp
	sound/OpenAL/AL_SoundSample.cpp
	sound/OpenAL/AL_SoundVoice.cpp)

set(STUBAUDIO_INCLUDES
	sound/stub/SoundStub.h)

set(STUBAUDIO_SOURCES
	sound/stub/SoundSample.cpp)
	
set(OGGVORBIS_INCLUDES
	libs/oggvorbis/ogg/ogg.h
	libs/oggvorbis/ogg/os_types.h)
	
set(OGGVORBIS_SOURCES
	libs/oggvorbis/oggsrc/bitwise.c
	libs/oggvorbis/oggsrc/framing.c
	
	libs/oggvorbis/vorbissrc/mdct.c
	libs/oggvorbis/vorbissrc/smallft.c
	libs/oggvorbis/vorbissrc/block.c
	libs/oggvorbis/vorbissrc/envelope.c
	libs/oggvorbis/vorbissrc/windowvb.c
	libs/oggvorbis/vorbissrc/lsp.c
	libs/oggvorbis/vorbissrc/lpc.c
	libs/oggvorbis/vorbissrc/analysis.c
	libs/oggvorbis/vorbissrc/synthesis.c
	libs/oggvorbis/vorbissrc/psy.c
	libs/oggvorbis/vorbissrc/info.c
	libs/oggvorbis/vorbissrc/floor1.c
	libs/oggvorbis/vorbissrc/floor0.c
	libs/oggvorbis/vorbissrc/res0.c
	libs/oggvorbis/vorbissrc/mapping0.c
	libs/oggvorbis/vorbissrc/registry.c
	libs/oggvorbis/vorbissrc/codebook.c
	libs/oggvorbis/vorbissrc/sharedbook.c
	libs/oggvorbis/vorbissrc/lookup.c
	libs/oggvorbis/vorbissrc/bitrate.c
	libs/oggvorbis/vorbissrc/vorbisfile.c)

file(GLOB SYS_INCLUDES sys/*.h)
file(GLOB SYS_SOURCES sys/*.cpp)

file(GLOB UI_INCLUDES ui/*.h)
file(GLOB UI_SOURCES ui/*.cpp)

file(GLOB VR_INCLUDES vr/*.h)
file(GLOB VR_SOURCES vr/*.cpp)

file(GLOB SWF_INCLUDES swf/*.h)
file(GLOB SWF_SOURCES swf/*.cpp)

set(GAMED3XP_INCLUDES
	d3xp/Achievements.h
	d3xp/Actor.h
	d3xp/AF.h
	d3xp/AFEntity.h
	d3xp/AimAssist.h
	d3xp/BrittleFracture.h
	d3xp/Camera.h
	#d3xp/EndLevel.h
	d3xp/Entity.h
	d3xp/Fx.h
	d3xp/Game.h
	d3xp/GameEdit.h
	d3xp/Game_local.h
	d3xp/Grabber.h
	d3xp/IK.h
	d3xp/Item.h
	d3xp/Leaderboards.h
	d3xp/Light.h
	d3xp/Misc.h
	d3xp/Moveable.h
	d3xp/Mover.h
	d3xp/MultiplayerGame.h
	d3xp/Player.h
	d3xp/PlayerIcon.h
	d3xp/PlayerView.h
	d3xp/PredictedValue.h
	d3xp/Projectile.h
	d3xp/Pvs.h
	d3xp/SecurityCamera.h
	d3xp/SmokeParticles.h
	d3xp/Sound.h
	d3xp/Target.h
	d3xp/Trigger.h
	d3xp/Weapon.h
	d3xp/WorldSpawn.h)
	
set(GAMED3XP_SOURCES
	d3xp/Achievements.cpp
	d3xp/Actor.cpp
	d3xp/AF.cpp
	d3xp/AFEntity.cpp
	d3xp/AimAssist.cpp
	d3xp/BrittleFracture.cpp
	d3xp/Camera.cpp
	#d3xp/EndLevel.cpp
	d3xp/Entity.cpp
	d3xp/Fx.cpp
	d3xp/GameEdit.cpp
	d3xp/Game_local.cpp
	d3xp/Game_network.cpp
	d3xp/Grabber.cpp
	d3xp/IK.cpp
	d3xp/Item.cpp
	d3xp/Leaderboards.cpp
	d3xp/Light.cpp
	d3xp/Misc.cpp
	d3xp/Moveable.cpp
	d3xp/Mover.cpp
	d3xp/MultiplayerGame.cpp
	d3xp/Player.cpp
	d3xp/PlayerIcon.cpp
	d3xp/PlayerView.cpp
	d3xp/precompiled.cpp
	d3xp/Projectile.cpp
	d3xp/Pvs.cpp
	d3xp/SecurityCamera.cpp
	d3xp/SmokeParticles.cpp
	d3xp/Sound.cpp
	d3xp/Target.cpp
	d3xp/Trigger.cpp
	d3xp/Weapon.cpp
	d3xp/WorldSpawn.cpp)

file(GLOB GAMED3XP_AI_INCLUDES d3xp/ai/*.h)
file(GLOB GAMED3XP_AI_SOURCES d3xp/ai/*.cpp)

file(GLOB GAMED3XP_ANIM_INCLUDES d3xp/anim/*.h)
file(GLOB GAMED3XP_ANIM_SOURCES d3xp/anim/*.cpp)

file(GLOB GAMED3XP_GAMESYS_INCLUDES d3xp/gamesys/*.h)
set(GAMED3XP_GAMESYS_SOURCES
	#d3xp/gamesys/Callbacks.cpp
	d3xp/gamesys/Class.cpp
	#d3xp/gamesys/DebugGraph.cpp
	d3xp/gamesys/Event.cpp
	d3xp/gamesys/SaveGame.cpp
	d3xp/gamesys/SysCmds.cpp
	d3xp/gamesys/SysCvar.cpp
	#d3xp/gamesys/TypeInfo.cpp
	)

file(GLOB GAMED3XP_MENUS_INCLUDES d3xp/menus/*.h)
file(GLOB GAMED3XP_MENUS_SOURCES d3xp/menus/*.cpp)
	
file(GLOB GAMED3XP_PHYSICS_INCLUDES d3xp/physics/*.h)
file(GLOB GAMED3XP_PHYSICS_SOURCES d3xp/physics/*.cpp)

file(GLOB GAMED3XP_SCRIPT_INCLUDES d3xp/script/*.h)
file(GLOB GAMED3XP_SCRIPT_SOURCES d3xp/script/*.cpp)


file(GLOB COMPILER_INCLUDES tools/compilers/*.h)

file(GLOB COMPILER_AAS_INCLUDES tools/compilers/aas/*.h)
file(GLOB COMPILER_AAS_SOURCES tools/compilers/aas/*.cpp)

file(GLOB COMPILER_DMAP_INCLUDES tools/compilers/dmap/*.h)
file(GLOB COMPILER_DMAP_SOURCES tools/compilers/dmap/*.cpp)

file(GLOB COMPILER_RENDERBUMP_INCLUDES tools/compilers/renderbump/*.h)
file(GLOB COMPILER_RENDERBUMP_SOURCES tools/compilers/renderbump/*.cpp)

file(GLOB COMPILER_ROQVQ_INCLUDES tools/compilers/roqvq/*.h)
file(GLOB COMPILER_ROQVQ_SOURCES tools/compilers/roqvq/*.cpp)

set(TOOLS_INCLUDES tools/edit_public.h)

file(GLOB TOOLS_COMAFX_INCLUDES tools/comafx/*.h)
file(GLOB TOOLS_COMAFX_SOURCES tools/comafx/*.cpp)

set(TOOLS_COMMON_INCLUDES
	tools/common/ColorButton.h
	tools/common/DialogHelpers.h
	tools/common/MaskEdit.h
	tools/common/OpenFileDialog.h
	tools/common/PropertyGrid.h
	tools/common/RegistryOptions.h
	#tools/common/RenderBumpFlatDialog.h
	tools/common/RollupPanel.h
	tools/common/SpinButton.h)

set(TOOLS_COMMON_SOURCES
	tools/common/AlphaPopup.cpp
	tools/common/ColorButton.cpp
	tools/common/MaskEdit.cpp
	tools/common/OpenFileDialog.cpp
	tools/common/PropertyGrid.cpp
	tools/common/RegistryOptions.cpp
	#tools/common/RenderBumpFlatDialog.cpp
	tools/common/RollupPanel.cpp
	tools/common/SpinButton.cpp)

file(GLOB TOOLS_COMMON_PROPTREE_INCLUDES tools/common/PropTree/*.h)
file(GLOB TOOLS_COMMON_PROPTREE_SOURCES tools/common/PropTree/*.cpp)

file(GLOB EDITOR_AF_INCLUDES tools/af/*.h)
file(GLOB EDITOR_AF_SOURCES tools/af/*.cpp)

set(EDITOR_DECL_INCLUDES
	tools/decl/DialogDeclBrowser.h
	tools/decl/DialogDeclEditor.h
	tools/decl/DialogDeclNew.h)
	
set(EDITOR_DECL_SOURCES
	tools/decl/DialogDeclBrowser.cpp
	tools/decl/DialogDeclEditor.cpp
	tools/decl/DialogDeclNew.cpp)
	
file(GLOB EDITOR_DEBUGGER_INCLUDES tools/debugger/*.h)
file(GLOB EDITOR_DEBUGGER_SOURCES tools/debugger/*.cpp)

file(GLOB EDITOR_GUIED_INCLUDES tools/guied/*.h)
set(EDITOR_GUIED_SOURCES
	tools/guied/GEApp.cpp
	tools/guied/GECheckInDlg.cpp
	tools/guied/GEDeleteModifier.cpp
	tools/guied/GEHideModifier.cpp
	tools/guied/GEInsertModifier.cpp
	tools/guied/GEItemPropsDlg.cpp
	tools/guied/GEItemScriptsDlg.cpp
	tools/guied/GEKeyValueModifier.cpp
	tools/guied/GEModifier.cpp
	tools/guied/GEModifierGroup.cpp
	tools/guied/GEModifierStack.cpp
	tools/guied/GEMoveModifier.cpp
	tools/guied/GENavigator.cpp
	tools/guied/GEOptions.cpp
	tools/guied/GEOptionsDlg.cpp
	tools/guied/GEProperties.cpp
	tools/guied/GEPropertyPage.cpp
	tools/guied/GESelectionMgr.cpp
	tools/guied/GESizeModifier.cpp
	tools/guied/GEStateModifier.cpp
	tools/guied/GEStatusBar.cpp
	tools/guied/GETransformer.cpp
	tools/guied/GEViewer.cpp
	tools/guied/GEWindowWrapper.cpp
	#tools/guied/GEWindowWrapper_stub.cpp
	tools/guied/GEWorkspace.cpp
	tools/guied/GEWorkspaceFile.cpp
	tools/guied/GEZOrderModifier.cpp
	tools/guied/guied.cpp)

file(GLOB EDITOR_MATERIALEDITOR_INCLUDES tools/materialeditor/*.h)
file(GLOB EDITOR_MATERIALEDITOR_SOURCES tools/materialeditor/*.cpp)

file(GLOB EDITOR_PARTICLE_INCLUDES tools/particle/*.h)
file(GLOB EDITOR_PARTICLE_SOURCES tools/particle/*.cpp)

file(GLOB EDITOR_PDA_INCLUDES tools/pda/*.h)
file(GLOB EDITOR_PDA_SOURCES tools/pda/*.cpp)

file(GLOB EDITOR_RADIANT_INCLUDES tools/radiant/*.h)
file(GLOB EDITOR_RADIANT_SOURCES tools/radiant/*.cpp)

file(GLOB EDITOR_SCRIPT_INCLUDES tools/script/*.h)
file(GLOB EDITOR_SCRIPT_SOURCES tools/script/*.cpp)

file(GLOB EDITOR_SOUND_INCLUDES tools/sound/*.h)
file(GLOB EDITOR_SOUND_SOURCES tools/sound/*.cpp)

file(GLOB DOOMCLASSIC_INCLUDES ../doomclassic/src/*.h)
file(GLOB DOOMCLASSIC_SOURCES ../doomclassic/src/*.cpp;../doomclassic/src/*.c)
include_directories(../doomclassic/src)

file(GLOB DOOMCLASSIC_R_INCLUDES ../doomclassic/src/r_*.h)
file(GLOB DOOMCLASSIC_R_SOURCES ../doomclassic/src/r_*.cpp)
list(REMOVE_ITEM DOOMCLASSIC_INCLUDES ${DOOMCLASSIC_R_INCLUDES})
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${DOOMCLASSIC_R_SOURCES})

file(GLOB DOOMCLASSIC_VERSIONING_INCLUDES ../doomclassic/src/version.h)
file(GLOB DOOMCLASSIC_VERSIONING_SOURCES ../doomclassic/src/win32/zdoom.rc)
list(REMOVE_ITEM DOOMCLASSIC_INCLUDES ${DOOMCLASSIC_VERSIONING_INCLUDES})
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${DOOMCLASSIC_VERSIONING_SOURCES})

file(GLOB DOOMCLASSIC_FRAGGLE_INCLUDES ../doomclassic/src/fragglescript/*.h)
file(GLOB DOOMCLASSIC_FRAGGLE_SOURCES ../doomclassic/src/fragglescript/*.cpp)

set(DOOMCLASSIC_G_DOOM_INCLUDES ../doomclassic/src/g_doom/a_doomglobal.h)
set(DOOMCLASSIC_G_DOOM_SOURCES ../doomclassic/src/g_doom/a_doommisc.cpp)
include_directories(../doomclassic/src/g_doom)

set(DOOMCLASSIC_G_HERETIC_INCLUDES )
set(DOOMCLASSIC_G_HERETIC_SOURCES ../doomclassic/src/g_heretic/a_hereticmisc.cpp)
include_directories(../doomclassic/src/g_heretic)

set(DOOMCLASSIC_G_HEXEN_INCLUDES ../doomclassic/src/g_hexen/a_hexenglobal.h)
set(DOOMCLASSIC_G_HEXEN_SOURCES ../doomclassic/src/g_hexen/a_hexenmisc.cpp)
include_directories(../doomclassic/src/g_hexen)

file(GLOB DOOMCLASSIC_G_RAVEN_INCLUDES ../doomclassic/src/g_raven/*.h)
file(GLOB DOOMCLASSIC_G_RAVEN_SOURCES ../doomclassic/src/g_raven/*.cpp)
include_directories(../doomclassic/src/g_raven)

set(DOOMCLASSIC_G_STRIFE_INCLUDES ../doomclassic/src/g_strife/a_strifeglobal.h)
set(DOOMCLASSIC_G_STRIFE_SOURCES
	../doomclassic/src/g_strife/a_strifestuff.cpp
	../doomclassic/src/g_strife/strife_sbar.cpp)
include_directories(../doomclassic/src/g_strife)

file(GLOB DOOMCLASSIC_G_SHARED_INCLUDES ../doomclassic/src/g_shared/*.h)
file(GLOB DOOMCLASSIC_G_SHARED_SOURCES ../doomclassic/src/g_shared/*.cpp)
file(GLOB DOOMCLASSIC_G_SHARED_BAD ../doomclassic/src/g_shared/sbarinfo_commands.cpp)
list(REMOVE_ITEM DOOMCLASSIC_G_SHARED_SOURCES ${DOOMCLASSIC_G_SHARED_BAD})
include_directories(../doomclassic/src/g_shared)

file(GLOB_RECURSE DOOMCLASSIC_GL_INCLUDES ../doomclassic/src/gl/*.h)
file(GLOB_RECURSE DOOMCLASSIC_GL_SOURCES ../doomclassic/src/gl/*.c*)
file(GLOB DOOMCLASSIC_GL_BAD ../doomclassic/src/gl/unused/*.*;../doomclassic/src/gl/scene/gl_ovrisolator.cpp)
list(REMOVE_ITEM DOOMCLASSIC_GL_SOURCES ${DOOMCLASSIC_GL_BAD})

file(GLOB DOOMCLASSIC_INTERMISSION_INCLUDES ../doomclassic/src/intermission/*.h)
file(GLOB DOOMCLASSIC_INTERMISSION_SOURCES ../doomclassic/src/intermission/*.cpp)

file(GLOB DOOMCLASSIC_MENU_INCLUDES ../doomclassic/src/menu/*.h)
file(GLOB DOOMCLASSIC_MENU_SOURCES ../doomclassic/src/menu/*.cpp)

file(GLOB_RECURSE DOOMCLASSIC_OPLSYNTH_INCLUDES ../doomclassic/src/oplsynth/*.h)
file(GLOB_RECURSE DOOMCLASSIC_OPLSYNTH_SOURCES ../doomclassic/src/oplsynth/*.cpp)

file(GLOB_RECURSE DOOMCLASSIC_POSIX_INCLUDES ../doomclassic/src/posix/*.h;../doomclassic/src/posix/sdl/*.h)
file(GLOB DOOMCLASSIC_POSIX_SOURCES ../doomclassic/src/posix/*.cpp;../doomclassic/src/posix/sdl/*.cpp)

file(GLOB DOOMCLASSIC_R_DATA_INCLUDES ../doomclassic/src/r_data/*.h)
file(GLOB DOOMCLASSIC_R_DATA_SOURCES ../doomclassic/src/r_data/*.cpp)

file(GLOB DOOMCLASSIC_RESOURCE_INCLUDES ../doomclassic/src/resourcefiles/*.h)
file(GLOB DOOMCLASSIC_RESOURCE_SOURCES ../doomclassic/src/resourcefiles/*.cpp)

file(GLOB DOOMCLASSIC_SFMT_INCLUDES ../doomclassic/src/sfmt/*.h)
file(GLOB DOOMCLASSIC_SFMT_SOURCES ../doomclassic/src/sfmt/*.cpp)

file(GLOB DOOMCLASSIC_SOUND_INCLUDES ../doomclassic/src/sound/*.h)
file(GLOB DOOMCLASSIC_SOUND_SOURCES ../doomclassic/src/sound/*.cpp)
include_directories(../doomclassic/src/sound)

file(GLOB DOOMCLASSIC_TEXTURES_INCLUDES ../doomclassic/src/textures/*.h)
file(GLOB DOOMCLASSIC_TEXTURES_SOURCES ../doomclassic/src/textures/*.cpp)

file(GLOB DOOMCLASSIC_THINGDEF_INCLUDES ../doomclassic/src/thingdef/*.h)
file(GLOB DOOMCLASSIC_THINGDEF_SOURCES ../doomclassic/src/thingdef/*.cpp)
include_directories(../doomclassic/src/thingdef)

file(GLOB DOOMCLASSIC_WILDMIDI_INCLUDES ../doomclassic/src/wildmidi/*.h)
file(GLOB DOOMCLASSIC_WILDMIDI_SOURCES ../doomclassic/src/wildmidi/*.cpp)

file(GLOB DOOMCLASSIC_WIN32_INCLUDES ../doomclassic/src/win32/*.h)
file(GLOB DOOMCLASSIC_WIN32_SOURCES ../doomclassic/src/win32/*.cpp)
include_directories(../doomclassic/src/win32)

file(GLOB DOOMCLASSIC_XLAT_INCLUDES ../doomclassic/src/xlat/*.h)
file(GLOB DOOMCLASSIC_XLAT_SOURCES ../doomclassic/src/xlat/*.cpp)

file(GLOB DOOMCLASSIC_BZIP2_INCLUDES ../doomclassic/bzip2/*.h)
file(GLOB DOOMCLASSIC_BZIP2_SOURCES ../doomclassic/bzip2/*.c)
include_directories(../doomclassic/bzip2)

file(GLOB_RECURSE DOOMCLASSIC_DUMB_INCLUDES ../doomclassic/dumb/*.h)
file(GLOB_RECURSE DOOMCLASSIC_DUMB_SOURCES ../doomclassic/dumb/src/core/*.c;../doomclassic/dumb/src/helpers/*.c;../doomclassic/dumb/src/it/*.c*)
include_directories(../doomclassic/dumb/include)

file(GLOB DOOMCLASSIC_GDTOA_INCLUDES ../doomclassic/gdtoa/*.h)
file(GLOB DOOMCLASSIC_GDTOA_SOURCES ../doomclassic/gdtoa/dmisc.c ../doomclassic/gdtoa/dtoa.c ../doomclassic/gdtoa/misc.c)
include_directories(../doomclassic/gdtoa)

file(GLOB DOOMCLASSIC_GME_INCLUDES "../doomclassic/game-music-emu/gme/*.h")
file(GLOB DOOMCLASSIC_GME_SOURCES "../doomclassic/game-music-emu/gme/*.cpp")
include_directories(../doomclassic/game-music-emu)

file(GLOB DOOMCLASSIC_LZMA_INCLUDES ../doomclassic/lzma/c/*.h)
file(GLOB DOOMCLASSIC_LZMA_SOURCES ../doomclassic/lzma/c/*.c)
include_directories(../doomclassic/lzma/c)

set(DOOMCLASSIC_STUBAUDIO_SOURCES ../doomclassic/src/i_sound_stub.cpp)
set(DOOMCLASSIC_OPENAL_SOURCES ../doomclassic/src/i_sound_openal.cpp)
set(DOOMCLASSIC_XAUDIO2_SOURCES ../doomclassic/src/i_sound_win32.cpp)

file(GLOB DOOMCLASSIC_BFG_INCLUDES ../doomclassic/src/doomlib.h;../doomclassic/src/doominterface.h; ../doomclassic/src/doomdef.h;../doomclassic/src/d_event.h;../doomclassic/src/d_main.h; ../doomclassic/src/globaldata.h;../doomclassic/src/vars.h;../doomclassic/src/constructs.h; ../doomclassic/src/sound/i_sound.h; ../doomclassic/src/Precompiled.h)
file(GLOB DOOMCLASSIC_BFG_SOURCES ../doomclassic/src/doomlib.cpp;../doomclassic/src/doominterface.cpp; ../doomclassic/src/d_main.cpp; ../doomclassic/src/globaldata.cpp; ../doomclassic/src/i_sound_win32.cpp; ../doomclassic/src/Precompiled.cpp)
list(REMOVE_ITEM DOOMCLASSIC_INCLUDES ${DOOMCLASSIC_BFG_INCLUDES})
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${DOOMCLASSIC_BFG_SOURCES})

list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/src/i_sound_stub.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/src/i_sound_openal.cpp)
list(REMOVE_ITEM DOOMCLASSIC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../doomclassic/src/i_sound_win32.cpp)

file(GLOB TIMIDITY_INCLUDES ../doomclassic/src/timidity/*.h)
file(GLOB TIMIDITY_SOURCES ../doomclassic/src/timidity/*.cpp)

file(GLOB_RECURSE WIN32_INCLUDES sys/win32/*.h)
#file(GLOB_RECURSE WIN32_SOURCES sys/win32/*.cpp)

set(WIN32_SOURCES
	sys/win32/win_glimp.cpp
	sys/win32/win_input.cpp
	sys/win32/win_main.cpp
	sys/win32/win_shared.cpp
	sys/win32/win_snd.cpp
	sys/win32/win_syscon.cpp
	sys/win32/win_taskkeyhook.cpp
	sys/win32/win_wndproc.cpp)
	
if(MSVC)
	list(APPEND WIN32_SOURCES sys/win32/win_cpu.cpp)
endif()

set(WIN32_RESOURCES
	# sys/win32/rc/res/BEVEL.BMP
	# sys/win32/rc/res/BITMAP2.BMP
	# sys/win32/rc/res/BMP00001.BMP
	# sys/win32/rc/res/BMP0002.BMP
	# sys/win32/rc/res/DEFTEX.WAL
	# sys/win32/rc/res/ENDCAP.BMP
	# sys/win32/rc/res/GetString.htm
	# sys/win32/rc/res/IBEVEL.BMP
	# sys/win32/rc/res/IENDCAP.BMP
	# sys/win32/rc/res/MEFileToolbar.bmp
	# sys/win32/rc/res/MEtoolbar.bmp
	# sys/win32/rc/res/MaterialEditor.ico
	# sys/win32/rc/res/PropTree.rc2
	# sys/win32/rc/res/Q.BMP
	# sys/win32/rc/res/RADIANT3.GIF
	# sys/win32/rc/res/Radiant.ico
	# sys/win32/rc/res/RadiantDoc.ico
	# sys/win32/rc/res/TOOLBAR1.BMP
	# sys/win32/rc/res/TOOLBAR2.BMP
	# sys/win32/rc/res/Toolbar.bmp
	# sys/win32/rc/res/VIEWDEFA.BMP
	# sys/win32/rc/res/VIEWOPPO.BMP
	# sys/win32/rc/res/bmp00002.bmp
	# sys/win32/rc/res/bmp00003.bmp
	# sys/win32/rc/res/bmp00004.bmp
	# sys/win32/rc/res/bmp00005.bmp
	# sys/win32/rc/res/cchsb.bmp
	# sys/win32/rc/res/ccrgb.bmp
	# sys/win32/rc/res/dbg_back.bmp
	# sys/win32/rc/res/dbg_breakpoint.ico
	# sys/win32/rc/res/dbg_current.ico
	# sys/win32/rc/res/dbg_currentline.ico
	# sys/win32/rc/res/dbg_empty.ico
	# sys/win32/rc/res/dbg_open.bmp
	# sys/win32/rc/res/dbg_toolbar.bmp
	sys/win32/rc/res/doom.ico
	# sys/win32/rc/res/fpoint.cur
	# sys/win32/rc/res/fxed_link.ico
	# sys/win32/rc/res/fxed_toolbar.bmp
	# sys/win32/rc/res/fxeditor.ico
	# sys/win32/rc/res/guied.ico
	# sys/win32/rc/res/guied_collapse.ico
	# sys/win32/rc/res/guied_expand.ico
	# sys/win32/rc/res/guied_hand.cur
	# sys/win32/rc/res/guied_nav_visible.ico
	# sys/win32/rc/res/guied_nav_visibledisabled.ico
	# sys/win32/rc/res/guied_scripts.ico
	# sys/win32/rc/res/guied_scripts_white.ico
	# sys/win32/rc/res/guied_viewer_toolbar.bmp
	# sys/win32/rc/res/icon2.ico
	# sys/win32/rc/res/logo_sm3dfx.bmp
	# sys/win32/rc/res/matedtree.bmp
	# sys/win32/rc/res/me_disabled_icon.ico
	# sys/win32/rc/res/me_enabled.ico
	# sys/win32/rc/res/me_off_icon.ico
	# sys/win32/rc/res/me_on_icon.ico
	# sys/win32/rc/res/qe3.ico
	# sys/win32/rc/res/shaderbar.bmp
	# sys/win32/rc/res/shaderdoc.ico
	# sys/win32/rc/res/shaderframe.ico
	# sys/win32/rc/res/spliter.cur
	)



#if(USE_MFC_TOOLS)
	list(APPEND WIN32_RESOURCES sys/win32/rc/doom.rc)
#else()
#	list(APPEND WIN32_RESOURCES sys/win32/rc/doom_nomfc.rc)
#endif()


file(GLOB POSIX_INCLUDES sys/posix/*.h)
file(GLOB POSIX_SOURCES sys/posix/*.cpp)

file(GLOB COMMON_INCLUDES sys/common/*.h)
file(GLOB COMMON_SOURCES sys/common/*.cpp)

file(GLOB SDL_INCLUDES sys/sdl/*.h)
file(GLOB SDL_SOURCES sys/sdl/*.cpp)

# Carl: GZ3Doom
add_subdirectory( ../doomclassic/bzip2 ${CMAKE_CURRENT_BINARY_DIR}/bzip2)
add_subdirectory( ../doomclassic/game-music-emu ${CMAKE_CURRENT_BINARY_DIR}/game-music-emu)
add_subdirectory( ../doomclassic/lzma ${CMAKE_CURRENT_BINARY_DIR}/lzma)
add_subdirectory( ../doomclassic/tools ${CMAKE_CURRENT_BINARY_DIR}/tools)
add_subdirectory( ../doomclassic/dumb ${CMAKE_CURRENT_BINARY_DIR}/dumb)
add_subdirectory( ../doomclassic/gdtoa ${CMAKE_CURRENT_BINARY_DIR}/gdtoa)
add_subdirectory( ../doomclassic/wadsrc ${CMAKE_CURRENT_BINARY_DIR}/wadsrc)
add_subdirectory( ../doomclassic/wadsrc_bm ${CMAKE_CURRENT_BINARY_DIR}/wadsrc_bm)
add_subdirectory( ../doomclassic/wadsrc_lights ${CMAKE_CURRENT_BINARY_DIR}/wadsrc_lights)

# Update gitinfo.h

add_custom_target( revision_check ALL
	COMMAND updaterevision ../doomclassic/src/gitinfo.h
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	DEPENDS updaterevision )


source_group("aas" FILES ${AAS_INCLUDES})
source_group("aas" FILES ${AAS_SOURCES})

source_group("cm" FILES ${CM_INCLUDES})
source_group("cm" FILES ${CM_SOURCES})

source_group("framework" FILES ${FRAMEWORK_INCLUDES})
source_group("framework" FILES ${FRAMEWORK_SOURCES})

source_group("framework\\async" FILES ${FRAMEWORK_ASYNC_INCLUDES})
source_group("framework\\async" FILES ${FRAMEWORK_ASYNC_SOURCES})

source_group("renderer" FILES ${RENDERER_INCLUDES})
source_group("renderer" FILES ${RENDERER_SOURCES})

source_group("renderer\\Color" FILES ${RENDERER_COLOR_INCLUDES})
source_group("renderer\\Color" FILES ${RENDERER_COLOR_SOURCES})

source_group("renderer\\DXT" FILES ${RENDERER_DXT_INCLUDES})
source_group("renderer\\DXT" FILES ${RENDERER_DXT_SOURCES})

source_group("renderer\\jobs" FILES ${RENDERER_JOBS_INCLUDES})
source_group("renderer\\jobs" FILES ${RENDERER_JOBS_SOURCES})

source_group("renderer\\jobs\\dynamicshadowvolume" FILES ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\dynamicshadowvolume" FILES ${RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES})

source_group("renderer\\jobs\\prelightshadowvolume" FILES ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\prelightshadowvolume" FILES ${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES})

source_group("renderer\\jobs\\staticshadowvolume" FILES ${RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES})
source_group("renderer\\jobs\\staticshadowvolume" FILES ${RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES})

source_group("renderer\\OpenGL" FILES ${RENDERER_OPENGL_INCLUDES})
source_group("renderer\\OpenGL" FILES ${RENDERER_OPENGL_SOURCES})

source_group("libs\\irrxml" FILES ${IRRXML_INCLUDES})
source_group("libs\\irrxml" FILES ${IRRXML_SOURCES})

source_group("libs\\jpeg-9b" FILES ${JPEG_INCLUDES})
source_group("libs\\jpeg-9b" FILES ${JPEG_SOURCES})

source_group("libs\\png" FILES ${PNG_INCLUDES})
source_group("libs\\png" FILES ${PNG_SOURCES})

source_group("libs\\zlib" FILES ${ZLIB_INCLUDES})
source_group("libs\\zlib" FILES ${ZLIB_SOURCES})

source_group("libs\\zlib\\minizip" FILES ${MINIZIP_INCLUDES})
source_group("libs\\zlib\\minizip" FILES ${MINIZIP_SOURCES})

source_group("libs\\glew" FILES ${GLEW_INCLUDES})
source_group("libs\\glew" FILES ${GLEW_SOURCES})

source_group("libs\\freetype" FILES ${FREETYPE_SOURCES})

source_group("sound" FILES ${SOUND_INCLUDES})
source_group("sound" FILES ${SOUND_SOURCES})

source_group("sound\\XAudio2" FILES ${XAUDIO2_INCLUDES})
source_group("sound\\XAudio2" FILES ${XAUDIO2_SOURCES})

source_group("sound\\OpenAL" FILES ${OPENAL_INCLUDES})
source_group("sound\\OpenAL" FILES ${OPENAL_SOURCES})

source_group("sound\\stub" FILES ${STUBAUDIO_INCLUDES})
source_group("sound\\stub" FILES ${STUBAUDIO_SOURCES})

source_group("libs\\oggvorbis" FILES ${OGGVORBIS_INCLUDES})
source_group("libs\\oggvorbis" FILES ${OGGVORBIS_SOURCES})

source_group("ui" FILES ${UI_INCLUDES})
source_group("ui" FILES ${UI_SOURCES})

source_group("vr" FILES ${VR_INCLUDES})
source_group("vr" FILES ${VR_SOURCES})

source_group("swf" FILES ${SWF_INCLUDES})
source_group("swf" FILES ${SWF_SOURCES})

source_group("sys" FILES ${SYS_INCLUDES})
source_group("sys" FILES ${SYS_SOURCES})

source_group("doomclassic\\Header Files" FILES ${DOOMCLASSIC_INCLUDES})
source_group("doomclassic\\Source Files" FILES ${DOOMCLASSIC_SOURCES})

source_group("doomclassic\\BFG interface" FILES ${DOOMCLASSIC_BFG_INCLUDES} ${DOOMCLASSIC_BFG_SOURCES})

source_group("doomclassic\\Audio Files" FILES ${DOOMCLASSIC_STUBAUDIO_SOURCES})
source_group("doomclassic\\Audio Files" FILES ${DOOMCLASSIC_OPENAL_SOURCES})
#source_group("doomclassic\\Audio Files" FILES ${DOOMCLASSIC_XAUDIO2_SOURCES})

source_group("doomclassic\\Fragglescript" FILES ${DOOMCLASSIC_FRAGGLE_INCLUDES} ${DOOMCLASSIC_FRAGGLE_SOURCES})
source_group("doomclassic\\Games\\Doom Game" FILES ${DOOMCLASSIC_G_DOOM_INCLUDES} ${DOOMCLASSIC_G_DOOM_SOURCES})
source_group("doomclassic\\Games\\Heretic Game" FILES ${DOOMCLASSIC_G_HERETIC_INCLUDES} ${DOOMCLASSIC_G_HERETIC_SOURCES})
source_group("doomclassic\\Games\\Hexen Game" FILES ${DOOMCLASSIC_G_HEXEN_INCLUDES} ${DOOMCLASSIC_G_HEXEN_SOURCES})
source_group("doomclassic\\Games\\Strife Game" FILES ${DOOMCLASSIC_G_STRIFE_INCLUDES} ${DOOMCLASSIC_G_STRIFE_SOURCES})
source_group("doomclassic\\Games\\Raven Shared" FILES ${DOOMCLASSIC_G_RAVEN_INCLUDES} ${DOOMCLASSIC_G_RAVEN_SOURCES})
source_group("doomclassic\\Shared Game" FILES ${DOOMCLASSIC_G_SHARED_INCLUDES} ${DOOMCLASSIC_G_SHARED_SOURCES})
source_group("doomclassic\\OpenGL Renderer" FILES ${DOOMCLASSIC_GL_INCLUDES} ${DOOMCLASSIC_GL_SOURCES})
source_group("doomclassic\\Render Core" FILES ${DOOMCLASSIC_R_INCLUDES} ${DOOMCLASSIC_R_SOURCES})
source_group("doomclassic\\Intermission" FILES ${DOOMCLASSIC_INTERMISSION_INCLUDES} ${DOOMCLASSIC_INTERMISSION_SOURCES})
source_group("doomclassic\\Menu" FILES ${DOOMCLASSIC_MENU_INCLUDES} ${DOOMCLASSIC_MENU_SOURCES})
source_group("doomclassic\\Audio Files\\OPL Synth" FILES ${DOOMCLASSIC_OPLSYNTH_INCLUDES} ${DOOMCLASSIC_OPLSYNTH_SOURCES})
source_group("doomclassic\\POSIX Files" FILES ${DOOMCLASSIC_POSIX_INCLUDES} ${DOOMCLASSIC_POSIX_SOURCES})
source_group("doomclassic\\Resource Files" FILES ${DOOMCLASSIC_RESOURCE_INCLUDES} ${DOOMCLASSIC_RESOURCE_SOURCES})
source_group("doomclassic\\SFMT" FILES ${DOOMCLASSIC_SFMT_INCLUDES} ${DOOMCLASSIC_SFMT_SOURCES})
source_group("doomclassic\\Audio Files" FILES ${DOOMCLASSIC_SOUND_INCLUDES} ${DOOMCLASSIC_SOUND_SOURCES})
source_group("doomclassic\\Audio Files\\WildMidi" FILES ${DOOMCLASSIC_WILDMIDI_INCLUDES} ${DOOMCLASSIC_WILDMIDI_SOURCES})
source_group("doomclassic\\Render Data\\Resources" FILES ${DOOMCLASSIC_R_DATA_INCLUDES} ${DOOMCLASSIC_R_DATA_SOURCES})
source_group("doomclassic\\Render Data\\Textures" FILES ${DOOMCLASSIC_TEXTURES_INCLUDES} ${DOOMCLASSIC_TEXTURES_SOURCES})
source_group("doomclassic\\Decorate++" FILES ${DOOMCLASSIC_THINGDEF_INCLUDES} ${DOOMCLASSIC_THINGDEF_SOURCES})
source_group("doomclassic\\Versioning" FILES ${DOOMCLASSIC_VERSIONING_INCLUDES} ${DOOMCLASSIC_VERSIONING_SOURCES})
source_group("doomclassic\\Win32 Files" FILES ${DOOMCLASSIC_WIN32_INCLUDES} ${DOOMCLASSIC_WIN32_SOURCES})
source_group("doomclassic\\xlat" FILES ${DOOMCLASSIC_XLAT_INCLUDES} ${DOOMCLASSIC_XLAT_SOURCES})

source_group("doomclassic\\tools" FILES ${DOOMCLASSIC_TOOLS_INCLUDES} ${DOOMCLASSIC_TOOLS_SOURCES})
source_group("doomclassic\\wadsrc" FILES ${DOOMCLASSIC_WADSRC_INCLUDES})
source_group("doomclassic\\wadsrc" FILES ${DOOMCLASSIC_WADSRC_SOURCES})
source_group("doomclassic\\wadsrc_bm" FILES ${DOOMCLASSIC_WADSRC_BM_INCLUDES})
source_group("doomclassic\\wadsrc_bm" FILES ${DOOMCLASSIC_WADSRC_BM_SOURCES})
source_group("doomclassic\\wadsrc_lights" FILES ${DOOMCLASSIC_WADSRC_LIGHTS_INCLUDES})
source_group("doomclassic\\wadsrc_lights" FILES ${DOOMCLASSIC_WADSRC_LIGHTS_SOURCES})

source_group("libs\\timidity" FILES ${TIMIDITY_INCLUDES})
source_group("libs\\timidity" FILES ${TIMIDITY_SOURCES})

source_group("libs\\bzip2" FILES ${DOOMCLASSIC_BZIP2_INCLUDES} ${DOOMCLASSIC_BZIP2_SOURCES})
source_group("libs\\DUMB" FILES ${DOOMCLASSIC_DUMB_INCLUDES} ${DOOMCLASSIC_DUMB_SOURCES})
source_group("libs\\gdtoa" FILES ${DOOMCLASSIC_GDTOA_INCLUDES} ${DOOMCLASSIC_GDTOA_SOURCES})
source_group("libs\\Game Music Emulator" FILES ${DOOMCLASSIC_GME_INCLUDES} ${DOOMCLASSIC_GME_SOURCES})
source_group("libs\\lzma" FILES ${DOOMCLASSIC_LZMA_INCLUDES} ${DOOMCLASSIC_LZMA_SOURCES})

source_group("game-d3xp" FILES ${GAMED3XP_INCLUDES})
source_group("game-d3xp" FILES ${GAMED3XP_SOURCES})

source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_INCLUDES})
source_group("game-d3xp\\ai" FILES ${GAMED3XP_AI_SOURCES})

source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_INCLUDES})
source_group("game-d3xp\\anim" FILES ${GAMED3XP_ANIM_SOURCES})

source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_INCLUDES})
source_group("game-d3xp\\gamesys" FILES ${GAMED3XP_GAMESYS_SOURCES})

source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_INCLUDES})
source_group("game-d3xp\\menus" FILES ${GAMED3XP_MENUS_SOURCES})

source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_INCLUDES})
source_group("game-d3xp\\physics" FILES ${GAMED3XP_PHYSICS_SOURCES})

source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_INCLUDES})
source_group("game-d3xp\\script" FILES ${GAMED3XP_SCRIPT_SOURCES})




source_group("sys\\win32" FILES ${WIN32_INCLUDES})
source_group("sys\\win32" FILES ${WIN32_SOURCES})
source_group("sys\\win32\\Resources" FILES ${WIN32_RESOURCES})

source_group("sys\\posix" FILES ${POSIX_INCLUDES})
source_group("sys\\posix" FILES ${POSIX_SOURCES})

source_group("sys\\common" FILES ${COMMON_INCLUDES})
source_group("sys\\common" FILES ${COMMON_SOURCES})

source_group("sys\\sdl" FILES ${SDL_INCLUDES})
source_group("sys\\sdl" FILES ${SDL_SOURCES})


source_group("tools\\compilers" FILES ${COMPILER_INCLUDES})

source_group("tools\\compilers\\aas" FILES ${COMPILER_AAS_INCLUDES})
source_group("tools\\compilers\\aas" FILES ${COMPILER_AAS_SOURCES})

source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_INCLUDES})
source_group("tools\\compilers\\dmap" FILES ${COMPILER_DMAP_SOURCES})

source_group("tools\\compilers\\renderbump" FILES ${COMPILER_RENDERBUMP_INCLUDES})
source_group("tools\\compilers\\renderbump" FILES ${COMPILER_RENDERBUMP_SOURCES})

source_group("tools\\compilers\\roqvq" FILES ${COMPILER_ROQVQ_INCLUDES})
source_group("tools\\compilers\\roqvq" FILES ${COMPILER_ROQVQ_SOURCES})

source_group("tools" FILES ${TOOLS_INCLUDES})

source_group("tools\\comafx" FILES ${TOOLS_COMAFX_INCLUDES})
source_group("tools\\comafx" FILES ${TOOLS_COMAFX_SOURCES})

source_group("tools\\common" FILES ${TOOLS_COMMON_INCLUDES})
source_group("tools\\common" FILES ${TOOLS_COMMON_SOURCES})

source_group("tools\\common\\proptree" FILES ${TOOLS_COMMON_PROPTREE_INCLUDES})
source_group("tools\\common\\proptree" FILES ${TOOLS_COMMON_PROPTREE_SOURCES})

source_group("tools\\af" FILES ${EDITOR_AF_INCLUDES})
source_group("tools\\af" FILES ${EDITOR_AF_SOURCES})

source_group("tools\\debugger" FILES ${EDITOR_DEBUGGER_INCLUDES})
source_group("tools\\debugger" FILES ${EDITOR_DEBUGGER_SOURCES})

source_group("tools\\decl" FILES ${EDITOR_DECL_INCLUDES})
source_group("tools\\decl" FILES ${EDITOR_DECL_SOURCES})

source_group("tools\\guied" FILES ${EDITOR_GUIED_INCLUDES})
source_group("tools\\guied" FILES ${EDITOR_GUIED_SOURCES})

source_group("tools\\materialeditor" FILES ${EDITOR_MATERIALEDITOR_INCLUDES})
source_group("tools\\materialeditor" FILES ${EDITOR_MATERIALEDITOR_SOURCES})

source_group("tools\\particle" FILES ${EDITOR_PARTICLE_INCLUDES})
source_group("tools\\particle" FILES ${EDITOR_PARTICLE_SOURCES})

source_group("tools\\pda" FILES ${EDITOR_PDA_INCLUDES})
source_group("tools\\pda" FILES ${EDITOR_PDA_SOURCES})

source_group("tools\\radiant" FILES ${EDITOR_RADIANT_INCLUDES})
source_group("tools\\radiant" FILES ${EDITOR_RADIANT_SOURCES})

source_group("tools\\script" FILES ${EDITOR_SCRIPT_INCLUDES})
source_group("tools\\script" FILES ${EDITOR_SCRIPT_SOURCES})

source_group("tools\\sound" FILES ${EDITOR_SOUND_INCLUDES})
source_group("tools\\sound" FILES ${EDITOR_SOUND_SOURCES})


set(DOOM3_INCLUDES
			${AAS_INCLUDES}
			${CM_INCLUDES}
			${FRAMEWORK_INCLUDES}
			${FRAMEWORK_ASYNC_INCLUDES}
			${RENDERER_INCLUDES}
			${RENDERER_COLOR_INCLUDES}
			${RENDERER_DXT_INCLUDES}
			${RENDERER_JOBS_INCLUDES}
			${RENDERER_JOBS_DYNAMICSHADOWVOLUME_INCLUDES}
			${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_INCLUDES}
			${RENDERER_JOBS_STATICSHADOWVOLUME_INCLUDES}
			${RENDERER_OPENGL_INCLUDES}
			#${IRRXML_INCLUDES}
			${JPEG_INCLUDES}
			${PNG_INCLUDES}
			${ZLIB_INCLUDES}
			${MINIZIP_INCLUDES}
			${GLEW_INCLUDES}
			#${FREETYPE_SOURCES}
			${SOUND_INCLUDES}
			#${OGGVORBIS_INCLUDES}
			${UI_INCLUDES}
			${VR_INCLUDES}
			${SWF_INCLUDES}
			${COMMON_INCLUDES}

			${DOOMCLASSIC_INCLUDES}
			${DOOMCLASSIC_R_INCLUDES}
			${DOOMCLASSIC_VERSIONING_INCLUDES}
			${DOOMCLASSIC_BFG_INCLUDES}
			${DOOMCLASSIC_FRAGGLE_INCLUDES}
			${DOOMCLASSIC_G_DOOM_INCLUDES}
			${DOOMCLASSIC_G_HERETIC_INCLUDES}
			${DOOMCLASSIC_G_HEXEN_INCLUDES}
			${DOOMCLASSIC_G_RAVEN_INCLUDES}
			${DOOMCLASSIC_G_SHARED_INCLUDES}
			${DOOMCLASSIC_G_STRIFE_INCLUDES}
			${DOOMCLASSIC_GL_INCLUDES}
			${DOOMCLASSIC_INTERMISSION_INCLUDES}
			${DOOMCLASSIC_MENU_INCLUDES}
			${DOOMCLASSIC_OPLSYNTH_INCLUDES}
			#${DOOMCLASSIC_POSIX_INCLUDES}
			${DOOMCLASSIC_R_DATA_INCLUDES}
			${DOOMCLASSIC_RESOURCE_INCLUDES}
			#${DOOMCLASSIC_SDL_INCLUDES}
			${DOOMCLASSIC_SFMT_INCLUDES}
			${DOOMCLASSIC_SOUND_INCLUDES}
			${DOOMCLASSIC_TEXTURES_INCLUDES}
			${DOOMCLASSIC_THINGDEF_INCLUDES}
			${DOOMCLASSIC_WILDMIDI_INCLUDES}
			${DOOMCLASSIC_WIN32_INCLUDES}
			${DOOMCLASSIC_XLAT_INCLUDES}
			${DOOMCLASSIC_BZIP2_INCLUDES}
			${DOOMCLASSIC_DUMB_INCLUDES}
			${DOOMCLASSIC_GDTOA_INCLUDES}
			${DOOMCLASSIC_GME_INCLUDES}
			${DOOMCLASSIC_LZMA_INCLUDES}
			${TIMIDITY_INCLUDES}
			
			${COMPILER_INCLUDES}
			${COMPILER_AAS_INCLUDES} ${COMPILER_AAS_SOURCES}
			${COMPILER_DMAP_INCLUDES} ${COMPILER_DMAP_SOURCES}
			#${COMPILER_RENDERBUMP_INCLUDES} ${COMPILER_RENDERBUMP_SOURCES}
			#${COMPILER_ROQVQ_INCLUDES} ${COMPILER_ROQVQ_SOURCES}
			)
			
set(DOOM3_SOURCES
			${AAS_SOURCES}
			${CM_SOURCES}
			${FRAMEWORK_SOURCES}
			${FRAMEWORK_ASYNC_SOURCES} 
			${RENDERER_SOURCES}
			${RENDERER_COLOR_SOURCES}
			${RENDERER_DXT_SOURCES}
			${RENDERER_JOBS_SOURCES}
			${RENDERER_JOBS_DYNAMICSHADOWVOLUME_SOURCES}
			${RENDERER_JOBS_PRELIGHTSHADOWVOLUME_SOURCES}
			${RENDERER_JOBS_STATICSHADOWVOLUME_SOURCES}
			${RENDERER_OPENGL_SOURCES}
			#${IRRXML_SOURCES}
			${JPEG_SOURCES}
			${PNG_SOURCES}
			${ZLIB_SOURCES}
			${MINIZIP_SOURCES}
			${GLEW_SOURCES}
			#${FREETYPE_SOURCES}
			${SOUND_SOURCES}
			#${OGGVORBIS_INCLUDES} ${OGGVORBIS_SOURCES}
			${UI_SOURCES}
			${VR_SOURCES}
			${SWF_SOURCES}
			${COMMON_SOURCES}

			${DOOMCLASSIC_SOURCES}
			${DOOMCLASSIC_R_SOURCES}
			${DOOMCLASSIC_VERSIONING_SOURCES}
			${DOOMCLASSIC_BFG_SOURCES}
			${DOOMCLASSIC_FRAGGLE_SOURCES}
			${DOOMCLASSIC_G_DOOM_SOURCES}
			${DOOMCLASSIC_G_HERETIC_SOURCES}
			${DOOMCLASSIC_G_HEXEN_SOURCES}
			${DOOMCLASSIC_G_RAVEN_SOURCES}
			${DOOMCLASSIC_G_SHARED_SOURCES}
			${DOOMCLASSIC_G_STRIFE_SOURCES}
			${DOOMCLASSIC_GL_SOURCES}
			${DOOMCLASSIC_INTERMISSION_SOURCES}
			${DOOMCLASSIC_MENU_SOURCES}
			${DOOMCLASSIC_OPLSYNTH_SOURCES}
			#${DOOMCLASSIC_POSIX_SOURCES}
			${DOOMCLASSIC_R_DATA_SOURCES}
			${DOOMCLASSIC_RESOURCE_SOURCES}
			#${DOOMCLASSIC_SDL_SOURCES}
			${DOOMCLASSIC_SFMT_SOURCES}
			${DOOMCLASSIC_SOUND_SOURCES}
			${DOOMCLASSIC_TEXTURES_SOURCES}
			${DOOMCLASSIC_THINGDEF_SOURCES}
			${DOOMCLASSIC_WILDMIDI_SOURCES}
			${DOOMCLASSIC_WIN32_SOURCES}
			${DOOMCLASSIC_XLAT_SOURCES}
			${DOOMCLASSIC_BZIP2_SOURCES}
			${DOOMCLASSIC_DUMB_SOURCES}
			${DOOMCLASSIC_GDTOA_SOURCES}
			${DOOMCLASSIC_GME_SOURCES}
			${DOOMCLASSIC_LZMA_SOURCES}
			${TIMIDITY_SOURCES}
			
			${COMPILER_AAS_SOURCES}
			${COMPILER_DMAP_SOURCES}
			#${COMPILER_RENDERBUMP_SOURCES}
			#${COMPILER_ROQVQ_SOURCES}
			)

add_definitions(-DUSE_DOOMCLASSIC)

add_definitions(-DNOASM)
add_definitions(-D__DOOM__
				#-DBUILD_FREETYPE
				#-DFT2_BUILD_LIBRARY
				)
				
if(MONOLITH)
	list(APPEND DOOM3_INCLUDES
		${GAMED3XP_INCLUDES}
		${GAMED3XP_AI_INCLUDES} 
		${GAMED3XP_ANIM_INCLUDES} 
		${GAMED3XP_GAMESYS_INCLUDES} 
		${GAMED3XP_MENUS_INCLUDES}
		${GAMED3XP_PHYSICS_INCLUDES}
		${GAMED3XP_SCRIPT_INCLUDES})
		
	list(APPEND DOOM3_SOURCES
		${GAMED3XP_SOURCES}
		${GAMED3XP_AI_SOURCES}
		${GAMED3XP_ANIM_SOURCES}
		${GAMED3XP_GAMESYS_SOURCES}
		${GAMED3XP_MENUS_SOURCES}
		${GAMED3XP_PHYSICS_SOURCES}
		${GAMED3XP_SCRIPT_SOURCES})
else()
	add_definitions(-D__DOOM_DLL__)
endif()

include_directories(
			.
			idlib
			#libs/curl/include
			#libs/openal/include
			#libs/freetype/include
            )
			
#link_directories(
#			${CMAKE_CURRENT_SOURCE_DIR}/libs/curl/lib
#			${CMAKE_CURRENT_SOURCE_DIR}/libs/openal/lib)

if(MSVC)

	# if(MSVC_VERSION EQUAL 1700)
		# set(DirectX_LIBRARIES
			# dinput8
			# dsound
			# dxguid
			# #dxerr
			# Xinput
			# xaudio2.lib
			# )
		
		# if(NOT OPENAL)
			# list(APPEND DirectX_LIBRARIES xaudio2.lib)
		# endif()
		
	# else()
		find_package(DirectX REQUIRED)
		include_directories(${DirectX_INCLUDE_DIR})
	#endif()
	
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/LibOVR/include)
			
	link_directories(
			${CMAKE_CURRENT_SOURCE_DIR}/libs/LibOVR/Lib/Windows/${WIN32_X64}/Release/VS2013
			${CMAKE_CURRENT_SOURCE_DIR}/libs/LibOVR/Lib/${WIN32_X64}/VS2013)
					
	set(LibOVR_LIBRARIES
		libovr.lib
		ws2_32.lib)
				
	
	if(OPENAL)
		add_definitions(-DUSE_OPENAL)
	
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/include)
		
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/${WIN32_WIN64})
		
		list(APPEND DOOM3_INCLUDES ${OPENAL_INCLUDES})
		list(APPEND DOOM3_SOURCES
			${OPENAL_SOURCES}
			${DOOMCLASSIC_OPENAL_SOURCES})
		
		set(OpenAL_LIBRARIES
			OpenAL32)
			
		install(FILES libs/openal-soft/lib/${WIN32_WIN64}/OpenAL${X32_64}.dll DESTINATION .)
		if(CMAKE_CL_64)
		else()
			install(FILES libs/openal-soft/lib/${WIN32_WIN64}/OpenAL${X32_64}.pdb DESTINATION .)
		endif()
	else()
		add_definitions(-DNO_OPENAL)
		
		list(APPEND DOOM3_INCLUDES ${XAUDIO2_INCLUDES})
		list(APPEND DOOM3_SOURCES
			${XAUDIO2_SOURCES}
			${DOOMCLASSIC_XAUDIO2_SOURCES})
	endif()
	
	if(FFMPEG)
		add_definitions(-DUSE_FFMPEG)
		
		include_directories(libs/ffmpeg-${WIN32_WIN64}/include)
		include_directories(libs/ffmpeg-${WIN32_WIN64}/include/libswscale)
		include_directories(libs/ffmpeg-${WIN32_WIN64}/include/libavformat)
		include_directories(libs/ffmpeg-${WIN32_WIN64}/include/libavdevice)
		include_directories(libs/ffmpeg-${WIN32_WIN64}/include/libavcodec)
		
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/ffmpeg-${WIN32_WIN64}/lib)
		
		set(FFmpeg_LIBRARIES
				avcodec
				avformat
				avutil
				swscale)
		
	endif()
	
	if(USE_MFC_TOOLS)
		
		include_directories(libs/atlmfc/include)
		if(CMAKE_CL_64)
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/atlmfc/lib/amd64)
		else()
			link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/atlmfc/lib)
		endif()
		
		add_definitions(-DUSE_MFC_TOOLS)
		
		list(APPEND DOOM3_SOURCES
			${TOOLS_INCLUDES}
			${TOOLS_COMAFX_INCLUDES} ${TOOLS_COMAFX_SOURCES}
			${TOOLS_COMMON_INCLUDES} ${TOOLS_COMMON_SOURCES}
			${TOOLS_COMMON_PROPTREE_INCLUDES} ${TOOLS_COMMON_PROPTREE_SOURCES}
			
			${EDITOR_AF_INCLUDES} ${EDITOR_AF_SOURCES}
			${EDITOR_DEBUGGER_INCLUDES} ${EDITOR_DEBUGGER_SOURCES}
			${EDITOR_DECL_INCLUDES} ${EDITOR_DECL_SOURCES}
			${EDITOR_GUIED_INCLUDES} ${EDITOR_GUIED_SOURCES}
			${EDITOR_MATERIALEDITOR_INCLUDES} ${EDITOR_MATERIALEDITOR_SOURCES}
			${EDITOR_PARTICLE_INCLUDES} ${EDITOR_PARTICLE_SOURCES}
			${EDITOR_PDA_INCLUDES} ${EDITOR_PDA_SOURCES}
			${EDITOR_RADIANT_INCLUDES} ${EDITOR_RADIANT_SOURCES}
			${EDITOR_SCRIPT_INCLUDES} ${EDITOR_SCRIPT_SOURCES}
			${EDITOR_SOUND_INCLUDES} ${EDITOR_SOUND_SOURCES})
	endif()
	
	list(APPEND DOOM3_INCLUDES
		${SYS_INCLUDES} 
		${WIN32_INCLUDES})
		
	list(APPEND DOOM3_SOURCES
		${SYS_SOURCES}
		${WIN32_SOURCES})

	list(REMOVE_DUPLICATES DOOM3_SOURCES)
	
	set(DOOM3_PRECOMPILED_SOURCES ${DOOM3_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${TIMIDITY_SOURCES} ${JPEG_SOURCES} ${PNG_SOURCES} ${ZLIB_SOURCES} ${GLEW_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib/minizip/ioapi.c)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTDecoder.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder_SSE2.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/dynamicshadowvolume/DynamicShadowVolume.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/prelightshadowvolume/PreLightShadowVolume.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/staticshadowvolume/StaticShadowVolume.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/ShadowShared.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/RenderLog.cpp)
        list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/framework/precompiled.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_R_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_BFG_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_FRAGGLE_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_G_DOOM_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_G_HERETIC_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_G_HEXEN_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_G_RAVEN_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_G_SHARED_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_G_STRIFE_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_GL_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_INTERMISSION_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_MENU_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_OPLSYNTH_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_R_DATA_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_RESOURCE_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_SFMT_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_SOUND_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_TEXTURES_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_THINGDEF_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_WIN32_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_WILDMIDI_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_XLAT_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_BZIP2_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_DUMB_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_GDTOA_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_GME_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${DOOMCLASSIC_LZMA_SOURCES})

	
        set_source_files_properties(
            ${DOOM3_PRECOMPILED_SOURCES}
            PROPERTIES
            COMPILE_FLAGS "/Yuprecompiled.h"
            OBJECT_DEPENDS "precompiled.pch"

            )
	
	set_source_files_properties(framework/precompiled.cpp
        PROPERTIES
        COMPILE_FLAGS "/Ycprecompiled.h"
        OBJECT_OUTPUTS "precompiled.pch"
        )
	
	list(APPEND DOOM3_SOURCES ${WIN32_RESOURCES})
	
	add_executable(Doom3BFGVR WIN32 ${DOOM3_INCLUDES} ${DOOM3_SOURCES})
		
	#add_custom_command(TARGET Doom3BFGVR
    #               PRE_BUILD
	#			   COMMAND nmake ARGS /f Makefile.vc6 CFG=release
	#			   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/curl/lib
    #               COMMENT "Compiling libcURL")

	# copy OpenVR dlls
	add_custom_command(TARGET Doom3BFGVR
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/libs/openvr/bin/${WIN32_WIN64}/openvr_api.dll ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
	# add_custom_command(TARGET Doom3BFGVR
	#		POST_BUILD
	#		COMMAND ${CMAKE_COMMAND} -E copy
	#		${CMAKE_CURRENT_SOURCE_DIR}/libs/openvr/bin/${WIN32_WIN64}/openvr_api.pdb ${CMAKE_CURRENT_BINARY_DIR}/Debug)
	# add_custom_command(TARGET Doom3BFGVR
	#		POST_BUILD
	#		COMMAND ${CMAKE_COMMAND} -E copy
	#		${CMAKE_CURRENT_SOURCE_DIR}/libs/openvr/bin/${WIN32_WIN64}/openvr_api.pdb ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo)
	# copy FMOD dlls
	add_custom_command(TARGET Doom3BFGVR
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_CURRENT_SOURCE_DIR}/libs/fmod/fmodex${X64}.dll ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})

	add_dependencies(Doom3BFGVR idlib)
	target_link_libraries(Doom3BFGVR
		idlib
		${DirectX_LIBRARIES}
		#libcurl 
		#openal32 
		opengl32
		glu32
		dbghelp
		#eaxguid
		iphlpapi
		comctl32
		winmm
		wsock32.lib
		${OpenAL_LIBRARIES}
		${FFmpeg_LIBRARIES}
        ${ZLIB_LIBRARY}
        ${PNG_LIBRARY}
        ${JPEG_LIBRARY}
        ${GLEW_LIBRARY}
        ${OPENVR_LIBRARY}
		${LibOVR_LIBRARIES}
        ${FMOD_LIBRARY}
		sapi.lib
		)
		
	#CMAKE_BINARY_DIR
	if(CMAKE_CL_64)
		install(TARGETS Doom3BFGVR
			RUNTIME DESTINATION .)
	else()
		install(TARGETS Doom3BFGVR
			RUNTIME DESTINATION .)
	endif()
	
else()

	list(APPEND DOOM3_SOURCES
		${SYS_INCLUDES} ${SYS_SOURCES})
	
	find_package(OpenGL REQUIRED)
	include_directories(${OPENGL_INCLUDE_DIRS})

	if(UNIX)
		if(FFMPEG)
			find_package(FFMPEG REQUIRED)
			add_definitions(-DUSE_FFMPEG)
			
			include_directories(${FFMPEG_INCLUDE_DIR})
			link_directories(${FFMPEG_LIBRARIES_DIRS})
		endif()

		if(SDL2)
			find_package(SDL2 REQUIRED)
			include_directories(${SDL2_INCLUDE_DIR})
			set(SDLx_LIBRARY ${SDL2_LIBRARY})
		else()
			find_package(SDL REQUIRED)
			include_directories(${SDL_INCLUDE_DIR})
			set(SDLx_LIBRARY ${SDL_LIBRARY})
		endif()

		if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
			list(REMOVE_ITEM POSIX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_linux.cpp)
		else()
			list(REMOVE_ITEM POSIX_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/sys/posix/platform_osx.cpp)
		endif()

		list(APPEND DOOM3_SOURCES
			${POSIX_INCLUDES} ${POSIX_SOURCES}
			${SDL_INCLUDES} ${SDL_SOURCES})
			
		if(OPENAL)
			find_package(OpenAL REQUIRED)
			add_definitions(-DUSE_OPENAL)

			#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/include)
			
			#if(CMAKE_CL_64)
			#	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win64)
			#else()
			#	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/openal-soft/lib/win32)
			#endif()
			
			list(APPEND DOOM3_INCLUDES ${OPENAL_INCLUDES})
			list(APPEND DOOM3_SOURCES
				${OPENAL_SOURCES}
				${DOOMCLASSIC_OPENAL_SOURCES})
		else()
			list(APPEND DOOM3_INCLUDES ${STUBAUDIO_INCLUDES})
			list(APPEND DOOM3_SOURCES
				${STUBAUDIO_SOURCES}
				${DOOMCLASSIC_STUBAUDIO_SOURCES})
		endif()
			
	endif()

	list(REMOVE_DUPLICATES DOOM3_SOURCES)
	
	set(DOOM3_PRECOMPILED_SOURCES ${DOOM3_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${TIMIDITY_SOURCES} ${JPEG_SOURCES} ${PNG_SOURCES} ${ZLIB_SOURCES} ${GLEW_SOURCES})
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/libs/zlib/minizip/ioapi.c)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTDecoder.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/DXT/DXTEncoder_SSE2.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/dynamicshadowvolume/DynamicShadowVolume.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/prelightshadowvolume/PreLightShadowVolume.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/staticshadowvolume/StaticShadowVolume.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/jobs/ShadowShared.cpp)
	list(REMOVE_ITEM DOOM3_PRECOMPILED_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/renderer/RenderLog.cpp)

	foreach( src_file ${DOOM3_PRECOMPILED_SOURCES} )
		#message(STATUS "-include precompiled.h for ${src_file}")
		set_source_files_properties(
			${src_file}
			PROPERTIES
			COMPILE_FLAGS "-include ${CMAKE_CURRENT_SOURCE_DIR}/idlib/precompiled.h"
			)
	endforeach()

	# precompiled magic for GCC/clang, adapted from https://gist.github.com/573926
	STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
	SET(_compiler_FLAGS ${${_flags_var_name}})
	GET_DIRECTORY_PROPERTY(_directory_flags INCLUDE_DIRECTORIES)
	FOREACH(item ${_directory_flags})
		LIST(APPEND _compiler_FLAGS " -I${item}")
	ENDFOREACH(item)

	GET_DIRECTORY_PROPERTY(_directory_flags DEFINITIONS)
	LIST(APPEND _compiler_FLAGS ${_directory_flags})
	SEPARATE_ARGUMENTS(_compiler_FLAGS)
	
	# we need to recreate the precompiled header for Doom3BFGVR 
	# (i.e. can't use the one created for idlib before)
	# because some definitions (e.g. -D__IDLIB__ -D__DOOM_DLL__) differ
	add_custom_target(precomp_header_doom3bfgvr ALL
	                  COMMAND ${CMAKE_CXX_COMPILER} ${_compiler_FLAGS} -x c++-header idlib/precompiled.h -o idlib/precompiled.h.gch
	                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	                  COMMENT "Creating idlib/precompiled.h.gch for Doom3BFGVR"
	                  )
	
	if(WIN32)
		set(remove_command "del")
	else()
		set(remove_command "rm")
	endif()
	# it's ugly enough that the precompiled header binary needs to be in the 
	# source directory (instead of the build directory), so let's at least
	# delete it after build.
	add_custom_target(rm_precomp_header ALL
	                  COMMAND ${remove_command} "idlib/precompiled.h.gch"
	                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	                  COMMENT "remove idlib/precompiled.h.gch"
	                  )
	
	# make sure this is run after creating idlib
	add_dependencies(precomp_header_doom3bfgvr idlib)
	
	add_executable(Doom3BFGVR WIN32 ${DOOM3_SOURCES})
	
	# make sure precompiled header is created before executable is compiled
	add_dependencies(Doom3BFGVR precomp_header_doom3bfgvr)
	
	# make sure precompiled header is deleted after executable is compiled
	add_dependencies(rm_precomp_header Doom3BFGVR)
	

	if(NOT WIN32)
		if(NOT "${CMAKE_SYSTEM}" MATCHES "Darwin")
			set(RT_LIBRARY rt)
		endif()

		if(NOT "${CMAKE_SYSTEM}" MATCHES "FreeBSD")
			set(DL_LIBRARY dl)
		endif()

		target_link_libraries(Doom3BFGVR
			idlib
			${OPENGL_LIBRARIES}
			pthread
			${DL_LIBRARY}
			${RT_LIBRARY}
			${SDLx_LIBRARY}
			${OPENAL_LIBRARY}
			${FFMPEG_LIBRARIES}
            ${ZLIB_LIBRARY}
            ${PNG_LIBRARY}
            ${JPEG_LIBRARY}
            ${GLEW_LIBRARY}
            ${OPENVR_LIBRARY}
            ${FMOD_LIBRARY}
			)
	endif()
    
endif()
