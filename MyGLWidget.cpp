
// MyGLWidget.cpp
#include "MyGLWidget.h"
#include <iostream>
#include <stdio.h>
#include<utility>
#include <vector>
#include <cstdlib>
#include <ctime>

#define CHECK() printOglError(__FILE__, __LINE__, __FUNCTION__)
#define DEBUG(text) std::cout << __FILE__ << " " << __LINE__ << " " << __FUNCTION__ << ":"<<text<<std::endl;


                                                                                MyGLWidget::MyGLWidget(QWidget *parent): BL2GLWidget(parent) {

    connect(&timer, SIGNAL(timeout()),this, SLOT(rotateCoins()));
    timer.start(16);

}

MyGLWidget::~MyGLWidget() {
}

void MyGLWidget::resizeGL (int width, int height) {
// Aquest codi és necessari únicament per a MACs amb pantalla retina.
#ifdef __APPLE__
    GLint vp[4];
    glGetIntegerv (GL_VIEWPORT, vp);
    ample = vp[2];
    alt = vp[3];
#else
    ample = width;
    alt = height;
#endif


}


void MyGLWidget::initializeGL ( ){

    DEBUG ("Model carregat");
    // Cal inicialitzar l'ús de les funcions d'OpenGL
    initializeOpenGLFunctions();
    carregaShaders();
    creaBuffersMorty();
    creaBuffersFantasma();
    creaBuffersMoneda();
    creaBuffersTorre();
    creaBuffersTorreText();
    creaBuffersCub();
    creaBuffersCubText();

    //calcula posicions Models
    iniPosTorre();
    iniPosMorty();
    iniPosFantasma();
    iniPosMoneda();
    iniPosParet();

    DEBUG("InitializeGL");

    glEnable(GL_DEPTH_TEST); //Important per pintar
    definicioMatriuLaberint();
    srand(time(NULL)); //para generar numeros aleatorios diferentes
    generaMonedas(10);
    //datos para ubicar la camara en primera persona
    inicializaPosMorty();
    inicializaPosFantasma();

    emit hideWinMessage();
    emit hideOverMessage();
    emit adverticeStartMessage();
    //para que slider aparezcan en la mitad (mitad de min y max)
    emit zoomChanged(45);
    emit thetaChanged (0);
    emit angleChanged (90);
    emit rotateCoins(true); //para que el check este visto desde el inicio

    vista = "General";
    angleCoins = 0.0f;
    angleFocus = 90.0f; //inicio luz(sol) arriba escena
    rotacionMonedas = true;


    //Angles de Euler i moviment càmara
    psi = glm::radians(0.0f);
    theta = glm::radians(45.0f);
    setMouseTracking(true);
    CurrentAction = NONE;
    xClick = 0;
    yClick = 0;
    //Inicialitzem càlcul del angulo del FOV aqui, ja que pel zoom pot ser modificat
    //1. Càlcul Pmin i Pmax i Centre capsa contenidora
    glm::vec3 Pmin, Pmax;
    Pmin = glm::vec3 (0.0,0.0,0.0);
    escala = 6.0f/172.0f;
    float diametreTorre = (xmaxTorre-xminTorre)*escala;
    //Tenir en compte les torres!
    Pmax = glm::vec3 (float(N)+2.0f*diametreTorre,6.0f,float(M)+2.0f*diametreTorre);
    //centre respecte només el laberint (normalment és pmin+pmax/2)
    Centre = (Pmin+glm::vec3 (float(N),6.0f,float(M)))/2.0f; //VPR = Centre !

    //2. Radi esfera
    float radi;
    radi = glm::distance (Pmin,Pmax)/2.0f;
    //pos Luz
    radiusFocus = radi;
    d = 2*radi;

    //3.Càlcul FOV
    angulo = glm::asin (radi/d);

}




