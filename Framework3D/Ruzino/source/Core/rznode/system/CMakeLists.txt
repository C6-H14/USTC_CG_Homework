RUZINO_ADD_LIB(
	nodes_system
	SHARED
	PUBLIC_LIBS nodes_core
	PRIVATE_LIBS spdlog::spdlog
	COMPILE_DEFS 
		RUZINO_BUILD_MODULE=1
		$<$<BOOL:${ENABLE_GEOM_USD_EXTENSION}>:GEOM_USD_EXTENSION=1>
	PYTHON_WRAP_DIR python
)

# If USD extension is enabled, link geometry library for GeomPayload
if(ENABLE_GEOM_USD_EXTENSION)
	target_link_libraries(nodes_system PUBLIC geometry)
endif()

add_subdirectory(tests)