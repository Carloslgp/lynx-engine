#include <SDL.h>
#include <glad/glad.h>
#include <algorithm>
#include <cmath>
#include <string>

// O stb_image e uma biblioteca "header only": o codigo dela mora dentro do
// proprio .h. Esse #define liga a parte de implementacao, e ele tem que
// aparecer em EXATAMENTE UM arquivo .cpp do projeto, senao da erro de simbolo
// repetido na hora de linkar.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// Tamanho da janela, em pixels. Deixamos como constante porque varios
// calculos (limites da tela, conversao de coordenadas) precisam desses valores.
const int LARGURA = 800;
const int ALTURA  = 600;

// A arte do sapo e 32x32. Nessa resolucao ela fica minuscula na tela, entao
// desenhamos ela ampliada. 3 = cada pixel da arte vira um quadrado de 3x3.
const float ESCALA = 3.0f;

// Quantos pixels o sapo anda por SEGUNDO (nao por frame!).
const float VELOCIDADE = 260.0f;


// ---------------------------------------------------------------------------
// SHADERS
// ---------------------------------------------------------------------------
// Shader = um programinha que roda dentro da placa de video. No OpenGL moderno
// nada aparece na tela sem eles. Precisamos de dois:
//   - vertex shader:   roda 1x por vertice (canto) e decide ONDE ele fica
//   - fragment shader: roda 1x por pixel   e decide QUAL COR ele tem
// O R"( ... )" e uma "raw string" do C++: uma string que pode ter varias
// linhas e aspas dentro sem precisar escapar nada.

const char* CODIGO_VERTEX = R"(
#version 330 core

// Entrada: o canto do quadrado, com valores de 0 a 1.
// (0,0) = canto superior esquerdo, (1,1) = canto inferior direito.
layout (location = 0) in vec2 aPos;

uniform vec2 uPos;   // onde o retangulo comeca, em pixels
uniform vec2 uTam;   // largura e altura do retangulo, em pixels
uniform vec2 uTela;  // tamanho da janela, em pixels

// Manda pro fragment shader qual ponto da imagem este canto representa.
out vec2 vUV;

void main() {
    // Damos sorte aqui: o mesmo 0..1 que usamos pros cantos do quadrado serve
    // como coordenada da textura. (0,0) e o canto de cima da imagem, que e
    // exatamente onde queremos o canto de cima do quadrado na tela.
    vUV = aPos;

    // 1) Do quadrado 0..1 para a posicao real em pixels na tela.
    vec2 pixel = uPos + aPos * uTam;

    // 2) De pixels para NDC, que e o unico sistema que o OpenGL entende:
    //    a tela inteira vai de -1 a +1 nos dois eixos.
    vec2 ndc = (pixel / uTela) * 2.0 - 1.0;

    // 3) O OpenGL tem o Y crescendo pra CIMA, mas pensar em tela e mais facil
    //    com o Y crescendo pra BAIXO (0 = topo). Entao invertemos o Y aqui.
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
}
)";

const char* CODIGO_FRAGMENT = R"(
#version 330 core

in vec2 vUV;                 // veio do vertex shader, ja interpolado
uniform sampler2D uTextura;  // a imagem carregada do PNG
out vec4 FragColor;          // a cor final que este pixel vai ter

void main() {
    // Le a cor da imagem naquele ponto. O .a (alpha) vem do PNG: onde o
    // desenho e transparente ele vale 0, e o blend cuida de nao pintar nada.
    FragColor = texture(uTextura, vUV);
}
)";


// Compila UM shader e avisa no console se o codigo tiver erro de sintaxe.
// Sem essa checagem, um erro no shader simplesmente resulta numa tela preta
// sem nenhuma mensagem: e o bug mais chato de procurar em OpenGL.
GLuint compilarShader(GLenum tipo, const char* codigo) {
    GLuint shader = glCreateShader(tipo);
    glShaderSource(shader, 1, &codigo, nullptr);
    glCompileShader(shader);

    GLint deuCerto = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &deuCerto);
    if (!deuCerto) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        SDL_Log("Erro ao compilar shader: %s", log);
    }
    return shader;
}


