#version 330 core

in vec3 fNormSCO;
in vec3 fVertexSCO;

in vec3 fmatamb;
in vec3 fmatdiff;
in vec3 fmatspec;
in float fmatshin;

//TEXTURAS
in vec2 fUV;
uniform int textActive;
uniform sampler2D text;

//SOL
uniform vec3 colorFocus;
uniform vec3 posFocus; //en SCO

// Foco 2: Linterna del Personaje (Amarilla)
uniform vec3 posLanternaSCO; // en SCO (pos Morty)

// Foco 3: Luz del Fantasma
uniform vec3 posFantasmaSCO; // en SCO

uniform bool modeNit;

//LucesMonedas
#define NUM_MONEDES 10
uniform int quantitatMonedes;
uniform vec3 posMonedesSCO[NUM_MONEDES];
uniform vec3 dirMonedesSCO[NUM_MONEDES];
uniform vec3 colorMonedaLlum;

out vec4 FragColor;

vec3 Ambient(vec3 matamb) {
    return vec3(0.3, 0.3, 0.3) * matamb;
}

vec3 Difus (vec3 NormSCO, vec3 L, vec3 colFocus, vec3 matdiff) {
    vec3 colRes = vec3(0.0);
    if (dot (L, NormSCO) > 0)
        colRes = colFocus * matdiff * dot (L, NormSCO);
    return (colRes);
}

vec3 Especular (vec3 NormSCO, vec3 L, vec3 vertSCO, vec3 colFocus, vec3 matspec, float matshin) {
    vec3 colRes = vec3 (0.0);
    if ((dot(NormSCO,L) < 0) || (matshin == 0))
        return colRes;
    vec3 R = reflect(-L, NormSCO);
    vec3 V = normalize(-vertSCO);
    if (dot(R, V) < 0)
        return colRes;
    return (matspec * colFocus * pow(max(0.0, dot(R, V)), matshin));
}


void main()
{
        vec3 N = normalize(fNormSCO);

        vec3 matAmbientEfectiu = fmatamb;

        // textura
            vec3 matDifusEfectiu = fmatdiff;
            if (textActive == 1) {
                matDifusEfectiu = texture(text, fUV).xyz;
            }

        vec3 colorFinal = Ambient(matAmbientEfectiu);

        if (!modeNit) {
            vec3 L = normalize(posFocus - fVertexSCO);
            colorFinal += Difus(N, L, colorFocus, matDifusEfectiu) + Especular(N, L, fVertexSCO, colorFocus, fmatspec, fmatshin);
        } else {
            vec3 colorLanterna = vec3(1.0, 1.0, 0.0);
            vec3 L_morty = normalize(posLanternaSCO - fVertexSCO);
            colorFinal += Difus(N, L_morty, colorLanterna, matDifusEfectiu) + Especular(N, L_morty, fVertexSCO, colorLanterna, fmatspec, fmatshin);

            vec3 colorFantasma = vec3(0.5, 0.8, 1.0);
            vec3 L_fantasma = normalize(posFantasmaSCO - fVertexSCO);
            colorFinal += Difus(N, L_fantasma, colorFantasma, matDifusEfectiu) + Especular(N, L_fantasma, fVertexSCO, colorFantasma, fmatspec, fmatshin);
        }

        // Luces de las monedas
           for (int i = 0; i < quantitatMonedes; ++i) {
               vec3 raigLlum = posMonedesSCO[i] - fVertexSCO;
               float distancia = length(raigLlum);
               if (distancia < 4.0) {
                   vec3 L = normalize(raigLlum);
                   vec3 d = normalize(dirMonedesSCO[i]);
                   float efecteFocus = pow(max(0.0, dot(-L, d)), 4.0);
                   float atenuacioDistancia = 1.0 / (1.0 + exp(3.0 * (distancia - 2.0)));
                   colorFinal += (Difus(N, L, colorMonedaLlum, matDifusEfectiu) + Especular(N, L, fVertexSCO, colorMonedaLlum, fmatspec, fmatshin)) * efecteFocus * atenuacioDistancia;
               }
           }

        FragColor = vec4(colorFinal, 1.0);
}

