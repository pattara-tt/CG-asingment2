#version 330 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
out vec4 colour;

// Existing Ghost Kitty material and texture uniforms
uniform vec3 materialColor;
uniform float materialAlpha;
uniform int hasDiffuseMap;
uniform int hasOpacityMap;
uniform sampler2D texture2D;
uniform sampler2D opacityTexture;

// Spotlight uniforms set from main.cpp
uniform vec3 lightColour;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 spotDirection;   // Direction from light toward scene
uniform float spotAngle;     // Full cone angle, degrees
uniform float spotBlend;     // Softness 0..1
uniform float ambientStrength;
uniform vec3 attenuation;    // (constant, linear, quadratic)

// Three additional point lights.
uniform vec3 pointLightPos[3];
uniform vec3 pointLightColor[3];
uniform float pointLightPower[3];

void main() {
    vec4 texel = (hasDiffuseMap != 0)
        ? texture(texture2D, TexCoord) : vec4(1.0);

    float alpha = texel.a * materialAlpha;
    if (hasOpacityMap != 0) {
        vec4 mask = texture(opacityTexture, TexCoord);
        alpha *= mask.r;
    }
    if (alpha < 0.35) discard; // Preserve existing cutout transparency

    vec3 n = normalize(Normal);
    vec3 toLight = lightPos - FragPos;
    float dist = length(toLight);
    vec3 L = toLight / max(dist, 0.0001);

    // Angle between spotlight forward direction and light -> fragment.
    vec3 lightToFragment = -L;
    float cosTheta = dot(normalize(spotDirection), lightToFragment);

    float halfAngle = radians(spotAngle * 0.5);
    float outerCos = cos(halfAngle);
    // Approximate Blender's Blend as a transition across the cone.
    float innerCos = cos(halfAngle * (1.0 - clamp(spotBlend, 0.0, 1.0)));
    float spotFactor = smoothstep(outerCos, max(innerCos, outerCos + 0.00001), cosTheta);

    float attenuationFactor = 1.0 /
        max(attenuation.x + attenuation.y * dist +
            attenuation.z * dist * dist, 0.0001);

    float diffuse = max(dot(n, L), 0.0);

    // Simple Blinn-Phong specular highlight
    vec3 V = normalize(viewPos - FragPos);
    vec3 H = normalize(L + V);
    float specular = pow(max(dot(n, H), 0.0), 24.0) * 0.08;
    if (diffuse <= 0.0) specular = 0.0;

    // Ambient remains visible outside the cone, like a simple world fill.
    vec3 lighting = vec3(ambientStrength)
                  + lightColour * (diffuse + specular)
                    * spotFactor * attenuationFactor;

    // Add the three Blender-style point lights (omnidirectional).
    for (int i = 0; i < 3; ++i) {
        vec3 pointVector = pointLightPos[i] - FragPos;
        float pointDistance = length(pointVector);
        vec3 pointDir = pointVector / max(pointDistance, 0.0001);
        float pointDiffuse = max(dot(n, pointDir), 0.0);
        // Smooth falloff. This is an approximation, not Blender watt units.
        float pointFalloff = 1.0 / (1.0 + 2.0 * pointDistance
                                      + 4.0 * pointDistance * pointDistance);
        lighting += pointLightColor[i] * pointLightPower[i]
                    * pointDiffuse * pointFalloff;
    }

    vec3 rgb = texel.rgb * materialColor * lighting;
    colour = vec4(rgb, 1.0);
}
