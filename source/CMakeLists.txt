##==============================================================================
## General.
##==============================================================================
cmake_minimum_required(VERSION 3.19)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/UnigineCMakeVariables.cmake)

project(unigine-noesis-cpp-integration-sample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

##==============================================================================
## Environment.
##==============================================================================
set(UNIGINE_DOUBLE False CACHE BOOL "Double coords")
set(UNIGINE_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/../)
# Multi-sample repo fallback: if the per-sample SDK payload was not restored by
# SDK Browser (Repair), use the shared SDK at the repository root (two levels up).
# SDK Browser users (per-sample SDK present) are unaffected.
if(NOT EXISTS "${UNIGINE_SDK_PATH}include/UnigineEngine.h")
	get_filename_component(_repo_root "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
	set(UNIGINE_SDK_PATH "${_repo_root}/")
endif()
set(UNIGINE_BIN_DIR ${UNIGINE_SDK_PATH}bin)
set(UNIGINE_LIB_DIR ${UNIGINE_SDK_PATH}lib)
set(UNIGINE_INCLUDE_DIR ${UNIGINE_SDK_PATH}include)

##==============================================================================
## Dependencies.
##==============================================================================
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# Engine.
find_package(Engine REQUIRED MODULE QUIET)

# NoesisGUI SDK (external): resolved from an in-tree drop or the shared 3rdparty
# cache ($ENV{UNIGINE_3RDPARTY_DIR}). Skip the sample gracefully when absent.
find_package(NoesisGUI MODULE QUIET)
if(NOT NoesisGUI_FOUND)
	message(STATUS "\n======================================\n NoesisGUI SDK not found. unigine-noesis-cpp-integration-sample skipped.\n======================================\n")
	return()
endif()

##==============================================================================
## Target.
##==============================================================================
set(target "unigine-noesis-cpp-integration-sample")

if (WIN32)
	# Console app in Debug; windowed in Release. The app entry point is main().
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

# Vendored NoesisGUI "App framework" interactivity sources (compiled into the app).
# NOTE: this list mirrors NoesisGUI.vcxproj exactly and intentionally EXCLUDES
# Src/.../MediaActions.cpp (present on disk but not part of the sample build).
set(nsapp_dir ${CMAKE_CURRENT_LIST_DIR}/Src/Packages/App/Interactivity)
set(nsapp_sources
	App.Interactivity.cpp
	AttachableObject.cpp
	BackgroundEffectBehavior.cpp
	Behavior.cpp
	BehaviorCollection.cpp
	ChangePropertyAction.cpp
	CollectionFilterBehavior.cpp
	CollectionSortBehavior.cpp
	ComparisonCondition.cpp
	ComparisonLogic.cpp
	ConditionalExpression.cpp
	ConditionBehavior.cpp
	ControlStoryboardAction.cpp
	DataBindingHelper.cpp
	DataEventTrigger.cpp
	DataTrigger.Interactivity.cpp
	EventTrigger.Interactivity.cpp
	EventTriggerBase.cpp
	FilterPredicate.cpp
	GamepadTrigger.cpp
	GoToStateAction.cpp
	Interaction.cpp
	InvokeCommandAction.cpp
	KeyTrigger.cpp
	LaunchUriOrFileAction.cpp
	LineDecorationBehavior.cpp
	LoadContentAction.cpp
	MouseDragElementBehavior.cpp
	MoveFocusAction.cpp
	PlaySoundAction.cpp
	PropertyChangedTrigger.cpp
	RemoveElementAction.cpp
	SelectAction.cpp
	SelectAllAction.cpp
	SetFocusAction.cpp
	SortComparer.cpp
	StoryboardAction.cpp
	StoryboardCompletedTrigger.cpp
	StoryboardTrigger.cpp
	StringFilterPredicate.cpp
	StringSortComparer.cpp
	StyleInteraction.cpp
	TargetedTriggerAction.cpp
	TimerTrigger.cpp
	TranslateZoomRotateBehavior.cpp
	TriggerAction.Interactivity.cpp
	TriggerActionCollection.cpp
	TriggerBase.Interactivity.cpp
	TriggerCollection.cpp
	)
list(TRANSFORM nsapp_sources PREPEND ${nsapp_dir}/Src/)

add_executable(${target}
		${CMAKE_CURRENT_LIST_DIR}/main.cpp

		${CMAKE_CURRENT_LIST_DIR}/NoesisIntegration.cpp
		${CMAKE_CURRENT_LIST_DIR}/NoesisIntegration.h
		${CMAKE_CURRENT_LIST_DIR}/NoesisView.cpp
		${CMAKE_CURRENT_LIST_DIR}/NoesisView.h
		${CMAKE_CURRENT_LIST_DIR}/NoesisRenderDevice.cpp
		${CMAKE_CURRENT_LIST_DIR}/NoesisRenderDevice.h
		${CMAKE_CURRENT_LIST_DIR}/NoesisTexture.cpp
		${CMAKE_CURRENT_LIST_DIR}/NoesisTexture.h
		${CMAKE_CURRENT_LIST_DIR}/NoesisProviders.cpp
		${CMAKE_CURRENT_LIST_DIR}/NoesisProviders.h
		${CMAKE_CURRENT_LIST_DIR}/NoesisShader.h
		${CMAKE_CURRENT_LIST_DIR}/NoesisDataContext.cpp
		${CMAKE_CURRENT_LIST_DIR}/NoesisDataContext.h
		${CMAKE_CURRENT_LIST_DIR}/ObjectNoesisGui.cpp
		${CMAKE_CURRENT_LIST_DIR}/ObjectNoesisGui.h

		${nsapp_sources}
)

target_include_directories(${target}
	PRIVATE
	${UNIGINE_INCLUDE_DIR}
	${nsapp_dir}/Include
	)

target_link_libraries(${target}
	PRIVATE
	Unigine::Engine
	UnigineExt::Noesis
	)

target_compile_definitions(${target}
	PRIVATE
	UNICODE
	_UNICODE
	# The vendored App interactivity headers use this as their DLL export macro;
	# define it empty because those sources are compiled straight into the app.
	NS_APP_INTERACTIVITY_API=
	$<$<BOOL:${UNIX}>:_LINUX>
	$<$<CONFIG:Debug>:DEBUG>
	$<$<NOT:$<CONFIG:Debug>>:NDEBUG>
	)

##==============================================================================
## Compiler constants.
##==============================================================================
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(UNIGINE_COMPILER_IS_MSVC TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(UNIGINE_COMPILER_IS_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(UNIGINE_COMPILER_IS_GNU TRUE)
endif()

if (UNIGINE_COMPILER_IS_MSVC)
    target_compile_definitions(${target}
	PRIVATE
	_CRT_SECURE_NO_DEPRECATE
	)
    include(ProcessorCount)
    ProcessorCount(proc_count)
    target_compile_options(${target}
	PRIVATE
	/TP               # Specifies all source files are C++.
	/FS               # Forces writes to the program database (PDB) file to be serialized through MSPDBSRV.EXE.
	/fp:fast          # Specifies floating-point behavior.
	/bigobj           # Increases the number of addressable sections in an .obj file.
	/EHsc             # Specifies the kind of exception handling used by the compiler.
	/MP${proc_count}  # Build with Multiple Processes.
	)
    unset(proc_count)
    target_link_options(${target} INTERFACE "/FIXED:NO")

elseif(UNIGINE_COMPILER_IS_GNU OR UNIGINE_COMPILER_IS_CLANG)

    target_compile_options(${target}
	PRIVATE
	-m64
	-march=athlon64
	-msse4.2
	# Optimizations.
	-ffast-math
	-fno-strict-aliasing
	$<$<NOT:$<CONFIG:Debug>>:-fomit-frame-pointer>
	# Warnings.
	$<$<CONFIG:Debug>:-Wall>
	$<$<CONFIG:Debug>:-Wno-unknown-pragmas>
	$<$<CONFIG:Debug>:-Wno-unused-parameter>
	)
endif ()

set(binary_name ${target})
if (UNIGINE_DOUBLE)
	string(APPEND binary_name "_double")
endif()
string(APPEND binary_name "_x64")
set_target_properties(${target} PROPERTIES DEBUG_POSTFIX "d")
set_target_properties(${target} PROPERTIES OUTPUT_NAME ${binary_name})

set_target_properties(${target}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${UNIGINE_BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_DEBUG ${UNIGINE_BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELEASE ${UNIGINE_BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${UNIGINE_BIN_DIR}
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${UNIGINE_BIN_DIR}
	)

# Deploy the NoesisGUI runtime (Noesis.dll / libNoesis.so) next to the app.
if(NoesisGUI_DLL)
	get_filename_component(_noesis_runtime_name "${NoesisGUI_DLL}" NAME)
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${NoesisGUI_DLL}"
			"${UNIGINE_BIN_DIR}/${_noesis_runtime_name}"
	)
endif()

unset(UNIGINE_DOUBLE CACHE)
