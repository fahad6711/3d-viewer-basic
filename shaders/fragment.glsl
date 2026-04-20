#version 330 core

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;

uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;
uniform vec3 uObjectColor;

out vec4 FragColor;

void main() {
    // Ambient
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * uLightColor;

    // Diffuse (Lambertian)
    vec3  norm     = normalize(vNormal);
    vec3  lightDir = normalize(uLightPos - vFragPos);
    float diff     = max(dot(norm, lightDir), 0.0);
    vec3  diffuse  = diff * uLightColor;

    // Specular (Blinn-Phong)
    float specularStrength = 0.5;
    vec3  viewDir    = normalize(uViewPos - vFragPos);
    vec3  halfwayDir = normalize(lightDir + viewDir);
    float spec       = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3  specular   = specularStrength * spec * uLightColor;

    vec3 result = (ambient + diffuse + specular) * uObjectColor;
    FragColor = vec4(result, 1.0);
}