void MyGLWidget::paintGL ( ){

    glUseProgram(program->programId()); //para cambio uniform
    glUniform3f(colorFocusLoc, lightColor.redF(),lightColor.greenF(), lightColor.blueF());

    // --- SOLUCIÓN: Asegurar que el Scissor Test esté desactivado para el escenario principal ---
    glDisable(GL_SCISSOR_TEST);

    glUniform1i(modeNitLoc, modeNit);

    glClearColor(0.5, 0.7, 1.0, 1.0); // defineix color de fons (azul)
    // Esborrem el frame-buffer (important) sempre
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //VIEWPORT PRINCIPAL
    glViewport(0,0,ample,alt);

    //Inicializamos camara segun en el caso que nos encontremos
    if (vista == "General") {
        ProjectTransformVistaGeneral();
        ViewTransformVistaGeneral();
    }
    else {
        ProjectTransformVistaPersona();
        ViewTransformVistaPersona();
    }

    //calculamos la luz con la VM correcta de la vista principal
    glm::vec3 posFocusWorld;
    posFocusWorld.x = Centre.x + radiusFocus * cos(glm::radians(angleFocus));
    posFocusWorld.y = Centre.y + radiusFocus * sin(glm::radians(angleFocus));
    posFocusWorld.z = 0.0f;
    glm::vec4 posFocusSCO = VM * glm::vec4(posFocusWorld, 1.0f);

    glUseProgram(program->programId());
    glUniform3f(posFocusLoc, posFocusSCO.x, posFocusSCO.y, posFocusSCO.z);

    // CÀLCUL I ENVIAMENT LUCES PARA MODE NIT ---

        // Morty: Llanterna a l'alçada del personatge (ex: y = 0.5f)
        glm::vec4 posMortyWorld = glm::vec4(posxMorty, 0.5f, poszMorty, 1.0f);
        glm::vec3 posMortySCO = glm::vec3(VM * posMortyWorld);
        glUniform3fv(posLanternaLoc, 1, &posMortySCO[0]);

        // Fantasma: Focus a sobre d'ell (ex: alçada y = 1.0f per sobre de la seva base)
        glm::vec4 posFantasmaWorld = glm::vec4(posxFantasma, 1.0f, poszFantasma, 1.0f);
        glm::vec3 posFantasmaSCO = glm::vec3(VM * posFantasmaWorld);
        glUniform3fv(posFantasmaLoc, 1, &posFantasmaSCO[0]);

        // LUCES DE MONEDAS
        std::vector<glm::vec3> posMonedesSCO;
        std::vector<glm::vec3> dirMonedesSCO;

        // Recorre las posiciones del laberinto para encontrar monedas activas (Ajusta a tus variables)
        for (int f = 0; f < N; ++f) {
            for (int c = 0; c < M; ++c) {
                if (laberint[f][c] == 5) { // Tu condición: 5 significa que hay moneda
                    glm::mat4 TG_moneda = calcularTGMoneda(f, c, false); // false = Con rotación

                    // Posición en SCO (Origen transformado)
                    posMonedesSCO.push_back(glm::vec3(VM * TG_moneda * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));

                    // Dirección en SCO (Normal original de la moneda orientada en Z positivo)
                    glm::mat3 NM_moneda = glm::inverse(glm::transpose(glm::mat3(VM * TG_moneda)));
                    dirMonedesSCO.push_back(glm::normalize(NM_moneda * glm::vec3(0.0f, 0.0f, 1.0f)));
                }
            }
        }

        // Envío de uniforms al shader para el Viewport Principal
        int numMonedes = posMonedesSCO.size();
        glUniform1i(quantitatMonedesLoc, numMonedes);
        glUniform3f(colorMonedaLlumLoc, 1.0f, 0.85f, 0.2f);
        if (numMonedes > 0) {
            glUniform3fv(posMonedesSCOLoc, numMonedes, &posMonedesSCO[0][0]);
            glUniform3fv(dirMonedesSCOLoc, numMonedes, &dirMonedesSCO[0][0]);
        }


    //para saber que transformación usar en moneda
    pintarMonedaMiniMapa = false;
    //para saber si pintar Morty o no, se pinta siempre en el miniMapa y en el "general" solo en camara en tercera persona
    miniMapa = false;

    // PINTAR ESCENARIO PRINCIPAL
    pintarSegonsLaberint();


    //VIEWPORT MINIMAPA
    //Pintar mapa miniatura
    int wMini = ample / 4;
    int hMini = alt / 4;

    glViewport(ample - wMini - 10, 10, wMini, hMini);

    // limpiar SOLO minimapa utilizando Scissor
    glEnable(GL_SCISSOR_TEST);
    glScissor(ample - wMini - 10, 10, wMini, hMini);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); //color blanco de fondo para el minimapa
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Desactivamos el scissor inmediatamente después de limpiar el área del minimapa
    glDisable(GL_SCISSOR_TEST);

    ProjectTransformMiniMapa();
    ViewTransformMiniMapa();


    glm::vec4 posFocusSCO_Mini = VM * glm::vec4(posFocusWorld, 1.0f);

    glUseProgram(program->programId());
    glUniform3f(posFocusLoc, posFocusSCO_Mini.x, posFocusSCO_Mini.y, posFocusSCO_Mini.z);

    //luces minimapa

        glm::vec3 posMortySCO_MM = glm::vec3(VM * glm::vec4(posxMorty, 0.5f, poszMorty, 1.0f));
        glUniform3fv(posLanternaLoc, 1, &posMortySCO_MM[0]);

        glm::vec3 posFantasmaSCO_MM = glm::vec3(VM  * glm::vec4(posxFantasma, 1.0f, poszFantasma, 1.0f));
        glUniform3fv(posFantasmaLoc, 1, &posFantasmaSCO_MM[0]);

        //LUCES DE MONEDAS: VIEWPORT MINIMAPA
            std::vector<glm::vec3> posMonedesSCO_MM;
        std::vector<glm::vec3> dirMonedesSCO_MM;

        for (int f = 0; f < N; ++f) {
            for (int c = 0; c < M; ++c) {
                if (laberint[f][c] == 5) {
                    glm::mat4 TG_moneda_MM = calcularTGMoneda(f, c, true); // true = Estática/Minimapa

                    posMonedesSCO_MM.push_back(glm::vec3(VM * TG_moneda_MM * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)));

                    glm::mat3 NM_moneda_MM = glm::inverse(glm::transpose(glm::mat3(VM * TG_moneda_MM)));
                    dirMonedesSCO_MM.push_back(glm::normalize(NM_moneda_MM * glm::vec3(0.0f, 0.0f, 1.0f)));
                }
            }
        }

        // Actualizamos los uniforms con las coordenadas relativas a la cámara del Minimapa
        glUniform1i(quantitatMonedesLoc, numMonedes);
        if (numMonedes > 0) {
            glUniform3fv(posMonedesSCOLoc, numMonedes, &posMonedesSCO_MM[0][0]);
            glUniform3fv(dirMonedesSCOLoc, numMonedes, &dirMonedesSCO_MM[0][0]);
        }

    //para saber que transformación usar
    pintarMonedaMiniMapa = true;
    miniMapa = true;

    // PINTAR MINIMAPA
    pintarSegonsLaberint();

    //Restaurem viewport original
    glViewport(0,0,ample,alt);
}
void MyGLWidget::pintarSegonsLaberint() {

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            //pintar terra
              glUniform1i(texActiveLoc, 0); //sin textura
            modelTransformTerra(i,j);
            glBindVertexArray(VAO_Cub);
            glDrawArrays(GL_TRIANGLES,0,36);
            glBindVertexArray(0);

            if (laberint[i][j] == 1) { //pintar cubo (pared)
                //texturas
                modelTransformCub(i, j); // Tu transformación habitual

                glUniform1i(texLoc, 0);      // Usar Unidad 0 (Difusa)
                glUniform1i(texActiveLoc, 1); // Activar texturas en el Fragment Shader

                blockParet->Render();
                /*
                modelTransformCub(i,j);
                glBindVertexArray(VAO_Cub);
                glDrawArrays(GL_TRIANGLES,0,36);
                glBindVertexArray(0);
                */
            }
            else if (laberint[i][j] == 4) { //pintar torre
                //texturas
                modelTransformTorre(i, j); // Tu transformación habitual

                glUniform1i(texLoc, 0);      // Usar Unidad 0 (Difusa)
                glUniform1i(texActiveLoc, 1); // Activar texturas en el Fragment Shader

                blockTorre->Render();        // Renderiza el objeto de Assimp y enlaza su textura

                /*//Transformacio model Torre
                modelTransformTorre(i,j);
                // Activem el VAO Torre per pintar
                glBindVertexArray (VAO_Torre);
                // pintem
                glDrawArrays(GL_TRIANGLES, 0, TorreModel.faces().size()*3);
                glBindVertexArray (0);
                */
            }
            if (i == int(poszMorty-0.5) && j == int(posxMorty - 0.5f)) { //pintar morty
                //solo dibujo a Morty si estamos en vista general(no camara en primera persona), pero en el minimapa siempre lo dibujo
                if (vista == "General" or miniMapa == true) {
                    //Transformacio model Morty
                      glUniform1i(texActiveLoc, 0);
                    modelTransformMorty();
                    // Activem el VAO Morty per pintar
                    glBindVertexArray (VAO_Morty);
                    // pintem
                    glDrawArrays(GL_TRIANGLES, 0, MortyModel.faces().size()*3);
                    glBindVertexArray (0);
                }
            }
            else if (laberint[i][j] == 5){ //pintar monedas
                  glUniform1i(texActiveLoc, 0);
                if (pintarMonedaMiniMapa == false) modelTransformMoneda(i,j);
                else modelTransformMonedaMiniMapa(i,j);
                // Activem el VAO Moneda per pintar
                glBindVertexArray (VAO_Moneda);
                // pintem
                glDrawArrays(GL_TRIANGLES, 0, MonedaModel.faces().size()*3);
                glBindVertexArray (0);
            }
            if (i == int(poszFantasma-0.5) && j == int(posxFantasma - 0.5f)) { //pintar fantasma
                  glUniform1i(texActiveLoc, 0);
                //Transformacio model Fantasma
                modelTransformFantasma();
                // Activem el VAO Fantasma per pintar
                glBindVertexArray (VAO_Fantasma);
                // pintem
                glDrawArrays(GL_TRIANGLES, 0, FantasmaModel.faces().size()*3);
                glBindVertexArray (0);
            }
        }
    }
    glUniform1i(texActiveLoc, 0);
}

void MyGLWidget::inicializaPosMorty() {

    angleMovMorty = 0.0f;
    dirMorty = glm::vec3(0.0f,0.0f,1.0f);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {

            if (laberint[i][j] == 2) {

                posxMorty = j + 0.5f;
                poszMorty = i + 0.5f;

                return;
            }
        }
    }

}



//TRANSFORMACIONS
void MyGLWidget::modelTransformMorty() {
    // Matriu de transformació de model
    glm::mat4 transform (1.0f);
    //altura inicial
    float altini = ymaxMorty-yminMorty;
    //altura deseada dividio altura inicial
    escala = 1.5f/altini;

    //mover posición laberinto
    transform = glm::translate(transform, glm::vec3(posxMorty,0.1,poszMorty));

    //roto, en caso necesario (movimiento Morty)
    transform = glm::rotate (transform, glm::radians(angleMovMorty),glm::vec3(0.0f, 1.0f, 0.0f));

    //escalar
    transform = glm::scale(transform, glm::vec3(escala));

    // mover centro base al origen

    //Calculae base del modelo
    glm::vec3 center((xminMorty + xmaxMorty) * 0.5f,yminMorty,(zminMorty + zmaxMorty) * 0.5f);

    transform = glm::translate(transform, -center);

    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}

void MyGLWidget::inicializaPosFantasma() {

    //Inicialmente mira hacia  z postiva
    dirFantasma = glm::vec3 (0.0f,0.0f,1.0f);

    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {

            if (laberint[i][j] == 3) {

                posxFantasma = j + 0.5f;
                poszFantasma = i + 0.5f;

                return;
            }
        }
    }

}

