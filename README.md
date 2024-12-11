**ТУТ БУДЕТ ГИФКА**

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/withastro/astro/blob/main/LICENSE)

## Install

**Рекомендуемый способ** установить зависимости необходимые для проекта:
1. Устанавливаем пакетный менеджер pip и инструмент сборки CMake 
2. Устанавливаем пакетный менеджер для C++ conan 2
```bash
pip install conan
```
3. Переходим в директорию проекта
4. Детектим профайл conan
```bash
conan profile detect
```
5. Устанавливаем зависимости из conanfile.txt
```bash
conan install . --output-folder=build --build=missing -s compiler.cppstd=17 -g CMakeToolchain 
```
6. Переходим в папку build
```bash
cd build 
```
7. Конфигурируем проект CMake
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE="./build/build/generators/conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release 
```

## Build

Для сборки проекта используйте команду ниже:

```bash
cmake --build . --config Release
```

## Run
Для запуска серверной части используйте:

```bash
.\src\server\Release\chat_server.exe
```
Для клиентской части:

```bash
.\src\client\Release\chat_client.exe
```

## Структура проекта: boost-chat

### Верхний уровень:

- `build/`  
  Каталог для сборки.  

- `include/`  
  Содержит заголовочные файлы.
  - `common/message.hpp`  
    Заголовочный файл с реализацией сообщений.

- `src/`  
  Исходные файлы проекта.
  - `client/`  
    Реализация клиентской части.
    - `CMakeLists.txt`  
      Файл CMake для сборки клиента.
    - `main.cpp`  
      Точка входа для клиентского приложения.
  - `server/`  
    Реализация серверной части.
    - `CMakeLists.txt`  
      Файл CMake для сборки сервера.
    - `main.cpp`  
      Точка входа для серверного приложения.

- `.gitignore`  
  Файл, указывающий на файлы и директории, которые должны игнорироваться Git.

- `CMakeLists.txt`  
  Главный файл CMake для настройки сборки проекта.

- `conanfile.txt`  
  Файл для управления зависимостями с использованием Conan.
