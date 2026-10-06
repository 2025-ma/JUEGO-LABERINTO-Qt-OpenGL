#include "LaberintBase.h"

#include <iostream>

LaberintBase::LaberintBase (QWidget* parent) : QOpenGLWidget(parent), program(NULL)
{
    setFocusPolicy(Qt::StrongFocus);  // per rebre events de teclat
}

LaberintBase::~LaberintBase ()
{
    if (program != NULL)
        delete program;
}

void LaberintBase::initializeGL ()
{    // Cal inicialitzar l'ús de les funcions d'OpenGL
    initializeOpenGLFunctions();

    glClearColor(0.5, 0.7, 1.0, 1.0); // defineix color de fons (d'esborrat)
    carregaShaders();
    creaBuffers();
    escala = 1.0f;
}

void LaberintBase::paintGL ()
{
    // En cas de voler canviar els paràmetres del viewport, descomenteu la crida següent i
    // useu els paràmetres que considereu (els que hi ha són els de per defecte)
    //  glViewport (0, 0, ample, alt);

    // Esborrem el frame-buffer
    glClear (GL_COLOR_BUFFER_BIT);

    // Carreguem la transformació de model
    modelTransform ();

    // Activem el VAO per a pintar la caseta
    glBindVertexArray (VAO_Casa);

    // pintem
    glDrawArrays(GL_TRIANGLES, 0, 9);

    glBindVertexArray (0);
}

void LaberintBase::modelTransform ()
{
    // Matriu de transformació de model
    glm::mat4 transform (1.0f);
    transform = glm::scale(transform, glm::vec3(escala));
    glUniformMatrix4fv(transLoc, 1, GL_FALSE, &transform[0][0]);
}

void LaberintBase::resizeGL (int w, int h)
{
// Aquest codi és necessari únicament per a MACs amb pantalla retina.
#ifdef __APPLE__
    GLint vp[4];
    glGetIntegerv (GL_VIEWPORT, vp);
    ample = vp[2];
    alt = vp[3];
#else
    ample = w;
    alt = h;
#endif
}

void LaberintBase::keyPressEvent(QKeyEvent* event)
{
    makeCurrent();
    switch (event->key()) {
    case Qt::Key_S: { // escalar a més gran
        escala += 0.05;
        break;
    }
    case Qt::Key_D: { // escalar a més petit
        escala -= 0.05;
        break;
    }
    default: event->ignore(); break;
    }
    update();
}

void LaberintBase::creaBuffers ()
{
    // Dades de la caseta
    // Dos VBOs, un amb posició i l'altre amb color
    glm::vec3 posicio[9] = {
        glm::vec3(-0.5, -1.0, -0.5),
        glm::vec3( 0.5, -1.0, -0.5),
        glm::vec3(-0.5,  0.0, -0.5),
        glm::vec3(-0.5,  0.0, -0.5),
        glm::vec3( 0.5, -1.0, -0.5),
        glm::vec3( 0.5,  0.0, -0.5),
        glm::vec3( 0.5,  0.0, -0.5),
        glm::vec3( 0.0,  0.6, -0.5),
        glm::vec3(-0.5,  0.0, -0.5)
    };
    glm::vec3 color[9] = {
        glm::vec3(1,0,0),
        glm::vec3(0,1,0),
        glm::vec3(0,0,1),
        glm::vec3(0,0,1),
        glm::vec3(0,1,0),
        glm::vec3(1,0,0),
        glm::vec3(1,0,0),
        glm::vec3(0,1,0),
        glm::vec3(0,0,1)
    };

    // Creació del Vertex Array Object per pintar
    glGenVertexArrays(1, &VAO_Casa);
    glBindVertexArray(VAO_Casa);

    GLuint VBO_Casa[2];
    glGenBuffers(2, VBO_Casa);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Casa[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(posicio), posicio, GL_STATIC_DRAW);

    // Activem l'atribut vertexLoc
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertexLoc);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_Casa[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color), color, GL_STATIC_DRAW);

    // Activem l'atribut colorLoc
    glVertexAttribPointer(colorLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(colorLoc);

    glBindVertexArray (0);
}

void LaberintBase::carregaShaders()
{
    // Creem els shaders per al fragment shader i el vertex shader
    QOpenGLShader fs (QOpenGLShader::Fragment, this);
    QOpenGLShader vs (QOpenGLShader::Vertex, this);
    // Carreguem el codi dels fitxers i els compilem
    fs.compileSourceFile("shaders/basicShader.frag");
    vs.compileSourceFile("shaders/basicShader.vert");
    // Creem el program
    program = new QOpenGLShaderProgram(this);
    // Li afegim els shaders corresponents
    program->addShader(&fs);
    program->addShader(&vs);
    // Linkem el program
    program->link();
    // Indiquem que aquest és el program que volem usar
    program->bind();

    // Obtenim identificador per a l'atribut “vertex” del vertex shader
    vertexLoc = glGetAttribLocation (program->programId(), "vertex");
    // Obtenim identificador per a l'atribut “color” del vertex shader
    colorLoc = glGetAttribLocation (program->programId(), "color");
    // Uniform locations
    transLoc = glGetUniformLocation(program->programId(), "TG");
}


int LaberintBase::printOglError(const char file[], int line, const char func[])
{
    GLenum glErr;
    int    retCode = 0;

    glErr = glGetError();
    const char * error = 0;
    switch (glErr)
    {
    case 0x0500:
        error = "GL_INVALID_ENUM";
        break;
    case 0x501:
        error = "GL_INVALID_VALUE";
        break;
    case 0x502:
        error = "GL_INVALID_OPERATION";
        break;
    case 0x503:
        error = "GL_STACK_OVERFLOW";
        break;
    case 0x504:
        error = "GL_STACK_UNDERFLOW";
        break;
    case 0x505:
        error = "GL_OUT_OF_MEMORY";
        break;
    default:
        error = "unknown error!";
    }
    if (glErr != GL_NO_ERROR)
    {
        printf("glError in file %s @ line %d: %s function: %s\n",
               file, line, error, func);
        std::cout << std::endl;
        retCode = 1;
    }
    return retCode;
}