void MyGLWidget::moveFantasma () {
    //revisar si puede moverse hacia adelanta
    /*1.Buscar posición modelo en el laberinto
            !!posxFantasma = j+0.5
            !!poszFantasma = i+0.5
            */
    // celda actual de Morty
    int fila = int(poszFantasma - 0.5f);
    int col  = int(posxFantasma - 0.5f);

    // celda hacia delante
    int nuevaFila = fila + int(dirFantasma.z);
    int nuevaCol  = col  + int(dirFantasma.x);

    // comprobar límites
    if (nuevaFila >= 0 && nuevaFila < N && nuevaCol >= 0 && nuevaCol < M) {

        // comprobar si está libre la celda de adelante
        if (laberint[nuevaFila][nuevaCol] != 1) {

            if (laberint[fila][col] == 3) laberint [fila][col] = 0;

            // mover Fantasma
            posxFantasma = nuevaCol + 0.5f;
            poszFantasma = nuevaFila + 0.5f;

            //si fantasma toca a Morty
            if (posxMorty == posxFantasma and poszMorty == poszFantasma) {
                gameOver = true;
                emit gameOverSignal();
            }
            return;
        }

    }
    //si no esta libre, moverlo aleatroriamente en la dirección que pueda
    std::vector<glm::vec3> dirs;

    glm::vec3 opciones[4] = {
        glm::vec3(1,0,0),
        glm::vec3(-1,0,0),
        glm::vec3(0,0,1),
        glm::vec3(0,0,-1)
    };

    for (auto &d : opciones) {
        int nf = fila + int(d.z);
        int nc = col  + int(d.x);

        if (nf >= 0 && nf < N && nc >= 0 && nc < M && laberint[nf][nc]!= 1) {
            dirs.push_back(d);
        }
    }

    // si no hay salida → no se mueve (siempre habra salida)
    if (dirs.empty()) return;

    // 3. elegir dirección aleatoria
    int r = rand() % dirs.size();
    dirFantasma = dirs[r];

    //para rotación
    if (dirFantasma == glm::vec3(1,0,0))
        angleMovFantasma = 90.0f;

    else if (dirFantasma == glm::vec3(-1,0,0))
        angleMovFantasma = 270.0f;

    else if (dirFantasma == glm::vec3(0,0,1))
        angleMovFantasma = 0.0f;

    else if (dirFantasma == glm::vec3(0,0,-1))
        angleMovFantasma = 180.0f;

    //modificamos posición
    if (laberint[fila][col] == 3) laberint [fila][col] = 0;

    int nf = fila + int(dirFantasma.z);
    int nc = col  + int(dirFantasma.x);

    // 4. mover fantasma
    posxFantasma = nc + 0.5f;
    poszFantasma = nf + 0.5f;

    //si fantasma toca a Morty
    if (posxMorty == posxFantasma and poszMorty == poszFantasma) {
        gameOver = true;
        emit gameOverSignal();
    }

}

void MyGLWidget::modelTransformFantasma() {
    // Matriu de transformació de model
    glm::mat4 transform (1.0f);
    //altura inicial
    float altini = ymaxFantasma-yminFantasma;
    escala = 0.65f/altini;

    // mover a posición laberinto
    transform = glm::translate(transform, glm::vec3(posxFantasma,0.1,poszFantasma));

    //roto, en caso necesario (movimiento Fantasma)
    transform = glm::rotate (transform, glm::radians(angleMovFantasma),glm::vec3(0.0f, 1.0f, 0.0f));

    //escalar (centro base modelo inicialmente en (0,0,0))
    transform = glm::scale(transform, glm::vec3(escala));

    //Calculae base del modelo
    glm::vec3 center((xminFantasma + xmaxFantasma) * 0.5f,yminFantasma,(zminFantasma + zmaxFantasma) * 0.5f);

    transform = glm::translate(transform, -center);

    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}

void MyGLWidget::modelTransformMoneda(int fila, int colum) {
    // Matriu de transformació de model
    glm::mat4 transform (1.0f);
    //altura inicial
    float altini = ymaxMoneda-yminMoneda;
    //altura deseada dividio altura inicial
    escala = 0.5f/altini;

    // mover a posición laberinto
    transform = glm::translate(transform, glm::vec3(colum+0.5,0.1,fila+0.5));

    //rotación
    transform = glm::rotate(transform, glm::radians(angleCoins), glm::vec3(0,1,0));

    //escalar
    transform = glm::scale(transform, glm::vec3(escala));

    // mover centro base al origen
    glm::vec3 center((xminMoneda + xmaxMoneda) * 0.5f,yminMoneda,(zminMoneda + zmaxMoneda) * 0.5f);

    transform = glm::translate(transform, -center);

    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}
//sin rotación y mirando hacia arriba
void MyGLWidget::modelTransformMonedaMiniMapa (int fila, int colum) {
    // Matriu de transformació de model
    glm::mat4 transform (1.0f);
    //altura inicial
    float altini = ymaxMoneda-yminMoneda;
    //altura deseada dividio altura inicial
    escala = 0.5f/altini;

    // mover a posición laberinto
    transform = glm::translate(transform, glm::vec3(colum+0.5,0.1,fila+0.5));

    //rotación
    transform = glm::rotate(transform, glm::radians(-90.0f), glm::vec3(1,0,0));

    //escalar
    transform = glm::scale(transform, glm::vec3(escala));

    // mover centro base al origen
    glm::vec3 center((xminMoneda + xmaxMoneda) * 0.5f,yminMoneda,(zminMoneda + zmaxMoneda) * 0.5f);

    transform = glm::translate(transform, -center);

    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}

void MyGLWidget::modelTransformTorre(int fila, int colum) {

    // Matriu de transformació de model
    glm::mat4 transform (1.0f);
    escala = 6.0f/172.0f;

    float profundidad = (zmaxTorre-zminTorre)*escala;
        // mover a posición laberinto
    if (fila == 0)  {
        //en fila, restamos mitad de profundidad de la torre
        transform = glm::translate(transform, glm::vec3(colum+0.5,0,fila-profundidad/2.0f));

    }
    else if (fila == N-1){
        transform = glm::translate(transform, glm::vec3(colum+0.5,0,fila+profundidad-1.3));
        //roto
        transform = glm::rotate (transform, glm::radians(-180.0f),glm::vec3(0,1,0));
    }
    else if (colum == 0){
        transform = glm::translate(transform, glm::vec3(colum-profundidad/2.0f,0,fila+0.5));
        //roto
        transform = glm::rotate (transform, glm::radians(90.0f),glm::vec3(0,1,0));
    }
    else if (colum == M-1) {
        transform = glm::translate(transform, glm::vec3(colum+profundidad-1.3,0,fila+0.5));
        //roto
        transform = glm::rotate (transform, glm::radians(-90.0f),glm::vec3(0,1,0));
    }

    //escalar
    /*antes de escalar! sabemos que el ancho(X) y la profundidad(z) son:
    ANCHO = Xmax-Xmin = 134.913
    PROFUNDIDAD = Zmax-Zmin = 125.173

    DESPUYES DE ESCALAR
    ANCHO = 134.913* escala(un valor) = 4.7
    PROFUNDIDAD = 125.173 * escala = 4.36

    Ocupa 4.7 * 4.3 metros en el laberinto, es decir aprox 5*5 celdas (al trasladar no le sumo medio metro (media celda)porque ocupa mas de una celda!)
    */
    transform = glm::scale(transform, glm::vec3(escala));

    // mover centro base al origen
    transform = glm::translate(transform, glm::vec3(2.0f, 0.0f, 2.0f));

    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}

void MyGLWidget::modelTransformCub(int i, int j) {
    // 1. Inicializar la matriz de transformación de modelo
    glm::mat4 transform(1.0f);

    // 2. Calcular la posición en el mundo para la celda [i][j]
    // Sumamos 0.5f a 'j' (X) y a 'i' (Z) para que el cubo quede centrado dentro de su casilla.
    // La altura 'y' es 0.0f para que la base descanse sobre el suelo.
    float posX = float(j) + 0.5f;
    float posZ = float(i) + 0.5f;
    float posY = 0.0f;

    transform = glm::translate(transform, glm::vec3(posX, posY, posZ));

    // 3. Calcular el escalado
    // Queremos que ocupe exactamente 1 unidad en el eje X y 1 unidad en el eje Z.
    // Para el eje Y (altura), le daremos el valor que desees (por ejemplo, 3.0f o 2.0f).
    float ampladaDesitjadaX = 1.0f;
    float ampladaDesitjadaZ = 1.0f;
    float alcadaDesitjadaY  = 1.0f; // Cambia este valor si quieres paredes más altas o bajas

    float escalaX = ampladaDesitjadaX / (xmaxParet - xminParet);
    float escalaY = alcadaDesitjadaY  / (ymaxParet - yminParet);
    float escalaZ = ampladaDesitjadaZ / (zmaxParet - zminParet);

    transform = glm::scale(transform, glm::vec3(escalaX, escalaY, escalaZ));

    // 4. Mover el centro de la base del modelo al origen (0,0,0)
    // El centro en X y Z es el punto medio. En Y usamos 'yminParet' para situar el pivote abajo.
    glm::vec3 center(
        (xminParet + xmaxParet) * 0.5f,
        yminParet,
        (zminParet + zmaxParet) * 0.5f
        );

    // Trasladamos en negativo hacia el origen antes de que actúen las escalas y traslaciones
    transform = glm::translate(transform, -center);

    // 5. Enviar la matriz de transformación resultante al Vertex Shader
    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}

/*
void MyGLWidget::modelTransformCub (int fila, int colum) {
    glm::mat4 transform (1.0f);

    // mover a posición laberinto (inicialmente a (0,0,0)
    transform = glm::translate(transform, glm::vec3(colum,0.1,fila));

    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}
*/

