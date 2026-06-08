#version 330 core

in vec3 vertex;
in vec3 normal;

in vec3 matamb;
in vec3 matdiff;
in vec3 matspec;
in float matshin;

in vec2 UV; //textura

uniform mat4 PM;
uniform mat4 VM;
uniform mat4 TG;

out vec3 fNormSCO;
out vec3 fVertexSCO;

out vec2 fUV; //textura

out vec3 fmatamb;
out vec3 fmatdiff;
out vec3 fmatspec;
out float fmatshin;

void main()
{
    mat4 MVP = VM * TG;

    fVertexSCO = (MVP * vec4(vertex,1)).xyz;

    fUV = UV; // Pasar coordenadas de textura

    mat3 NM = transpose(inverse(mat3(MVP)));
    fNormSCO = normalize(NM * normal);

    fmatamb = matamb;
    fmatdiff = matdiff;
    fmatspec = matspec;
    fmatshin = matshin;

    gl_Position = PM * vec4(fVertexSCO,1);
}
