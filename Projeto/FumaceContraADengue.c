#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Definindo constantes para o jogo
#define INITIAL_NUM_TARGETS 5
#define MAX_NUM_TARGETS 25
#define INITIAL_GAME_DURATION 200  // Dura��o inicial do jogo em segundos
#define MAX_LEVELS 2  // N�mero m�ximo de fases

// Estrutura para armazenar informa��es sobre a flecha
typedef struct {
    float x;
    float y;
    float angle;
    int fired;
    float speed;
} Jet;

// Estrutura para armazenar informa��es sobre os alvos
typedef struct {
    float x;
    float y;
    float radius;
    int hit;
    float orbitRadius;  // Raio da �rbita
    float orbitCenterX; // Centro da �rbita X
    float orbitCenterY; // Centro da �rbita Y
    float orbitAngle;   // �ngulo da �rbita
    float orbitSpeed;   // Velocidade da �rbita
} Target;

// Inicializa��o das vari�veis globais
Jet jet = { -0.8, -0.2, 45.0, 0, 0.03 };  // Inicializa o fumac� com uma posi��o, �ngulo e velocidade
Jet initialJet = { -0.8, -0.2, 45.0, 0, 0.03 };  // Armazena a posi��o inicial do carro
Target targets[MAX_NUM_TARGETS];  // Vetor para armazenar os alvos
int numTargets = INITIAL_NUM_TARGETS;  // N�mero de alvos iniciais
int gameRunning = 1;  // Vari�vel para verificar se o jogo est� em execu��o
time_t startTime;  // Tempo de in�cio do jogo
int gameDuration = INITIAL_GAME_DURATION;  // Dura��o do jogo
int currentLevel = 1;  // N�vel atual
char statusMessage[50] = "";  // Mensagem de status
GLuint textureID;

// Fun��es do jogo
void initTargets();
void nextLevel(int value);
void drawText(float x, float y, char *string);
void drawJet(float x, float y, float angle);
void drawTarget(Target target);
void drawLauncher(float x, float y);
void display();
void update(int value);
void handleMouseMotion(int x, int y);
void handleMouse(int button, int state, int x, int y);
void handleKeys(unsigned char key, int x, int y);
void init();

//fun��o para carregar a textura
void loadTexture(const char* filename) {
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (!image) {
        printf("Falha ao carregar a imagem da textura.\n");
        exit(1);
    }
    
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Definindo os par�metros da textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Carregando a imagem da textura na mem�ria do OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    
    // Liberando a mem�ria da imagem
    stbi_image_free(image);
}

// Fun��o para inicializar os alvos
void initTargets() {
    int i;
    for (i = 0; i < numTargets; i++) {
        targets[i].orbitCenterX = ((float)rand() / RAND_MAX) * 1.6 - 0.8;  // Centro da �rbita em x entre -0.8 e 0.8
        targets[i].orbitCenterY = ((float)rand() / RAND_MAX) * 1.6 - 0.8;  // Centro da �rbita em y entre -0.8 e 0.8
        targets[i].orbitRadius = 0.2 + ((float)rand() / RAND_MAX) * 0.3;   // Raio da �rbita entre 0.2 e 0.5
        targets[i].orbitAngle = ((float)rand() / RAND_MAX) * 360.0;        // �ngulo inicial aleat�rio
        targets[i].orbitSpeed = 0.5 + ((float)rand() / RAND_MAX) * 0.5;    // Velocidade da �rbita entre 0.5 e 1.0
        targets[i].radius = 0.05;       // Define o raio do alvo
        targets[i].hit = 0;             // Inicializa o alvo como n�o acertado
    }
}

// Fun��o para passar para o pr�ximo n�vel
void nextLevel(int value) {
    if (currentLevel > MAX_LEVELS) {
        snprintf(statusMessage, sizeof(statusMessage), "Voc� Venceu! Dengue erradicada!");
        gameRunning = 0;
        return;
    }
    if (numTargets < MAX_NUM_TARGETS) {
        numTargets += 5;  // Aumenta o n�mero de alvos
    }
    if (gameDuration > 10) {
        gameDuration -= 5;  // Diminui a dura��o do jogo
    }
    initTargets();  // Inicializa os alvos
    jet = initialJet;  // Restaura o jato para a posi��o inicial
    jet.angle = 45.0;
    jet.fired = 0;
    startTime = time(NULL);  // Reinicia o timer do jogo
    snprintf(statusMessage, sizeof(statusMessage), "Level %d", currentLevel);
    gameRunning = 1;
    glutTimerFunc(16, update, 0);  // Reinicia a fun��o de atualiza��o
}

// Fun��o para desenhar texto na tela
void drawText(float x, float y, char *string) {
    glColor3f(1.0, 1.0, 1.0); // Cor do texto
    glRasterPos2f(x, y);
    while (*string) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *string);
        string++;
    }
}

