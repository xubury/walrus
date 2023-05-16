find_package(cgltf REQUIRED)

walrus_add_executable(gltf_demo SOURCES src/main.c)

target_link_libraries(gltf_demo PUBLIC walrus_engine cgltf::cgltf)
