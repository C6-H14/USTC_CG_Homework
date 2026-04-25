add_subdirectory(external)


option(ENABLE_GEOM_USD_EXTENSION "Enable USD extension for geometry module" ON)
option(ENABLE_GPU_GEOM_ALGORITHM "Enable GPU-based geometry algorithms" OFF)

if(ENABLE_GEOM_USD_EXTENSION)
set(SKIP_LIST)
else()
set(SKIP_LIST usd_extension)
endif()

RUZINO_ADD_LIB(
	geometry 
	SHARED
	PUBLIC_LIBS OpenMeshCore spdlog::spdlog igl_core triangle::triangle glm::glm OpenVolumeMesh tetgen_geom
	PYTHON_WRAP_DIR python
	COMPILE_DEFS
		NOMINMAX 
		RUZINO_BACKEND_NVRHI
	SKIP_DIRS
	    external ${SKIP_LIST}
)

if(ENABLE_GEOM_USD_EXTENSION)
	target_compile_definitions(geometry PUBLIC GEOM_USD_EXTENSION=1)
	target_include_directories(geometry PUBLIC usd_extension/include)
	target_link_libraries(geometry PUBLIC usd usdVol hioOpenVDB usdShade usdSkel)
endif()

if(ENABLE_GPU_GEOM_ALGORITHM)
	target_compile_definitions(geometry PUBLIC GPU_GEOM_ALGORITHM=1)
	target_link_libraries(geometry PUBLIC GPUContext shaders_interface)
	target_link_libraries(geom_algorithms_test PRIVATE usdImagingGL)
endif()

target_compile_definitions(geometry PRIVATE GEOM_COMPUTE_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/source/algorithms/CS/")