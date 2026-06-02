
compile() {
    glslc /home/vladimir/Documents/VulkanTest/shaders/shader.vert -o /home/vladimir/Documents/VulkanTest/shaders/vert.spv
    glslc /home/vladimir/Documents/VulkanTest/shaders/shader.frag -o /home/vladimir/Documents/VulkanTest/shaders/frag.spv
}

${1}