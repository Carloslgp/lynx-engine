#include <SDL.h>
#include <glad/glad.h>

int main(int argc, char* argv[]) {

    // Liga o subsistema de video do SDL
    SDL_Init(SDL_INIT_VIDEO);


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
        800, 600,
        // Flags: SHOWN = mostrar a janela; OPENGL = ela vai usar OpenGL
        SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL
    );

    // Cria o contexto OpenGL ligado a essa janela
    SDL_GLContext context = SDL_GL_CreateContext(window);
    // Carrega as funcoes do OpenGL
    gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);

    // Controla se o loop continua
    bool running = true;
    // Variavel onde o SDL vai colocar cada evento
    SDL_Event event;

    // Declara variável que vai contar o tempo desde que o programa abriu
    Uint32 tempo;

    // Loop principal: roda repetidamente ate 'running' virar false
    while (running) {
        // Lemos a quantidade de tempo em ms desde que o programa foia aberto
        tempo = SDL_GetTicks();

        // Pega os eventos um por um da fila
        while (SDL_PollEvent(&event)) {
            // Evento de fechar a janela (clicar no X): encerra o loop
            if (event.type == SDL_QUIT) running = false;
            // Se apertou uma tecla e a tecla foi ESC: tambem encerra
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
        }


        // Verificamos se o segundo atual  valor é par ou ímpar
        if ((tempo / 1000) % 2 == 0) {
            glClearColor(0.8f, 0.2f, 0.2f, 1.0f);  // vermelho
        } else {
            glClearColor(0.2f, 0.2f, 0.8f, 1.0f);  // azul
        }

        // Pinta a tela inteira com a cor definida acima
        glClear(GL_COLOR_BUFFER_BIT);

        // Mostra na tela o que foi desenhado neste frame
        SDL_GL_SwapWindow(window);
    }

    // Destroi o contexto OpenGL
    SDL_GL_DeleteContext(context);
    // Destroi a janela
    SDL_DestroyWindow(window);
    // Desliga o SDL, liberando tudo que ele alocou
    SDL_Quit();


    return 0;

}



