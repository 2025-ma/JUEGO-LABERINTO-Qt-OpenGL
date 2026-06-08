// MyGLWidget.h
#include "BL2GLWidget.h"
#include "Model/model.h"
#include "assimp/Mesh.h"
//para el giro de las monedas
#include <QTimer>
#include <QCursor>
#include <QMouseEvent>
#include <QColorDialog>

class MyGLWidget : public BL2GLWidget {
  Q_OBJECT

  public:
   MyGLWidget(QWidget *parent=0);
    ~MyGLWidget();

  protected:
    // initializeGL - Aqui incluim les inicialitzacions del contexte grafic.
    void initializeGL( )  override;

    void creaBuffersMorty ();
    void creaBuffersFantasma ();
    void creaBuffersMoneda();
    void creaBuffersCub();
    void creaBuffersCubText(); //texturas
    void creaBuffersTorre();
    void creaBuffersTorreText(); //texturas


    void carregaShaders () override;

    void definicioMatriuLaberint();
    //posicion aleatoria de las monedas en el laberinto
    void generaMonedas(int numMonedas);

    // paintGL - Mètode cridat cada cop que cal refrescar la finestra.
    // Tot el que es dibuixa es dibuixa aqui.
    void paintGL( ) override;
    void pintarSegonsLaberint();

    void resizeGL(int width, int height) override;

    void ViewTransformVistaGeneral();
    void ProjectTransformVistaGeneral();

    void ViewTransformVistaPersona();
    void ProjectTransformVistaPersona();

    void ViewTransformMiniMapa();
    //Camara ortográfica
    void ProjectTransformMiniMapa();

    //para datos de la camara en primera persona y movimiento
    void inicializaPosMorty();

    //para datos de movimiento Fantasma
    void inicializaPosFantasma();
    void moveFantasma();

    //Transformacions models
    void modelTransformMorty();
    void modelTransformFantasma();
    void modelTransformMoneda(int fila, int colum);
    //sin rotación y mirando hacia arriba
    void modelTransformMonedaMiniMapa (int fila, int colum);
    void modelTransformTorre(int fila, int colum);
    void modelTransformCub (int fila, int colum);
    void modelTransformTerra (int fila, int colum);


    //Càlcul de les posicions i mides inicials
    void iniPosMorty();
    void iniPosFantasma();
    void iniPosMoneda();
    void iniPosTorre();
    void iniPosParet();


    // keyPressEvent - Es cridat quan es prem una tecla
    void keyPressEvent (QKeyEvent *event) override;
    void mouseMoveEvent (QMouseEvent* e) override;
    void mousePressEvent (QMouseEvent * e ) override;
    void mouseReleaseEvent (QMouseEvent * e ) override;

    //Para cargar modelos
    GLuint VAO_Homer, VAO_Cub, VAO_Morty, VAO_Fantasma, VAO_Moneda, VAO_Torre;
    //Model homerModel;
    Model MortyModel, FantasmaModel, MonedaModel, TorreModel;

    Mesh *blockTorre; // Para el modelo de las torres texturizadas
    Mesh *blockParet; // Para el modelo de las paredes texturizadas (sustituye al VAO_Cub en paredes)

    static const int N = 10;
    static const int M = 10;

    int laberint[N][M];

    //medidas bounding box torre
    float xminTorre, xmaxTorre;
    float yminTorre, ymaxTorre;
    float zminTorre, zmaxTorre;

    //medidas bounding box cub
    float xminCub, xmaxCub;
    float yminCub, ymaxCub;
    float zminCub, zmaxCub;


    //medidas Morty
    float xminMorty, xmaxMorty;
    float yminMorty, ymaxMorty;
    float zminMorty, zmaxMorty;

    //medidas Fantasma
    float xminFantasma, xmaxFantasma;
    float yminFantasma, ymaxFantasma;
    float zminFantasma, zmaxFantasma;

