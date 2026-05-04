RUZINO_ADD_LIB(
	nodes_core
	SHARED
	INC_DIR  ${PROJECT_SOURCE_DIR}/include/api ${PROJECT_SOURCE_DIR}/include/Utils
	PUBLIC_LIBS EnTT::EnTT spdlog::spdlog
	PYTHON_WRAP_DIR python
)