void MyGLWidget::modelTransformTerra (int fila, int colum) {
    glm::mat4 transform (1.0f);

    // mover a posición laberinto (inicialmente a (0,0,0)
    transform = glm::translate(transform, glm::vec3(colum,0,fila));

    //escalar
    transform = glm::scale(transform, glm::vec3(1,0.1,1));

    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);

}



void MyGLWidget::definicioMatriuLaberint() {
    int aux[N][M] = {

        {1,4,1,1,1,1,1,1,1,1},
        {1,0,0,0,1,1,1,0,0,1},
        {1,1,1,0,1,0,1,0,0,1},
        {1,1,1,0,0,2,0,0,0,4},
        {1,1,1,0,0,0,0,0,0,1},
        {4,0,0,0,1,0,0,1,1,1},
        {1,0,1,1,1,0,0,1,1,1},
        {1,3,0,0,0,0,0,0,1,1},
        {1,1,1,1,1,0,1,1,1,1},
        {1,1,1,1,1,4,1,1,1,1}

    };

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {

            laberint[i][j] = aux[i][j];
        }
    }
}

glm::mat4 MyGLWidget::calcularTGMoneda(int fila, int colum, bool esMinimapa) {
    glm::mat4 transform(1.0f);
    float altini = ymaxMoneda - yminMoneda;
    float escalaLocal = 0.5f / altini;

    transform = glm::translate(transform, glm::vec3(colum + 0.5f, 0.1f, fila + 0.5f));

    if (esMinimapa) {
        transform = glm::rotate(transform, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    } else {
        transform = glm::rotate(transform, glm::radians(angleCoins), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    transform = glm::scale(transform, glm::vec3(escalaLocal));
    glm::vec3 center((xminMoneda + xmaxMoneda) * 0.5f, yminMoneda, (zminMoneda + zmaxMoneda) * 0.5f);
    transform = glm::translate(transform, -center);

    return transform;
}


void MyGLWidget::generaMonedas(int numMonedas) {

    int monedasGeneradas = 0;

    while (monedasGeneradas < numMonedas) {

        int fila = rand() % N;
        int col  = rand() % M;

        // SOLO si está vacío
        if (laberint[fila][col] == 0) {

            laberint[fila][col] = 5;

            monedasGeneradas++;
        }
    }
}

void MyGLWidget::iniPosMorty() {
    float* vertices = MortyModel.VBO_vertices();

    xminMorty  = yminMorty  = zminMorty  = 1e9;
    xmaxMorty  = ymaxMorty  = zmaxMorty  = -1e9;

    int n = MortyModel.faces().size()*3;

    for (int i = 0; i < n; i++) {

        float x = vertices[3*i];
        float y = vertices[3*i + 1];
        float z = vertices[3*i + 2];

        xminMorty = std::min(xminMorty, x);
        xmaxMorty  = std::max(xmaxMorty, x);

        yminMorty  = std::min(yminMorty, y);
        ymaxMorty  = std::max(ymaxMorty, y);

        zminMorty  = std::min(zminMorty, z);
        zmaxMorty  = std::max(zmaxMorty, z);
    }

    std::cout << "XminMorty: " << xminMorty<< "xmaxMorty: " << xmaxMorty << "fi " << std::endl;
    std::cout << "yminMorty: " << yminMorty<< "ymaxMorty: " << ymaxMorty << "fi " << std::endl;
    std::cout << "zminMorty: " << zminMorty<< "zmaxMorty: " << zmaxMorty << "fi " << std::endl;
}


void MyGLWidget::creaBuffersMorty () {

    //Carregar Model Morty
    MortyModel.load ("model3D/Morty.obj");
    // Creació VAO_Morty
    glGenVertexArrays(1, &VAO_Morty);
    glBindVertexArray(VAO_Morty); //ACtivació VAO

    GLuint VBO[6];
    glGenBuffers(6, VBO);

    //activació VBO vertexs
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    int sizeOfBuffer = sizeof(GLfloat)* MortyModel.faces().size()*3*3;
    glBufferData(GL_ARRAY_BUFFER, sizeOfBuffer, MortyModel.VBO_vertices(), GL_STATIC_DRAW); //enviar vertexs

    // Activem l'atribut vertexLoc
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertexLoc);

    /*Activació VBO colors
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeOfBuffer, MortyModel.VBO_matdiff(), GL_STATIC_DRAW);

    // Activem l'atribut colorLoc
    glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(colorLoc);

    glBindVertexArray (0);
    */
    //Ara tenim iluminació (it 5)

    // Buffer de normals
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*MortyModel.faces().size()*3*3, MortyModel.VBO_normals(), GL_STATIC_DRAW);

    glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normalLoc);

    // En lloc del color, ara passem tots els paràmetres dels materials
    // Buffer de component ambient
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*MortyModel.faces().size()*3*3, MortyModel.VBO_matamb(), GL_STATIC_DRAW);

    glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matambLoc);

    // Buffer de component difusa
    glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*MortyModel.faces().size()*3*3, MortyModel.VBO_matdiff(), GL_STATIC_DRAW);

    glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matdiffLoc);

    // Buffer de component especular
    glBindBuffer(GL_ARRAY_BUFFER, VBO[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*MortyModel.faces().size()*3*3, MortyModel.VBO_matspec(), GL_STATIC_DRAW);

    glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matspecLoc);

    // Buffer de component shininness
    glBindBuffer(GL_ARRAY_BUFFER, VBO[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*MortyModel.faces().size()*3,MortyModel.VBO_matshin(), GL_STATIC_DRAW);

    glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matshinLoc);

    glBindVertexArray(0);



}



void MyGLWidget::iniPosFantasma() {
    float* vertices = FantasmaModel.VBO_vertices();

    xminFantasma = yminFantasma  = zminFantasma = 1e9;
    xmaxFantasma  = ymaxFantasma  = zmaxFantasma  = -1e9;

    int n = FantasmaModel.faces().size()*3;

    for (int i = 0; i < n; i++) {

        float x = vertices[3*i];
        float y = vertices[3*i + 1];
        float z = vertices[3*i + 2];

        xminFantasma = std::min(xminFantasma, x);
        xmaxFantasma  = std::max(xmaxFantasma, x);

        yminFantasma  = std::min(yminFantasma, y);
        ymaxFantasma = std::max(ymaxFantasma, y);

        zminFantasma = std::min(zminFantasma, z);
        zmaxFantasma  = std::max(zmaxFantasma, z);
    }

    std::cout << "XminFantasma: " << xminFantasma<< "xmaxFantasma: " << xmaxFantasma << "fi " << std::endl;
    std::cout << "yminFantasma: " << yminFantasma<< "ymaxFantasma: " << ymaxFantasma << "fi " << std::endl;
    std::cout << "zminFantasma: " << zminFantasma<< "zmaxFantasma: " << zmaxFantasma << "fi " << std::endl;
}

void MyGLWidget::creaBuffersFantasma() {
    //Carregar Model Fantasma
    FantasmaModel.load ("model3D/Fantasma.obj");
    // Creació VAO_Morty
    glGenVertexArrays(1, &VAO_Fantasma);
    glBindVertexArray(VAO_Fantasma); //ACtivació VAO

    GLuint VBO[6];
    glGenBuffers(6, VBO);

    //activació VBO vertexs
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    int sizeOfBuffer = sizeof(GLfloat)* FantasmaModel.faces().size()*3*3;
    glBufferData(GL_ARRAY_BUFFER, sizeOfBuffer, FantasmaModel.VBO_vertices(), GL_STATIC_DRAW); //enviar vertexs

    // Activem l'atribut vertexLoc
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertexLoc);

    /*Activació VBO colors
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeOfBuffer, FantasmaModel.VBO_matdiff(), GL_STATIC_DRAW);

    // Activem l'atribut colorLoc
    glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(colorLoc);

    */
    //Ara tenim iluminació (it 5)

    // Buffer de normals
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*FantasmaModel.faces().size()*3*3, FantasmaModel.VBO_normals(), GL_STATIC_DRAW);

    glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normalLoc);

    // En lloc del color, ara passem tots els paràmetres dels materials
    // Buffer de component ambient
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*FantasmaModel.faces().size()*3*3, FantasmaModel.VBO_matamb(), GL_STATIC_DRAW);

    glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matambLoc);

    // Buffer de component difusa
    glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*FantasmaModel.faces().size()*3*3, FantasmaModel.VBO_matdiff(), GL_STATIC_DRAW);

    glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matdiffLoc);

    // Buffer de component especular
    glBindBuffer(GL_ARRAY_BUFFER, VBO[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*FantasmaModel.faces().size()*3*3, FantasmaModel.VBO_matspec(), GL_STATIC_DRAW);

    glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matspecLoc);

    // Buffer de component shininness
    glBindBuffer(GL_ARRAY_BUFFER, VBO[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*FantasmaModel.faces().size()*3, FantasmaModel.VBO_matshin(), GL_STATIC_DRAW);

    glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matshinLoc);

    glBindVertexArray(0);

}