// Carrega um PNG do disco e sobe ele pra memoria da placa de video.
// Devolve o "id" da textura, e escreve o tamanho da imagem em largura/altura.
GLuint carregarTextura(const std::string& caminho, int& largura, int& altura) {
    // O ultimo parametro (4) pede pro stb converter tudo pra RGBA, assim nao
    // precisamos tratar PNG com e sem transparencia de formas diferentes.
    int canais = 0;
    unsigned char* pixels = stbi_load(caminho.c_str(), &largura, &altura, &canais, 4);
    if (!pixels) {
        SDL_Log("Nao consegui abrir a imagem '%s': %s", caminho.c_str(), stbi_failure_reason());
        return 0;
    }

    GLuint textura;
    glGenTextures(1, &textura);
    glBindTexture(GL_TEXTURE_2D, textura);

    // GL_NEAREST e o detalhe mais importante aqui. O padrao (GL_LINEAR) mistura
    // os pixels vizinhos e deixa a pixel art borrada quando ampliada; NEAREST
    // pega o pixel mais proximo e mantem a borda quadrada e limpa.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // CLAMP_TO_EDGE evita que a imagem "repita" e vaze uma listra na borda.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, largura, altura, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Os pixels ja estao na placa de video; a copia na RAM nao serve mais.
    stbi_image_free(pixels);

    return textura;
}


