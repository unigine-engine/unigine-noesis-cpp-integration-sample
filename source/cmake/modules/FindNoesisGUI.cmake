# Copyright (C), UNIGINE. All rights reserved.

#  Find the NoesisGUI SDK.
#  Once done this will define:
#  UnigineExt::Noesis  - imported shared library target.
#  NoesisGUI_FOUND     - the SDK was found.
#  NoesisGUI_DLL       - full path to the runtime (Noesis.dll / libNoesis.so) to deploy next to the app.

set(UNIGINE_NOESIS_VERSION "3.2.13")

# Search roots, in order: an in-tree drop first, then the shared 3rdparty cache.
set(noesisroot ${UNIGINE_SDK_PATH}/source/NoesisSDK)
set(3rdnoesisroot $ENV{UNIGINE_3RDPARTY_DIR}/NoesisGUI/Cpp/${UNIGINE_NOESIS_VERSION})

# Platform-specific SDK subfolder + runtime file name.
if (WIN32)
	set(noesis_platform "windows_x86_64")
	set(noesis_runtime_name "Noesis.dll")
else ()
	set(noesis_platform "linux_x86_64")
	set(noesis_runtime_name "libNoesis.so")
endif ()

find_path(NOESIS_INCLUDE_DIR
	NAMES
		"NsRender/RenderDevice.h"
	PATHS
		${noesisroot}/Include
		${3rdnoesisroot}/Include
	NO_DEFAULT_PATH
)

if (WIN32)
	# Single shared library, one variant for all configs (matches the SDK layout).
	find_library(NOESIS_LIBRARY
		NAMES
			Noesis
		PATHS
			${noesisroot}/Lib/${noesis_platform}
			${3rdnoesisroot}/Lib/${noesis_platform}
		NO_DEFAULT_PATH
	)
endif()

find_file(NOESIS_RUNTIME
	NAMES
		${noesis_runtime_name}
	PATHS
		${noesisroot}/Bin/${noesis_platform}
		${3rdnoesisroot}/Bin/${noesis_platform}
	NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)

set(NoesisGUI_REQUIRED_VARS
	NOESIS_INCLUDE_DIR
	NOESIS_RUNTIME
)

if(WIN32)
	list(APPEND NoesisGUI_REQUIRED_VARS
		NOESIS_LIBRARY
	)
endif()

find_package_handle_standard_args(NoesisGUI
	FOUND_VAR NoesisGUI_FOUND
	REQUIRED_VARS
		${NoesisGUI_REQUIRED_VARS}
)

if (NoesisGUI_FOUND)
	set(NoesisGUI_DLL ${NOESIS_RUNTIME})

	if (NOT TARGET UnigineExt::Noesis)
		add_library(UnigineExt::Noesis SHARED IMPORTED)
		set_target_properties(UnigineExt::Noesis
			PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES ${NOESIS_INCLUDE_DIR}
			IMPORTED_LOCATION ${NOESIS_RUNTIME}
			)
		if (WIN32)
			# On Windows the link-time import library differs from the runtime DLL.
			set_target_properties(UnigineExt::Noesis
				PROPERTIES
				IMPORTED_IMPLIB ${NOESIS_LIBRARY}
				)
		endif ()
	endif ()
endif ()