void MyGLWidget::iniPosMoneda() {
    float* vertices = MonedaModel.VBO_vertices();

    xminMoneda  = yminMoneda  = zminMoneda  = 1e9;
    xmaxMoneda  = ymaxMoneda  = zmaxMoneda = -1e9;

    int n = MonedaModel.faces().size()*3;

    for (int i = 0; i < n; i++) {

        float x = vertices[3*i];
        float y = vertices[3*i + 1];
        float z = vertices[3*i + 2];

        xminMoneda = std::min(xminMoneda, x);
        xmaxMoneda  = std::max(xmaxMoneda, x);

        yminMoneda  = std::min(yminMoneda, y);
        ymaxMoneda  = std::max(ymaxMoneda, y);

        zminMoneda  = std::min(zminMoneda, z);
        zmaxMoneda  = std::max(zmaxMoneda, z);
    }

    std::cout << "XminMoneda: " << xminMoneda<< "xmaxMoneda: " << xmaxMoneda << "fi " << std::endl;
    std::cout << "yminMoneda: " << yminMoneda<< "ymaxMorty: " << ymaxMoneda<< "fi " << std::endl;
    std::cout << "zminMoneda: " << zminMoneda<< "zmaxMorty: " << zmaxMoneda<< "fi " << std::endl;
}

//slot de giro de monedas
void MyGLWidget::rotateCoins() {

    makeCurrent();

    // Si la rotación está desactivada no se hace nada
    if (rotacionMonedas != false ) {

        angleCoins += 2.0f;
        if (angleCoins > 360.0f) angleCoins -= 360.0f;
        update();
    }
}

void MyGLWidget::creaBuffersMoneda() {
    //Carregar Model Moneda
    MonedaModel.load ("model3D/Coin.obj");
    // Creació VAO_Morty
    glGenVertexArrays(1, &VAO_Moneda);
    glBindVertexArray(VAO_Moneda); //ACtivació VAO

    GLuint VBO[6];
    glGenBuffers(6, VBO);

    //activació VBO vertexs
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    int sizeOfBuffer = sizeof(GLfloat)* MonedaModel.faces().size()*3*3;
    glBufferData(GL_ARRAY_BUFFER, sizeOfBuffer, MonedaModel.VBO_vertices(), GL_STATIC_DRAW); //enviar vertexs

    // Activem l'atribut vertexLoc
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertexLoc);

    /*Activació VBO colors
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeOfBuffer, MonedaModel.VBO_matdiff(), GL_STATIC_DRAW);

    // Activem l'atribut colorLoc
    glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(colorLoc);

    */
    //Ara tenim iluminació (it 5)

    // Buffer de normals
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* MonedaModel.faces().size()*3*3,  MonedaModel.VBO_normals(), GL_STATIC_DRAW);

    glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normalLoc);

    // En lloc del color, ara passem tots els paràmetres dels materials
    // Buffer de component ambient
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* MonedaModel.faces().size()*3*3,  MonedaModel.VBO_matamb(), GL_STATIC_DRAW);

    glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matambLoc);

    // Buffer de component difusa
    glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* MonedaModel.faces().size()*3*3,  MonedaModel.VBO_matdiff(), GL_STATIC_DRAW);

    glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matdiffLoc);

    // Buffer de component especular
    glBindBuffer(GL_ARRAY_BUFFER, VBO[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* MonedaModel.faces().size()*3*3,  MonedaModel.VBO_matspec(), GL_STATIC_DRAW);

    glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matspecLoc);

    // Buffer de component shininness
    glBindBuffer(GL_ARRAY_BUFFER, VBO[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)* MonedaModel.faces().size()*3,  MonedaModel.VBO_matshin(), GL_STATIC_DRAW);

    glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matshinLoc);

    glBindVertexArray(0);


}

void MyGLWidget::iniPosTorre() {

    float* vertices = TorreModel.VBO_vertices();

    xminTorre = yminTorre = zminTorre = 1e9;
    xmaxTorre = ymaxTorre = zmaxTorre = -1e9;

    int n = TorreModel.faces().size()*3;

    for (int i = 0; i < n; i++) {

        float x = vertices[3*i];
        float y = vertices[3*i + 1];
        float z = vertices[3*i + 2];

        xminTorre = std::min(xminTorre, x);
        xmaxTorre = std::max(xmaxTorre, x);

        yminTorre = std::min(yminTorre, y);
        ymaxTorre = std::max(ymaxTorre, y);

        zminTorre = std::min(zminTorre, z);
        zmaxTorre = std::max(zmaxTorre, z);
    }

    std::cout << xminTorre<< "xmax: " << xmaxTorre << "fi " << std::endl;
    std::cout << yminTorre<< "ymax: " << ymaxTorre << "fi " << std::endl;
    std::cout << zminTorre<< "zmax: " << zmaxTorre << "fi " << std::endl;
}

void MyGLWidget::iniPosParet() {
    // Comprobamos primero que el modelo esté cargado para evitar que falle
    if (blockParet == nullptr) {
        qDebug() << "Error: blockParet no está inicializado todavía.";
        return;
    }

    // Guardamos los valores mínimos en los ejes X, Y, Z
    xminParet = blockParet->GetBBMin().x;
    yminParet = blockParet->GetBBMin().y;
    zminParet = blockParet->GetBBMin().z;

    // Guardamos los valores máximos en los ejes X, Y, Z
    xmaxParet = blockParet->GetBBMax().x;
    ymaxParet = blockParet->GetBBMax().y;
    zmaxParet = blockParet->GetBBMax().z;

    // (Opcional) Debug para comprobar que los datos se han guardado correctamente
    qDebug() << "---- MEDIDAS PARED ----";
    qDebug() << "X:" << xminParet << "a" << xmaxParet << " (Tam:" << (xmaxParet - xminParet) << ")";
    qDebug() << "Y:" << yminParet << "a" << ymaxParet << " (Tam:" << (ymaxParet - yminParet) << ")";
    qDebug() << "Z:" << zminParet << "a" << zmaxParet << " (Tam:" << (zmaxParet - zminParet) << ")";
}

void MyGLWidget::creaBuffersTorre() {
    //Carregar Model Torre
    TorreModel.load ("model3D/tower.obj");
    // Creació VAO_torre
    glGenVertexArrays(1, &VAO_Torre);
    glBindVertexArray(VAO_Torre); //ACtivació VAO

    GLuint VBO[6];
    glGenBuffers(6, VBO);

    //activació VBO vertexs
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    int sizeOfBuffer = sizeof(GLfloat)* TorreModel.faces().size()*3*3;
    glBufferData(GL_ARRAY_BUFFER, sizeOfBuffer, TorreModel.VBO_vertices(), GL_STATIC_DRAW); //enviar vertexs

    // Activem l'atribut vertexLoc
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertexLoc);

    /*Activació VBO colors
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeOfBuffer, TorreModel.VBO_matdiff(), GL_STATIC_DRAW);

    // Activem l'atribut colorLoc
    glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(colorLoc);
    */
    //Ara tenim iluminació (it 5)

    // Buffer de normals
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*TorreModel.faces().size()*3*3, TorreModel.VBO_normals(), GL_STATIC_DRAW);

    glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normalLoc);

    // En lloc del color, ara passem tots els paràmetres dels materials
    // Buffer de component ambient
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*TorreModel.faces().size()*3*3, TorreModel.VBO_matamb(), GL_STATIC_DRAW);

    glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matambLoc);

    // Buffer de component difusa
    glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*TorreModel.faces().size()*3*3, TorreModel.VBO_matdiff(), GL_STATIC_DRAW);

    glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matdiffLoc);

    // Buffer de component especular
    glBindBuffer(GL_ARRAY_BUFFER, VBO[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*TorreModel.faces().size()*3*3, TorreModel.VBO_matspec(), GL_STATIC_DRAW);

    glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matspecLoc);

    // Buffer de component shininness
    glBindBuffer(GL_ARRAY_BUFFER, VBO[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*TorreModel.faces().size()*3, TorreModel.VBO_matshin(), GL_STATIC_DRAW);

    glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matshinLoc);

    glBindVertexArray(0);

}

void MyGLWidget::creaBuffersTorreText() {
    blockTorre = new Mesh(this, vertexLoc, normalLoc, texUVLoc, matdiffLoc, matspecLoc, matambLoc, matshinLoc);
    blockTorre->LoadMesh("model3D/tower.obj");
}

