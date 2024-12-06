#version 330 core
out vec4 FragColor;

uniform int ammoCount;  

void main() {
    int segmentIndex = gl_PrimitiveID / 2;

    if (segmentIndex < ammoCount) {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    } else {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0); 
    }
}
