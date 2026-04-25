
RUZINO_ADD_LIB(
	nodes_ui_imgui SHARED
	PUBLIC_LIBS GUI nodes_core RHI blueprints nodes_system
	PRIVATE_LIBS glfw spdlog::spdlog stb_image test_nodes glm::glm
)