// Funció d'utilitat per crear un VAO d'un cub amb l'aresta mínima a l'origen
// i d'aresta unitària.
void MyGLWidget::creaBuffersCub()
{
    // Dades del cub
    // Vèrtexs del cub
    glm::vec3 vertexs[8] = {
        /* 0*/ glm::vec3( 0.0, 0.0, 0.0),  /* 1*/ glm::vec3( 1.0, 0.0, 0.0),
        /* 2*/ glm::vec3( 0.0, 1.0, 0.0),  /* 3*/ glm::vec3( 1.0, 1.0, 0.0),
        /* 4*/ glm::vec3( 0.0, 0.0, 1.0),  /* 5*/ glm::vec3( 1.0, 0.0, 1.0),
        /* 6*/ glm::vec3( 0.0, 1.0, 1.0),  /* 7*/ glm::vec3( 1.0, 1.0, 1.0)
    };

    // VBO amb la posició dels vèrtexs
    glm::vec3 poscub[36] = {  // 12 triangles
        vertexs[0], vertexs[2], vertexs[1],
        vertexs[1], vertexs[2], vertexs[3],
        vertexs[5], vertexs[1], vertexs[7],
        vertexs[1], vertexs[3], vertexs[7],
        vertexs[2], vertexs[6], vertexs[3],
        vertexs[3], vertexs[6], vertexs[7],
        vertexs[0], vertexs[4], vertexs[6],
        vertexs[0], vertexs[6], vertexs[2],
        vertexs[0], vertexs[1], vertexs[4],
        vertexs[1], vertexs[5], vertexs[4],
        vertexs[4], vertexs[5], vertexs[6],
        vertexs[5], vertexs[7], vertexs[6]
    };

    // VBO amb la normal de cada vèrtex
    glm::vec3 normals[6] = {
        /* 0*/ glm::vec3( 1.0, 0.0,  0.0),  /* 1*/ glm::vec3( -1.0, 0.0, 0.0),
        /* 2*/ glm::vec3( 0.0, 1.0,  0.0),  /* 3*/ glm::vec3( 0.0, -1.0, 0.0),
        /* 4*/ glm::vec3( 0.0, 0.0,  1.0),  /* 5*/ glm::vec3( 0.0, 0.0, -1.0)
    };
    glm::vec3 normcub[36] = {
        normals[5], normals[5], normals[5],
        normals[5], normals[5], normals[5],
        normals[0], normals[0], normals[0],
        normals[0], normals[0], normals[0],
        normals[2], normals[2], normals[2],
        normals[2], normals[2], normals[2],
        normals[1], normals[1], normals[1],
        normals[1], normals[1], normals[1],
        normals[3], normals[3], normals[3],
        normals[3], normals[3], normals[3],
        normals[4], normals[4], normals[4],
        normals[4], normals[4], normals[4]
    };

    // inicialitzem el material del cub
    glm::vec3 amb, diff, spec;
    float shin;
    amb = glm::vec3(0.1,0.0,0.0);
    diff = glm::vec3(0.6,0.5,0.5);
    spec = glm::vec3(0.6,0.6,0.6);
    shin = 100;

    // Fem que aquest material afecti a tots els vèrtexs per igual
    glm::vec3 matambcub[36] = {
        amb, amb, amb, amb, amb, amb,
        amb, amb, amb, amb, amb, amb,
        amb, amb, amb, amb, amb, amb,
        amb, amb, amb, amb, amb, amb,
        amb, amb, amb, amb, amb, amb,
        amb, amb, amb, amb, amb, amb
    };
    float a=0.5;
    glm::vec3 diff1 = glm::vec3(0.6,0.0,0.0);
    glm::vec3 matdiffcub[36] = {
        diff, diff, diff1, diff, diff, diff,
        diff, diff, diff1, diff, diff, diff,
        diff*a, diff*a, diff1*a, diff*a, diff*a, diff1*a,
        diff, diff, diff1, diff, diff, diff,
        diff, diff, diff1, diff, diff, diff,
        diff, diff, diff1, diff, diff, diff
    };
    glm::vec3 matspeccub[36] = {
        spec, spec, spec, spec, spec, spec,
        spec, spec, spec, spec, spec, spec,
        spec, spec, spec, spec, spec, spec,
        spec, spec, spec, spec, spec, spec,
        spec, spec, spec, spec, spec, spec,
        spec, spec, spec, spec, spec, spec
    };
    float matshincub[36] = {
        shin, shin, shin, shin, shin, shin,
        shin, shin, shin, shin, shin, shin,
        shin, shin, shin, shin, shin, shin,
        shin, shin, shin, shin, shin, shin,
        shin, shin, shin, shin, shin, shin,
        shin, shin, shin, shin, shin, shin
    };

    // Creació del Vertex Array Object del cub
    glGenVertexArrays(1, &VAO_Cub);
    glBindVertexArray(VAO_Cub);

    GLuint VBO_Cub[6];
    glGenBuffers(6, VBO_Cub);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(poscub), poscub, GL_STATIC_DRAW);

    // Activem l'atribut vertexLoc
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertexLoc);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(normcub), normcub, GL_STATIC_DRAW);

    // Activem l'atribut normalLoc
    glVertexAttribPointer(normalLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(normalLoc);

    // En lloc del color, ara passem tots els paràmetres dels materials
    // Buffer de component ambient
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(matambcub), matambcub, GL_STATIC_DRAW);

    glVertexAttribPointer(matambLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matambLoc);

    // Buffer de component difusa
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(matdiffcub), matdiffcub, GL_STATIC_DRAW);

    glVertexAttribPointer(matdiffLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matdiffLoc);

    // Buffer de component especular
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(matspeccub), matspeccub, GL_STATIC_DRAW);

    glVertexAttribPointer(matspecLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matspecLoc);

    // Buffer de component shininness
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[5]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(matshincub), matshincub, GL_STATIC_DRAW);

    glVertexAttribPointer(matshinLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(matshinLoc);

    glBindVertexArray(0);

    /* Color
    glm::vec3 diff = glm::vec3 (0.6,0.5,0.5);
    glm::vec3 diff1 = glm::vec3 (0.6,0.0,0.0);
    float a = 0.5;

    glm::vec3 colorcub[36] = {
        diff, diff, diff1,diff,diff,diff,diff,diff, diff1,diff,diff,diff,
        diff*a,diff*a,diff1*a,diff*a,diff*a,diff1*a,
        diff,diff,diff1,diff,diff,diff,diff,diff,diff1,diff,diff,diff,
        diff,diff,diff1,diff,diff,diff
    };
    */


    /*
    //BUFFERS COLORS
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Cub[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colorcub), colorcub,   GL_STATIC_DRAW);

    glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(colorLoc);
   */
}

void MyGLWidget::creaBuffersCubText() {

    blockParet = new Mesh(this, vertexLoc, normalLoc, texUVLoc, matdiffLoc, matspecLoc, matambLoc, matshinLoc);
    blockParet->LoadMesh("model3D/block.obj");
}


void MyGLWidget::carregaShaders () {
    BL2GLWidget::carregaShaders();

    PMLoc = glGetUniformLocation(program->programId(), "PM");
    VMLoc = glGetUniformLocation(program->programId(), "VM");

    // Obtenim identificador per a l'atribut “vertex” del vertex shader
    vertexLoc = glGetAttribLocation (program->programId(), "vertex");
    // Obtenim identificador per a l'atribut “normal” del vertex shader
    normalLoc = glGetAttribLocation (program->programId(), "normal");
    // Obtenim identificador per a l'atribut “matamb” del vertex shader
    matambLoc = glGetAttribLocation (program->programId(), "matamb");
    // Obtenim identificador per a l'atribut “matdiff” del vertex shader
    matdiffLoc = glGetAttribLocation (program->programId(), "matdiff");
    // Obtenim identificador per a l'atribut “matspec” del vertex shader
    matspecLoc = glGetAttribLocation (program->programId(), "matspec");
    // Obtenim identificador per a l'atribut “matshin” del vertex shader
    matshinLoc = glGetAttribLocation (program->programId(), "matshin");

    colorFocusLoc = glGetUniformLocation(program->programId(), "colorFocus");
    posFocusLoc = glGetUniformLocation(program->programId(), "posFocus");

    //noche
    modeNitLoc = glGetUniformLocation(program->programId(), "modeNit");
    posLanternaLoc = glGetUniformLocation(program->programId(), "posLanternaSCO");
    posFantasmaLoc = glGetUniformLocation(program->programId(), "posFantasmaSCO");

    //luz monedas
    quantitatMonedesLoc = glGetUniformLocation(program->programId(), "quantitatMonedes");
    posMonedesSCOLoc = glGetUniformLocation(program->programId(), "posMonedesSCO");
    dirMonedesSCOLoc = glGetUniformLocation(program->programId(), "dirMonedesSCO");
    colorMonedaLlumLoc = glGetUniformLocation(program->programId(), "colorMonedaLlum");

    //texturas
    texLoc = glGetUniformLocation(program->programId(), "text");
    texActiveLoc = glGetUniformLocation(program->programId(), "textActive");
    texUVLoc = glGetAttribLocation(program->programId(), "UV");

    DEBUG ("CarregaShaders");
}


