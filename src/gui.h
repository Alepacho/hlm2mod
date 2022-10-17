// это ваш так называемый "интерфейс"
#ifndef GUI_H
#define GUI_H

#ifdef __cplusplus
#define CFUN extern "C"
#else
#define CFUN
#include "stdbool.h"
#endif

#include "stdio.h"
#include <string>
#include "gml.h"

// #include "imgui.h"
// #include "imgui/backends/imgui_impl_sdl.h"
// #include "imgui/backends/imgui_impl_opengl3.h"
// #include "imgui/backends/imgui_impl_osx.h"

// const char *glsl_version = "#version 150";

// void *gui_obj
CFUN void gui_init();
CFUN void gui_term();
CFUN void gui_start();
CFUN void gui_update(void *obj_data, size_t mem_size);

CFUN void gui_logger_insert(const char* msg);
void gui_logger_insert(std::string msg);

// 
template <class T>
inline void hexdump(T const *ptr, size_t buflen = sizeof(T)) {
    unsigned char const *buf = reinterpret_cast<unsigned char const *>(ptr);
    int i, j;
    for (i=0; i<buflen; i+=16) {
        printf("%06x: ", i);
        for (j=0; j<16; j++) if (i+j < buflen) printf("%02x ", buf[i+j]); else printf("   ");
        printf(" ");
        for (j=0; j<16; j++) if (i+j < buflen) printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
        printf("\n");
    }
}

#endif // GUI_H