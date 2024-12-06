#version 330 core
layout (location = 0) in vec2 aPos;
uniform float rotationAngle;  
uniform float vibrationEffect; 
uniform vec2 rotationPoint; 
uniform bool hydraulicsEnabled; 
uniform float aspect; 

void main() {
    float adjustedAngle;

    if (hydraulicsEnabled) {
        adjustedAngle = rotationAngle + sin(vibrationEffect) * 0.02;
    } else {
        adjustedAngle = 3.14; 
    }

    // Translate so that the rotationPoint is at (0, 0)
    mat4 translationMatrix = mat4(1.0);
    translationMatrix[3][0] = -rotationPoint.x;
    translationMatrix[3][1] = -rotationPoint.y;

    // Create the rotation matrix
    mat4 rotationMatrix = mat4(1.0f);  
    rotationMatrix[0][0] = cos(adjustedAngle);  
    rotationMatrix[0][1] = -sin(adjustedAngle);
    rotationMatrix[1][0] = sin(adjustedAngle);
    rotationMatrix[1][1] = cos(adjustedAngle);

    // Translate back to original position
    mat4 reverseTranslationMatrix = mat4(1.0);
    reverseTranslationMatrix[3][0] = rotationPoint.x;
    reverseTranslationMatrix[3][1] = rotationPoint.y;

    // Apply the combined transformations
    mat4 model = reverseTranslationMatrix * rotationMatrix * translationMatrix;

    // Output the final position after transformation
    gl_Position = model *  vec4(aPos, 0.0, 1.0);
}