// keyPressEvent - Es cridat quan es prem una tecla
void MyGLWidget::keyPressEvent (QKeyEvent *event) {
    makeCurrent();

    //ZOOM SIEMPRE PERMITIDO
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Minus || event->key() == Qt::Key_C || event->key() == Qt::Key_O || event->key() == Qt::Key_P || event->key() == Qt::Key_N )
    {
        if (event->key() == Qt::Key_Plus) {
            angulo -= glm::radians(5.0f);
            //L'angle de obertura ha de ser major a 0º!
            if (angulo <= 0.01f)
                angulo = 0.01f;
            emit zoomChanged(int(glm::degrees(angulo)));
        }
        else if (event->key() == Qt::Key_Minus)  {
            angulo += glm::radians(5.0f);
            //FOV  ha ser menor a 180º (FOV = 2*angulo)! angulo < 90
            if (angulo >= glm::radians(89.0f))
                angulo = glm::radians(89.0f);
            emit zoomChanged(int(glm::degrees(angulo)));
        }
        else if (event->key() == Qt::Key_C) {
            if (vista == "General") vista = "Persona";
            else vista = "General";
        }
        else if (event->key() == Qt::Key_O) {
            angleFocus += 2.0f;
            if (angleFocus > 180.0f) angleFocus = 180.0f;
            emit angleChanged(180.0f - angleFocus);
        }
        else if (event->key() == Qt::Key_P){
            angleFocus -= 2.0f;
            if (angleFocus < 0.0f) angleFocus = 0.0f;
            emit angleChanged(180.0f - angleFocus);
            //qDebug() << "angle =" << angleFocus;
        }
        else { //N
            modeNit = !modeNit;
            emit modeNitChanged(modeNit);
            update();
        }

        update();
        return; // no seguir procesando
    }

    //Bloquear movimiento personaje si la partida no esta activa
    if (!gameStarted || gameOver || victoria)
    {
        emit adverticeStartMessage();
        event->ignore();
        return;
    }
    //CONTROLES JUEGO
    switch (event->key()) {
    case Qt::Key_Left: {

        angleMovMorty += 90.0f;
        if (angleMovMorty >= 360.0f) angleMovMorty -= 360.0f; //para no tener angulos muy grandes
        //modificar dirMorty para que la camara tambien se mueva
        //derecha: 0 → 90 → 180 → 270 → 0
        if (angleMovMorty == 0.0f) dirMorty = glm::vec3 (0.0f,0.0f,1.0f); //z positivo
        else if (angleMovMorty == 90.0f) dirMorty = glm::vec3 (1.0f,0.0f,0.0f); //x positivo
        else if (angleMovMorty == 180.0f) dirMorty = glm::vec3 (0.0f,0.0f,-1.0f); //z negativo
        else dirMorty = glm::vec3 (-1.0f,0.0f,0.0f); //x negativo
        break;
    }

    case Qt::Key_Right: {
        angleMovMorty -= 90.0f;
        if (angleMovMorty < 0.0f) angleMovMorty += 360.0f;
        //modificar dirMorty para que la camara tambien se mueva
        //izquierda: 0 → 270 → 180 → 90 → 0  (ej, angle = 0-> angle -= 90 = -90 (<0)->-90+360 = 270...)
        if (angleMovMorty == 0.0f) dirMorty = glm::vec3 (0.0f,0.0f,1.0f); //z positivo
        else if (angleMovMorty == 90.0f) dirMorty = glm::vec3 (1.0f,0.0f,0.0f); //x positivo
        else if (angleMovMorty == 180.0f) dirMorty = glm::vec3 (0.0f,0.0f,-1.0f); //z negativo
        else dirMorty = glm::vec3 (-1.0f,0.0f,0.0f); //x negativo

        break;
    }
    case Qt::Key_Up: {
        //revisar si puede moverse hacia adelanta
        /*1.Buscar posición modelo en el laberinto
            !!posxMorty = j+0.5
            !!poszMorty = i+0.5
            */
        // celda actual de Morty
        int fila = int(poszMorty - 0.5f);
        int col  = int(posxMorty - 0.5f);

        // celda hacia delante
        int nuevaFila = fila + int(dirMorty.z);
        int nuevaCol  = col  + int(dirMorty.x);

        // comprobar límites
        if (nuevaFila >= 0 && nuevaFila < N &&
            nuevaCol >= 0 && nuevaCol < M) {

            // comprobar si está libre
            if (laberint[nuevaFila][nuevaCol] == 0 || laberint[nuevaFila][nuevaCol] == 5 || laberint[nuevaFila][nuevaCol] == 4) {
                //actualizamos contador monedas
                if (laberint[nuevaFila][nuevaCol] == 5) {
                    monedasObtenidas +=1;
                    //avisamos a interficie, para que muestre otro contador, (SIGNAL coinsChanged)
                    emit coinsChanged(monedasObtenidas);
                }
                // actualizar laberinto
                if (laberint[fila][col] == 2 || laberint[fila][col] == 5 ) laberint [fila][col] = 0;

                // mover Morty
                posxMorty = nuevaCol + 0.5f;
                poszMorty = nuevaFila + 0.5f;

                //si Morty toca a fantasma
                if (posxMorty == posxFantasma and poszMorty == poszFantasma) {
                    gameOver = true;
                    emit gameOverSignal();
                    return;
                }

                //Si llega a una salida y ha obtenido todas las monedas gana la partida
                if (laberint[nuevaFila][nuevaCol] == 4 and monedasObtenidas == 10) {
                    victoria = true;
                    emit victorySignal();
                    return;
                }
                //mover fantasma
                moveFantasma();
            }
        }
        break;
    }
    default: event->ignore(); break;
    }
    update();
}

void MyGLWidget::mouseReleaseEvent(QMouseEvent *){
    CurrentAction = NONE;
}

void MyGLWidget::mousePressEvent(QMouseEvent *e)
{
    xClick = e->x();
    yClick = e->y();

    if (e->button() == Qt::RightButton)  CurrentAction = ZOOM;
    else if (e->button()==Qt::LeftButton) CurrentAction = ROTATE;
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *e){
    makeCurrent();
    if (CurrentAction == ROTATE) {
        float dx = e->x() - xClick;
        float dy = e->y() - yClick;
        float sensibilidad = 0.01f;
        //Para velocidad de movimiento
        psi -= dx * sensibilidad;
        theta += dy * sensibilidad;
        xClick = e->x();
        yClick = e->y();
        emit psiChanged(int(glm::degrees(psi)));
        emit thetaChanged(int(glm::degrees(theta)));
        update();
    }
    else if (CurrentAction == ZOOM) {
        float dy = e->y() - yClick;
        float sensibilidad = 0.005f;
        angulo += dy * sensibilidad;
        if (angulo < 0.01f) angulo = 0.01f;
        //FOV ha ser menor a 180º (FOV = 2*angulo)! angulo < 90
        if (angulo > glm::radians(89.0f)) angulo = glm::radians(89.0f);
        yClick = e->y();
        emit zoomChanged(int(glm::degrees(angulo)));
        update();
    }
}

void MyGLWidget::ViewTransformVistaGeneral() {
    //AMB ANGLES DE EULER, PER ROTACIÓ

    VM = glm::mat4(1.0);

    VM = glm::translate(VM, glm::vec3(0.0f, 0.0f, -d));
    //Giro respecto a x
    VM = glm::rotate(VM, theta, glm::vec3(1.0f, 0.0f, 0.0f));
    //Giro respecto a y
    VM = glm::rotate(VM, -psi, glm::vec3(0.0f, 1.0f, 0.0f));
    VM = glm::translate(VM, -Centre);
    glUniformMatrix4fv(VMLoc, 1, GL_FALSE,  &VM[0][0]);

    /*Iteración 2 (Capsa contenidora) 3a persona
    glm::vec3 VPR = Centre;
    glm::vec3 OBS = VPR + d*glm::normalize(glm::vec3(-0.45,1,1));
    glm::vec3 UP = glm::vec3(0,1,0);

    glm::mat4 VM = glm::lookAt(OBS,VPR,UP);
    glUniformMatrix4fv (VMLoc,1,GL_FALSE,&VM[0][0]);
    */

    /*
     sense capsa contenidora (Iteració 1)
    glm::vec3 OBS = glm::vec3(-2,20,15);
    glm::vec3 VPR = glm::vec3(5,0,5);

    glm::vec3 UP = glm::vec3(0,1,0);

    glm::mat4 VM = glm::lookAt(OBS,VPR,UP);
    glUniformMatrix4fv (VMLoc,1,GL_FALSE,&VM[0][0]);
*/
}