int main(int argc, char* argv[]) {

    // Liga o subsistema de video do SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Nao consegui iniciar o SDL: %s", SDL_GetError());
        return 1;
    }


    // Pede a versao MAIOR do OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);

    // Pede a versao MENOR do OpenGL
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    // Pede o perfil "core": o OpenGL moderno, sem (funcoes antigas)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);


    SDL_Window* window = SDL_CreateWindow(
        // O titulo que aparece na barra de cima da janela
        "Lynx Engine",
        // Posicao X e Y: CENTERED = centraliza na tela
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        // Largura e altura da janela, em pixels
        LARGURA, ALTURA,
        // Flags: SHOWN = mostrar a janela; OPENGL = ela vai usar OpenGL
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
    );

    if (!window) {
        SDL_Log("Nao consegui criar a janela: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Cria o contexto OpenGL ligado a essa janela
    SDL_GLContext context = SDL_GL_CreateContext(window);
    // Carrega as funcoes do OpenGL
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        SDL_Log("Nao consegui carregar as funcoes do OpenGL");
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Liga o vsync: o programa espera o monitor pra trocar de frame.
    // Sem isso o loop roda milhares de vezes por segundo e esquenta a CPU atoa.
    SDL_GL_SetSwapInterval(1);

    // Diz ao OpenGL qual pedaco da janela ele pode pintar (a janela inteira).
    glViewport(0, 0, LARGURA, ALTURA);

    // Liga a transparencia. Sem isso o fundo transparente do PNG viraria uma
    // caixa preta em volta do sapo.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // -----------------------------------------------------------------------
    // MONTAGEM DO PROGRAMA DE SHADER
    // -----------------------------------------------------------------------
    // Compilamos os dois shaders e "linkamos" os dois num programa unico,
    // que e o que a placa de video de fato executa.
    GLuint vertexShader   = compilarShader(GL_VERTEX_SHADER,   CODIGO_VERTEX);
    GLuint fragmentShader = compilarShader(GL_FRAGMENT_SHADER, CODIGO_FRAGMENT);

    GLuint programa = glCreateProgram();
    glAttachShader(programa, vertexShader);
    glAttachShader(programa, fragmentShader);
    glLinkProgram(programa);

    GLint linkou = 0;
    glGetProgramiv(programa, GL_LINK_STATUS, &linkou);
    if (!linkou) {
        char log[512];
        glGetProgramInfoLog(programa, sizeof(log), nullptr, log);
        SDL_Log("Erro ao linkar o programa: %s", log);
    }

    // Depois de linkados, os shaders soltos nao servem mais pra nada.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    // -----------------------------------------------------------------------
    // A GEOMETRIA: UM QUADRADO
    // -----------------------------------------------------------------------
    // Mandamos pra placa de video UM unico quadrado, de tamanho 1x1. Todo
    // sprite do jogo e esse mesmo quadrado, esticado e com uma imagem colada.
    //
    // A placa so sabe desenhar triangulos, entao o quadrado sao 2 triangulos:
    float vertices[] = {
        // triangulo de cima-esquerda
        0.0f, 0.0f,   // canto superior esquerdo
        1.0f, 0.0f,   // canto superior direito
        0.0f, 1.0f,   // canto inferior esquerdo
        // triangulo de baixo-direita
        1.0f, 0.0f,   // canto superior direito
        1.0f, 1.0f,   // canto inferior direito
        0.0f, 1.0f    // canto inferior esquerdo
    };

    // VBO = o buffer com os numeros crus na memoria da placa de video.
    // VAO = a "receita" que explica como ler esses numeros.
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Explica o formato: o atributo 0 sao 2 floats seguidos, sem espaco entre eles.
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    // -----------------------------------------------------------------------
    // A IMAGEM DO SAPO
    // -----------------------------------------------------------------------
    // SDL_GetBasePath devolve a pasta onde o .exe esta. Usamos ela em vez de
    // um caminho tipo "assets/Frog.png" porque esse caminho relativo dependeria
    // de onde o programa foi aberto: rodar pelo Explorer e rodar pelo terminal
    // dariam resultados diferentes. O CMake copia os assets pra essa pasta.
    char* pastaExe = SDL_GetBasePath();
    std::string caminhoSapo = std::string(pastaExe ? pastaExe : "") + "assets/Frog.png";
    SDL_free(pastaExe);

    int sapoLarguraPx = 0;
    int sapoAlturaPx  = 0;
    GLuint texturaSapo = carregarTextura(caminhoSapo, sapoLarguraPx, sapoAlturaPx);
    if (texturaSapo == 0) {
        SDL_Log("Sem a imagem do sapo nao da pra continuar.");
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Tamanho que o sapo vai ocupar na tela, ja com o zoom aplicado.
    const float SAPO_L = sapoLarguraPx * ESCALA;
    const float SAPO_A = sapoAlturaPx  * ESCALA;


    // Pegamos o "endereco" de cada uniform pra poder mudar seu valor depois.
    GLint locPos  = glGetUniformLocation(programa, "uPos");
    GLint locTam  = glGetUniformLocation(programa, "uTam");
    GLint locTela = glGetUniformLocation(programa, "uTela");
    GLint locTex  = glGetUniformLocation(programa, "uTextura");

    // Deixamos o programa, o VAO e a textura ligados o tempo todo: como so
    // temos um de cada, nao precisamos ficar trocando dentro do loop.
    glUseProgram(programa);
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texturaSapo);

    // Diz ao shader que a uTextura esta na "unidade 0" (a do glActiveTexture).
    glUniform1i(locTex, 0);

    // O tamanho da tela nunca muda, entao mandamos uma vez so.
    glUniform2f(locTela, (float)LARGURA, (float)ALTURA);


    // Desenha a imagem atual esticada num retangulo da tela.
    // Isso e, na pratica, a primeira funcao do renderer da engine: um dia ela
    // vira um metodo de uma classe Renderer, mas por enquanto e uma lambda que
    // "captura" (o [&]) as variaveis declaradas acima.
    auto desenharSprite = [&](float x, float y, float largura, float altura) {
        glUniform2f(locPos, x, y);
        glUniform2f(locTam, largura, altura);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    };


    // -----------------------------------------------------------------------
    // ESTADO DO JOGO
    // -----------------------------------------------------------------------
    // Posicao do sapo: o canto superior esquerdo do sprite, em pixels.
    // Comeca no meio da tela.
    float jogadorX = (LARGURA - SAPO_L) / 2.0f;
    float jogadorY = (ALTURA  - SAPO_A) / 2.0f;

    // Controla se o loop continua
    bool running = true;
    // Variavel onde o SDL vai colocar cada evento
    SDL_Event event;

    // Guarda o instante do frame ANTERIOR, pra calcularmos quanto tempo passou.
    Uint32 tempoAnterior = SDL_GetTicks();

    // Loop principal: roda repetidamente ate 'running' virar false
    while (running) {
        // DELTA TIME: quantos segundos passaram desde o frame passado.
        // Multiplicar a velocidade por ele faz o sapo andar na mesma
        // velocidade em qualquer PC, rodando a 30 ou a 240 FPS.
        Uint32 agora = SDL_GetTicks();
        float dt = (agora - tempoAnterior) / 1000.0f;
        tempoAnterior = agora;

        // Pega os eventos um por um da fila
        while (SDL_PollEvent(&event)) {
            // Evento de fechar a janela (clicar no X): encerra o loop
            if (event.type == SDL_QUIT) running = false;
            // Se apertou uma tecla e a tecla foi ESC: tambem encerra
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        // -------------------------------------------------------------------
        // ENTRADA (WASD)
        // -------------------------------------------------------------------
        // Aqui NAO usamos eventos de tecla. Evento so avisa no instante em que
        // a tecla e apertada; o que queremos saber e se ela esta SEGURADA
        // agora, neste frame. Pra isso existe o SDL_GetKeyboardState.
        const Uint8* teclas = SDL_GetKeyboardState(nullptr);

        // Montamos uma direcao: -1, 0 ou +1 em cada eixo.
        float dirX = 0.0f;
        float dirY = 0.0f;
        if (teclas[SDL_SCANCODE_W]) dirY -= 1.0f;  // pra cima  (Y diminui)
        if (teclas[SDL_SCANCODE_S]) dirY += 1.0f;  // pra baixo (Y aumenta)
        if (teclas[SDL_SCANCODE_A]) dirX -= 1.0f;  // pra esquerda
        if (teclas[SDL_SCANCODE_D]) dirX += 1.0f;  // pra direita

        // Na diagonal (W+D, por exemplo) os dois eixos somam e o sapo andaria
        // ~41% mais rapido. Normalizar = encolher a direcao pra ela ter sempre
        // "tamanho 1", deixando a velocidade igual em qualquer direcao.
        if (dirX != 0.0f && dirY != 0.0f) {
            float tamanho = std::sqrt(dirX * dirX + dirY * dirY);
            dirX /= tamanho;
            dirY /= tamanho;
        }

        jogadorX += dirX * VELOCIDADE * dt;
        jogadorY += dirY * VELOCIDADE * dt;

        // Impede o sapo de sair da tela.
        jogadorX = std::clamp(jogadorX, 0.0f, LARGURA - SAPO_L);
        jogadorY = std::clamp(jogadorY, 0.0f, ALTURA  - SAPO_A);


        // -------------------------------------------------------------------
        // DESENHO
        // -------------------------------------------------------------------
        // Fundo verde (cor de grama).
        glClearColor(0.20f, 0.55f, 0.25f, 1.0f);

        // Pinta a tela inteira com a cor definida acima
        glClear(GL_COLOR_BUFFER_BIT);

        // O sapo.
        desenharSprite(jogadorX, jogadorY, SAPO_L, SAPO_A);

        // Mostra na tela o que foi desenhado neste frame
        SDL_GL_SwapWindow(window);
    }

    // Devolve pra placa de video a memoria que pedimos
    glDeleteTextures(1, &texturaSapo);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(programa);

    // Destroi o contexto OpenGL
    SDL_GL_DeleteContext(context);
    // Destroi a janela
    SDL_DestroyWindow(window);
    // Desliga o SDL, liberando tudo que ele alocou
    SDL_Quit();


    return 0;

}