// Fun��o para desenhar o jato do fumac�
void drawJet(float x, float y, float angle) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glRotatef(angle, 0, 0, 1);
    glBegin(GL_LINES);
    glVertex2f(0.0, 0.0);
    glVertex2f(0.1, 0.0);  // Define o tamanho do rastro
    glEnd();
    glPopMatrix();
}

// Fun��o para desenhar um alvo
void drawTarget(Target target) {
    if (target.hit) return;  // Pula o desenho se o alvo j� foi acertado

    //glColor3f(0.6, 0.6, 0.6);  // Define a cor do alvo
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
	
	glColor3f(1.0, 1.0, 1.0);
	
	glBegin(GL_POLYGON);
    int i;
    for (i = 0; i < 360; i++) {
        float theta = i * 3.14159 / 180;
        float x = target.x + target.radius * cos(theta);
        float y = target.y + target.radius * sin(theta);
        float s = (cos(theta) + 1.0) / 2.0;
        float t = (sin(theta) + 1.0) / 2.0;
    	glTexCoord2f(s, t);
    	glVertex2f(x, y);
	}
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

// Fun��o para desenhar o carro
void drawLauncher(float x, float y) {
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_POLYGON);
    glVertex2f(x - 0.05, y - 0.05);
    glVertex2f(x + 0.05, y - 0.05);
    glVertex2f(x + 0.05, y + 0.05);
    glVertex2f(x - 0.05, y + 0.05);
    glEnd();
}

// Fun��o de exibi��o
void display() {
    glClear(GL_COLOR_BUFFER_BIT);  // Limpa o buffer de cor

    // Desenha o ch�o
    glColor3f(0.0, 1.0, 0.0);
    glBegin(GL_POLYGON);
    glVertex2f(-1.0, -0.3);
    glVertex2f(1.0, -0.3);
    glVertex2f(1.0, -1.0);
    glVertex2f(-1.0, -1.0);
    glEnd();

    // Desenha o carro logo abaixo do jato de fuma�a
    drawLauncher(initialJet.x, initialJet.y - 0.05);

    // Desenha o rastro
    glColor3f(0.0, 0.0, 0.0);
    drawJet(jet.x, jet.y, jet.angle);

    // Desenha os alvos
    int i;
    for (i = 0; i < numTargets; i++) {
        drawTarget(targets[i]);
    }

    // Exibe a mensagem de status
    drawText(-0.9, 0.9, statusMessage);

    // Exibe o tempo restante
    time_t currentTime = time(NULL);
    int timeLeft = gameDuration - (int)difftime(currentTime, startTime);
    char timeMessage[50];
    snprintf(timeMessage, sizeof(timeMessage), "Tempo restante: %d segundos", timeLeft);
    drawText(-0.9, 0.8, timeMessage);

    
    glutSwapBuffers();  // Troca os buffers de exibi��o
}

// Fun��o de atualiza��o
void update(int value) {
    if (gameRunning) {
        // Atualiza a posi��o do rastro se tiver sido diparado
        if (jet.fired) {
            float angle_rad = jet.angle * 3.14159 / 180.0; //converte as cordenadas em radianos
            jet.x += jet.speed * cos(angle_rad);
            jet.y += jet.speed * sin(angle_rad);

            // Verifica colis�o com os alvos
            int i;
            for (i = 0; i < numTargets; i++) {
                if (!targets[i].hit && sqrt(pow(jet.x - targets[i].x, 2) + pow(jet.y - targets[i].y, 2)) <= targets[i].radius) { //distancia entre o rastro e o raio do alvo
                    printf("Hit target %d!\n", i);
                    snprintf(statusMessage, sizeof(statusMessage), "Mosquito %d atingido!", i + 1);
                    targets[i].hit = 1;
                }
            }

            // Verifica se o rastro se dispersou (saiu dos limites da janela)
            if (jet.x > 1.0 || jet.x < -1.0 || jet.y > 1.0 || jet.y < -1.0) {
                jet.fired = 0;
                jet = initialJet;  // Restaura o jato para a posi��o inicial
                jet.angle = 45.0;
            }
        }

        // Verifica se todos os alvos foram acertados
        int allHit = 1;
        int i;
        for (i = 0; i < numTargets; i++) {
            if (!targets[i].hit) {
                allHit = 0;
                break;
            }
        }
        if (allHit) {
            printf("VITORIA!\n");
            if (currentLevel < MAX_LEVELS){
            	snprintf(statusMessage, sizeof(statusMessage), "Mosquitos exterminados! Pr�ximo level...");
			}
			else{
				snprintf(statusMessage, sizeof(statusMessage), "VITORIA! Dengue Erradicada");
			}
            gameRunning = 0;
            currentLevel ++;
            glutTimerFunc(2000, nextLevel, 0);  // Passa para o pr�ximo n�vel ap�s 1 segundo
        }

        // Verifica se o tempo acabou
        time_t currentTime = time(NULL);
        if (difftime(currentTime, startTime) >= gameDuration) {
            printf("O tempo acabou!\n");
            snprintf(statusMessage, sizeof(statusMessage), "O tempo acabou! A dengue est� disseminada!");
            gameRunning = 0;
        }

        // Atualiza a posi��o dos alvos com base na �rbita
        for (i = 0; i < numTargets; i++) {
            if (!targets[i].hit) {
                targets[i].orbitAngle += targets[i].orbitSpeed;
                targets[i].x = targets[i].orbitCenterX + targets[i].orbitRadius * cos(targets[i].orbitAngle * 3.14159 / 180.0);
                targets[i].y = targets[i].orbitCenterY + targets[i].orbitRadius * sin(targets[i].orbitAngle * 3.14159 / 180.0);

                // Limita a movimenta��o dos alvos dentro da janela
                if (targets[i].x + targets[i].radius > 1.0 || targets[i].x - targets[i].radius < -1.0) {
                    targets[i].orbitSpeed = -targets[i].orbitSpeed;
                }
                if (targets[i].y + targets[i].radius > 1.0 || targets[i].y - targets[i].radius < -1.0) {
                    targets[i].orbitSpeed = -targets[i].orbitSpeed;
                }

                if (targets[i].x >= 1.0 || targets[i].x <= -1.0 || targets[i].y >= 1.0 || targets[i].y <= -1.0) {
                    targets[i].x = 0.0;  // Reposiciona o alvo para o centro
                    targets[i].y = 0.0;
                    targets[i].orbitCenterX = 0.0;  // Redefine o centro da �rbita
                    targets[i].orbitCenterY = 0.0;
                }
            }
        }

        glutPostRedisplay();  // Marca a janela atual para reexibi��o
        glutTimerFunc(16, update, 0);  // Chama update novamente ap�s 16 ms
    }
}