    //medidas Moneda
    float xminMoneda, xmaxMoneda;
    float yminMoneda, ymaxMoneda;
    float zminMoneda, zmaxMoneda;

    //medidas paret
    float xminParet, xmaxParet;
    float yminParet, ymaxParet;
    float zminParet, zmaxParet;

    //PARA SABER EN QUE VISTA NOS ENCONTRAMOS
    std::string vista;

    float FOV,near,far, angulo,d;
    glm::vec3 Centre;

    //Posición MOrty para datos necesarios en camara en primera persona y movimiento del modelo
    float posxMorty, poszMorty;
    //Inicialmente mira hacia  z postiva
    glm::vec3 dirMorty = glm::vec3 (0.0f,0.0f,1.0f);
    float angleMovMorty = 0.0f;

    //Posición Fantasma para datos necesarios en movimiento del modelo
    float posxFantasma, poszFantasma;
    glm::vec3 dirFantasma;
    float angleMovFantasma = 0.0f;

    //para giro de las monedas
    QTimer timer;
    float angleCoins = 0.0f;

    //Contador monedas
    int monedasObtenidas = 0;

    //Per la VM amb angles de Euler
        /*ψ (psi) angle de gir respecte Y
      θ (theta)angle de gir respecte X
    */
    float psi, theta;
    //Per moviment i zoom de la càmera
    //posiciones anteriores del cursor
    int xClick;
    int yClick;

    typedef enum {ROTATE, ZOOM, NONE} Action;
    Action CurrentAction;

    bool pintarMonedaMiniMapa;
    bool miniMapa;

    //Per controlar part interficie
    // Estado juego
    bool gameStarted = false;
    bool gameOver = false;
    bool victoria = false;

    // Monedas
    int totalMonedas = 10;

//PARAMETRES ILUMINACIÓ
    // Definim els paràmetres del material
    glm::vec3 amb, diff, spec;
    float shin;

    // uniform locations
    GLuint PMLoc, VMLoc;

    glm::mat4 VM = glm::mat4(1.0f);

    GLint colorFocusLoc, posFocusLoc, modeNitLoc, posLanternaLoc, posFantasmaLoc;
    // attribute locations
    GLuint vertexLoc, normalLoc, matambLoc, matdiffLoc, matspecLoc, matshinLoc, texUVLoc, texLoc, texActiveLoc;

    //Iluminación monedas
    glm::mat4 calcularTGMoneda (int fila, int colum, bool esMinimapa); //para iluminación monedas
    GLuint quantitatMonedesLoc;
    GLuint posMonedesSCOLoc;
    GLuint dirMonedesSCOLoc;
    GLuint colorMonedaLlumLoc;

    //para color boton cambiar color de iluminacion
    QColor lightColor = Qt::white;

    //movimiento posición luz (sol)
    float angleFocus = 0.0f;
    float radiusFocus;

    bool modeNit = false;

    bool rotacionMonedas = true;


public slots:
    //slot de giro de monedas
    void rotateCoins();

    //Slots per la UI
    void startGame();

    //cambios camara
    //void setZoom(int value);

    void setPsi(int value);
    void setTheta(int value);
    void setZoom(int value);

    void setVistaGeneral();
    void setVistaPersona();

    //cambiar color iluminación
    void onChangeLightColor();

    void setAngleFocus(int value); //Pos llum

    void setModeNit(bool actiu);

    void setRotacionMonedas(bool activa);
signals:

    void coinsChanged(int actual);

    void victorySignal();
    void hideWinMessage();

    void gameOverSignal();
    void hideOverMessage();

    //por si la persona intenta mover personaje sin haber iniciado el juego
    void adverticeStartMessage();
    void hideAdverticeMessage();

    //movimiento camara
    void psiChanged(int value);
    void thetaChanged(int value);
    void zoomChanged(int);

    void angleChanged(int);

    void modeNitChanged (bool);

    void rotateCoins (bool);


};
