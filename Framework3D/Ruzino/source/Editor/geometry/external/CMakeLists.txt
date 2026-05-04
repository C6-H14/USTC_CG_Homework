add_subdirectory(glm)

set_target_properties(glm PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
set_target_properties(glm PROPERTIES ${OUTPUT_DIR})
set(GLM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glm" CACHE STRING "")
target_compile_definitions(glm INTERFACE GLM_ENABLE_EXPERIMENTAL)
set_target_properties(glm PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(glm PROPERTIES FOLDER "ThirdParty")

add_subdirectory(OpenMesh)
set_target_properties(OpenMeshCore PROPERTIES FOLDER "external")
set_target_properties(OpenMeshTools PROPERTIES FOLDER "external")
target_compile_definitions(OpenMeshCore PUBLIC -D_USE_MATH_DEFINES=1)

# libigl
set(LIBIGL_RESTRICTED_TRIANGLE ON CACHE BOOL "" FORCE)
add_subdirectory(libigl)
set_target_properties(igl_core PROPERTIES ${OUTPUT_DIR})

# tetgen (reference from geometry_nodes)
set(TETGEN_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../geometry_nodes/tetgen")
add_library(tetgen_geom STATIC 
    ${TETGEN_SOURCE_DIR}/tetgen.cxx 
    ${TETGEN_SOURCE_DIR}/tetgen.h 
    ${TETGEN_SOURCE_DIR}/predicates.cxx)
set_target_properties(tetgen_geom PROPERTIES ${OUTPUT_DIR})
target_include_directories(tetgen_geom PUBLIC ${TETGEN_SOURCE_DIR})
if(MSVC)
    target_compile_options(tetgen_geom PRIVATE /wd4477)
endif()
set_target_properties(tetgen_geom PROPERTIES FOLDER "ThirdParty")

set_target_properties(OpenMeshCore PROPERTIES FOLDER "ThirdParty")
set_target_properties(OpenMeshTools PROPERTIES FOLDER "ThirdParty")
set_target_properties(igl_core PROPERTIES FOLDER "ThirdParty")


# suppress warnings for third-party libraries
if(MSVC)
    target_compile_options(igl_core INTERFACE /wd4244 /wd4267)
    target_compile_options(triangle PRIVATE /wd4311 /wd4302)
endif()

add_subdirectory(OpenVolumeMesh)
set_target_properties(OpenVolumeMesh PROPERTIES FOLDER "ThirdParty")
set_target_properties(OpenVolumeMesh PROPERTIES ${OUTPUT_DIR})

# suppress C4910 warnings for OpenVolumeMesh
if(MSVC)
    target_compile_options(OpenVolumeMesh PUBLIC /wd4910)
endif()