// Fun��o com o movimento do mouse
void handleMouseMotion(int x, int y) {
	if (!jet.fired && gameRunning) {
        // Converte as coordenadas da tela para coordenadas de mundo
        int windowWidth = glutGet(GLUT_WINDOW_WIDTH);
        int windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
        float nx = 2.0f * x / windowWidth - 1.0f;
        float ny = 1.0f - 2.0f * y / windowHeight;

        // Calcula o �ngulo entre a mira e a posi��o do mouse
        float dx = nx - jet.x;
        float dy = ny - jet.y;
        jet.angle = atan2(dy, dx) * 180.0 / 3.14159;
    }
}

// Fun��o com os cliques do mouse
void handleMouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && !jet.fired && gameRunning) {
        jet.fired = 1;  // Dispara o jato
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN && jet.fired) {
        jet.fired = 0;
        jet = initialJet;  // Prepara o pr�ximo disparo
        jet.angle = 45.0;
    }
}

// Fun��o com as teclas do teclado
void handleKeys(unsigned char key, int x, int y) {
	if (gameRunning) {
        if (key == 'a' && !jet.fired) {
            jet.x -= 0.05;  // Move o carro para a esquerda
            if (jet.x < -1.0) jet.x = -1.0;  // Limita o movimento � borda esquerda da janela
            initialJet = jet;  // Atualiza a posi��o inicial do carro
        }
        if (key == 'd' && !jet.fired) {
            jet.x += 0.05;  // Move o carro para a direita
            if (jet.x > 1.0) jet.x = 1.0;  // Limita o movimento � borda direita da janela
            initialJet = jet;  // Atualiza a posi��o inicial do carro
        }
     	  
    }
}

// Fun��o de inicializa��o do jogo
void init() {
    glClearColor(0.5, 0.8, 1.0, 1.0);  // Define a cor de fundo
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);  // Define a proje��o ortogr�fica
    initTargets();  // Inicializa os alvos
    startTime = time(NULL);  // Inicia o timer do jogo
    initialJet = jet;  // Inicializa a posi��o inicial do carro
    loadTexture("mosquito.png");
}

// Fun��o principal
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);  // Define o modo de exibi��o
    glutInitWindowSize(1200, 900);  // Define o tamanho da janela
    glutCreateWindow("Fumace contra a Dengue");  // Cria a janela do jogo

    init();  // Inicializa o jogo

    glutDisplayFunc(display);  // Registra a fun��o de exibi��o
    glutKeyboardFunc(handleKeys);  // Registra a fun��o de teclas
    glutMouseFunc(handleMouse);  // Registra a fun��o de cliques do mouse
    glutPassiveMotionFunc(handleMouseMotion);  // Registra a fun��o de movimento do mouse
    glutTimerFunc(16, update, 0);  // Inicia a fun��o de atualiza��o

    glutMainLoop();  // Inicia o loop principal do GLUT

    return 0;
}

