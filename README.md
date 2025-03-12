# Hidra++: Monitor de Hidratação em C para BitDogLab

Este projeto, carinhosamente apelidado de **Hidra++** (em referência ao trocadilho envolvendo hidratação e C++), é uma aplicação desenvolvida em C que monitora a hidratação de forma inteligente, integrando hardware e software para garantir a coleta e o monitoramento dos dados.

## Estrutura do Projeto

- **Arquivos de Código-Fonte**  
  - `monitor_hidratacao_IoT.c` e `monitor_hidratacao_IoT.h`: Implementam a lógica principal para o monitoramento da hidratação.  
  - `funcoes_gerais.c` e outros arquivos auxiliares: Contribuem com funções utilitárias para o funcionamento do sistema.  

- **Interface com o Hardware**  
  - `ssd1306.c` e `ssd1306.h`: Código para controle de displays SSD1306, proporcionando uma interface visual ao usuário.

- **Configuração e Build**  
  - `CMakeLists.txt`: Responsável pela configuração do build via CMake.
  - Arquivos e pastas relacionados à build, como `build/`, que contêm artefatos gerados (binários, logs, etc.).
  - Arquivo `pico_sdk_import.cmake`: Importa a SDK necessária para o desenvolvimento com a plataforma Pico.

- **Outros Arquivos**  
  - Arquivos de configuração do Git (`.gitignore`, `.gitattributes`) e um README para orientar o uso e a compreensão do projeto.

## Recursos e Funcionalidades

- **Monitoramento em Tempo Real:**  
  A aplicação realiza a coleta e o monitoramento contínuo dos níveis de hidratação, exibindo os dados em tempo real no display.

- **Interface Amigável:**  
  Utilização de um display SSD1306 para uma interface visual simples e intuitiva, facilitando a leitura e a interpretação dos dados monitorados.

- **Configuração Flexível e Portabilidade:**  
  Uso do CMake para gerenciamento do build, permitindo uma fácil portabilidade entre diferentes plataformas.

## Como Compilar e Executar

1. **Pré-requisitos:**  
   - [CMake](https://cmake.org/)
   - [Pico SDK](https://github.com/raspberrypi/pico-sdk) (caso esteja utilizando a plataforma Pico)