void MyGLWidget::ProjectTransformVistaGeneral() {
    //Camamra en 3 persona, (caja contenedora)
    //1. Càlcul Pmin i Pmax i Centre capsa contenidora
    glm::vec3 Pmin, Pmax;
    Pmin = glm::vec3 (0.0,0.0,0.0);
    escala = 6.0f/172.0f;
    float diametreTorre = (xmaxTorre-xminTorre)*escala;
    //Tenir en compte les torres!
    Pmax = glm::vec3 (float(N)+2.0f*diametreTorre,6.0f,float(M)+2.0f*diametreTorre);
    //centre respecte només el laberint (normalment és pmin+pmax/2)
    Centre = (Pmin+glm::vec3 (float(N),6.0f,float(M)))/2.0f; //VPR = Centre !

    //2. Radi esfera
    float radi;
    radi = glm::distance (Pmin,Pmax)/2.0f;
    d = 2*radi;

    //3.Càlcul FOV
    float ra = float(ample)/float(alt);

    if (ra < 1.0f)
        FOV = 2.0f * atan(tan(angulo)/ra);
    else
        FOV = 2.0f * angulo;


    near = d-radi;
    far = d+radi;

    glm::mat4 PM (1.0f);
    PM= glm::perspective(FOV, ra, near, far);
    glUniformMatrix4fv(PMLoc, 1, GL_FALSE, &PM[0][0]);
    /*
     Sense capsa contenidora (Iteració 2)
    glm::mat4 PM (1.0f);
    FOV = glm::radians(90.0f);
    float ra = float(ample)/float(alt);
    near = 10.0f;
    far = 35.0f;
    PM= glm::perspective(FOV, ra, near, far);
    glUniformMatrix4fv(PMLoc, 1, GL_FALSE, &PM[0][0])
*/
}


void MyGLWidget::ViewTransformVistaPersona() {
    glm::vec3 OBS = glm::vec3(posxMorty,0.5f,poszMorty);

    glm::vec3 VPR = OBS + dirMorty;

    glm::vec3 UP = glm::vec3(0,1,0);

    VM = glm::lookAt(OBS,VPR,UP);
    glUniformMatrix4fv (VMLoc,1,GL_FALSE,&VM[0][0]);

}
void MyGLWidget::ProjectTransformVistaPersona() {
    glm::mat4 PM (1.0f);
    FOV = glm::radians(90.0f);
    float ra = float(ample)/float(alt);
    near = 0.1f;
    far = 15.0f;
    PM= glm::perspective(FOV, ra, near, far);
    glUniformMatrix4fv(PMLoc, 1, GL_FALSE, &PM[0][0]);
}

void MyGLWidget::ViewTransformMiniMapa() {
    glm::vec3 VPR = Centre;
    // cámara encima del centro
    glm::vec3 OBS = Centre + glm::vec3(0.0f, d, 0.0f);

    glm::vec3 UP = glm::vec3(0.0f, 0.0f, -1.0f);

    VM = glm::lookAt(OBS, VPR, UP);

    glUniformMatrix4fv(VMLoc, 1, GL_FALSE, &VM[0][0]);
}

void MyGLWidget::ProjectTransformMiniMapa() {
    //Camara ortografica (todo se ve del mismo tamaño, no perspectiva)
    float mida = std::max(N, M);
    /*glm::ortho( left, right, bottom,top,near,far)
    ej: glm::ortho(-5.f, 5.f,-5.f, 5.f,1.f, 50.f);
    significa:  "Todo lo que esté entre x=-5 y x=5 y entre y=-5 y y=5 en coordenadas de cámara se verá."
    */
    float diametreTorre = (xmaxTorre-xminTorre)*escala;
    glm::mat4 PM = glm::ortho( -mida/2.0f-diametreTorre, mida/2.0f+diametreTorre,-mida/2.0f-diametreTorre,mida/2.0f+diametreTorre,0.1f,50.0f);
    glUniformMatrix4fv(PMLoc, 1, GL_FALSE, &PM[0][0]);
}

//Reinicialitzem l'escena (càmeres, monedes regenerades, posició del personatge i enemic, etc.)
void MyGLWidget::startGame()
{
    gameStarted = true;
    gameOver = false;
    victoria = false;
    monedasObtenidas = 0;
    emit coinsChanged(monedasObtenidas);
    emit hideWinMessage();
    emit hideOverMessage();
    emit hideAdverticeMessage();
    //para que slider aparezcan en la mitad (mitad de min y max)
    emit zoomChanged(45);
    emit thetaChanged (0);
    emit angleChanged(90); // Asegurar notificación del slider de luz

    glEnable(GL_DEPTH_TEST); //Important per pintar
    definicioMatriuLaberint();

    //Limpiar monedas anteriores
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            if (laberint[i][j] == 5) {
                laberint[i][j] = 0;
            }
        }
    }
    srand(time(NULL)); //para generar numeros aleatorios diferentes

    generaMonedas(10);
    //datos para ubicar la camara en primera persona
    inicializaPosMorty();
    inicializaPosFantasma();

    vista = "General";
    angleCoins = 0.0f;
    angleFocus = 90.0f;

    //Angles de Euler i moviment càmara
    psi = glm::radians(0.0f);
    theta = glm::radians(45.0f);
    setMouseTracking(true);
    CurrentAction = NONE;
    xClick = 0;
    yClick = 0;
    //Inicialitzem càlcul del angulo del FOV aqui, ja que pel zoom pot ser modificat
    //1. Càlcul Pmin i Pmax i Centre capsa contenidora
    glm::vec3 Pmin, Pmax;
    Pmin = glm::vec3 (0.0,0.0,0.0);
    escala = 6.0f/172.0f;
    float diametreTorre = (xmaxTorre-xminTorre)*escala;
    //Tenir en compte les torres!
    Pmax = glm::vec3 (float(N)+2.0f*diametreTorre,6.0f,float(M)+2.0f*diametreTorre);
    //centre respecte només el laberint (normalment és pmin+pmax/2)
    Centre = (Pmin+glm::vec3 (float(N),6.0f,float(M)))/2.0f; //VPR = Centre !

    //2. Radi esfera
    float radi;
    radi = glm::distance (Pmin,Pmax)/2.0f;
    //pos Luz
    radiusFocus = radi;
    d = 2*radi;

    //3.Càlcul FOV
    angulo = glm::asin (radi/d);

    glUseProgram(program->programId());
    update();
}

//giro respecto a y
void MyGLWidget::setPsi(int value)
{
    psi = glm::radians(float(value));
    update();
}

//giro respecto a x
void MyGLWidget::setTheta(int value)
{
    theta = glm::radians(float(value));
    update();
}

void MyGLWidget::setZoom(int value)
{

    angulo = glm::radians(float(value));

    //L'angle de obertura ha de ser major a 0º!
    if (angulo < 0.01f) angulo = 0.01f;

    //FOV  ha ser menor a 180º (FOV = 2*angulo)! angulo < 90
    if (angulo > glm::radians(89.0f)) angulo = glm::radians(89.0f);

    update();
}

void MyGLWidget::setVistaGeneral() {

    if (vista == "Persona") {
        vista = "General";
        update();
    }
}

void MyGLWidget::setVistaPersona() {
    if (vista == "General") {
        vista = "Persona";
        update();
    }
}

//Cambiar color iluminación
void MyGLWidget::onChangeLightColor()
{
    QColor c = QColorDialog::getColor(lightColor, this);

    if (c.isValid()) {
        lightColor = c;
        update();
    }
}


//Pos luz
void MyGLWidget::setAngleFocus(int value) {

    angleFocus = 180.0f - float(value); //inverso , ya que cuando el slider se mueva a la derecha el sol tam,bien lo hara hacia ese lado
    update();
}



void MyGLWidget::setModeNit(bool actiu) {
    makeCurrent();
    modeNit = actiu;
    update();
}


void MyGLWidget::setRotacionMonedas(bool activa) {
    makeCurrent();
    if (rotacionMonedas != activa) {
        rotacionMonedas = activa;
        update();
    }
}
