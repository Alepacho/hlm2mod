#import "gui.h"

#include <iostream>
#include <string>
#include <vector>
#include <mach-o/getsect.h>
#include <mach-o/dyld.h>

#import <Cocoa/Cocoa.h>                     // NSOpenGLView здесь
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>

#import "AutoHook.h"

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_osx.h"

#include "imgui_memory_editor.h"

extern unsigned long object_number;
extern unsigned long list_inst_number;
extern bool gl_check;
extern unsigned long gl_draw_calls;

NSOpenGLView *gl_view = nil;
static bool gui_inited = false;
bool show_logger = false;
bool show_dump = false;
static bool show_demo_window = false;
std::vector<std::string> logger_list;
int logger_list_max = 128;
gml::GMLObject *gmlObject_ptr = NULL;


// https://stackoverflow.com/questions/10301542/getting-process-base-address-in-mac-osx
uint64_t StaticBaseAddress(void)
{
    const struct segment_command_64* command = getsegbyname("__TEXT");
    uint64_t addr = command->vmaddr;
    return addr;
}

intptr_t ImageSlide(void)
{
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0) return -1;
    for (uint32_t i = 0; i < _dyld_image_count(); i++)
    {
        if (strcmp(_dyld_get_image_name(i), path) == 0)
            return _dyld_get_image_vmaddr_slide(i);
    }
    return 0;
}

uint64_t DynamicBaseAddress(void)
{
    return StaticBaseAddress() + ImageSlide();
}

// void *mem_data = (void*)0xdeadbeef;
// ! размер объекта – 0xB0 (176) | 0xA8 (168)
// size_t mem_size = 0xB0;//1024;//1024;
// ! размер instance - 0x1300
// AutoHook

@interface NSOpenGLViewHook: NSOpenGLView <AutoHook>
// - (void)keyDown: (NSEvent *)theEvent;
// - (BOOL)acceptsFirstResponder;
@end
@implementation NSOpenGLViewHook
+ (NSArray *)targetClasses {
    return @[@"NSOpenGLView"];
}

- (void)hook_prepareOpenGL
{
    NSLog(@"NSOpenGLViewHook Hook: %@", self);
    [self original_prepareOpenGL];
    gl_view = self;
}
- (void)hook_reshape
{
    [self original_reshape];
}
/*
// ! если объявить, то персонаж не сможет двигаться
- (void)keyDown: (NSEvent *)theEvent
{
    NSLog(@"Key Down Hook");
}
- (BOOL)acceptsFirstResponder
{
    return YES;
};
*/
// - (void)hook_keyDown: (NSEvent *)theEvent
// {
//     NSLog(@"Key Down Hook");
//     // [self original_keyDown];
// }

// MARK: - Placeholders
- (void)original_prepareOpenGL { }
- (void)original_reshape { }
// - (void)original_keyDown: (NSEvent *)theEvent { }
@end

void gui_start() {
    if (gui_inited == false) {
        printf("objc gui start\n");
        if (gl_view != nil) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            ImGui::StyleColorsDark();

            NSLog(@"ImGui baby");
            NSLog(@"Is of type NSOpenGLView: %s", [gl_view isKindOfClass:[NSOpenGLView class]] ? "true" : "false");
            // NSView *sex = (NSView*)0xdeadbeef;
            ImGui_ImplOSX_Init(gl_view); // gl_view
            NSLog(@"Impl OSX Init success");
            // const char* glsl_version = "#version 150";
            ImGui_ImplOpenGL3_Init();
            NSLog(@"Impl OpenGL3 Init success");

            // config
            // io.MouseDrawCursor = true;

            gui_inited = true;
        } else NSLog(@"no ImGui :(");
    }
}

// NSOpenGLView *view_ptr
void gui_init() {
    printf("objc gui init\n");
    printf("dynamic base address (%0llx) = static base address (%0llx) + image slide (%0lx)\n", DynamicBaseAddress(), StaticBaseAddress(), ImageSlide());
    // gl_view = view;
}

void gui_update(void *data_ptr, size_t mem_size) {
    if (!gui_inited) { gui_start(); return; };
    [NSCursor unhide];

    //
    if (logger_list.size() > logger_list_max) {
        logger_list.pop_back();
    }

    // NSLog(@"4");
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    static MemoryEditor mem_edit;
    // io.DisplayFramebufferScale = { 1, 1 };
    //io.FramebufferScale = { 1, 1 };

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplOSX_NewFrame(gl_view);
    ImGui::NewFrame();

    //ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
    if (show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);

    {
        ImGui::Begin("DEBUG MENU");
        ImGui::Text("APP AVG %.3f ms/frame (%.1f FPS)", 1000.0f
            / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        if (ImGui::Button("CONSOLE LOG")) show_logger = true; ImGui::SameLine();
        if (ImGui::Button("MEMORY DUMP")) show_dump = true;


        ImGui::Text("object_number: %lu", object_number);
        ImGui::Text("list_inst_number: %lu", list_inst_number);
        ImGui::Checkbox("gl_check", &gl_check);// ImGui::Text("gl_check: %i", gl_check);
        ImGui::Text("gl_draw_calls: %lu", gl_draw_calls);

        // data_ptr != NULL
        if (false) {
            void *func1 = (void*)*((uint64_t*)data_ptr + 3); ImGui::Text("func1: %s", (char*)func1);
            void *func2 = (void*)*((uint64_t*)data_ptr + 4); ImGui::Text("func2: %s", (char*)func2);
            void *func3 = (void*)*((uint64_t*)data_ptr + 5); ImGui::Text("func3: %s", (char*)func3);    // ! показывал указатель на название спрайта
            void *func4 = (void*)*((uint64_t*)data_ptr + 6); ImGui::Text("func4: %s", (char*)func4);
            void *func5 = (void*)*((uint64_t*)data_ptr + 8); ImGui::Text("func5: %s", (char*)func5);
        }

        if (false) { // data_ptr != NULL
            gmlObject_ptr = (gml::GMLObject*)data_ptr;

            ImGui::Text("costructor: %s",   (gmlObject_ptr->costructor  == nullptr) ? "NULL" : (char*)gmlObject_ptr->costructor);
            ImGui::Text("unknown1: %s",     (gmlObject_ptr->unknown1    == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown1);
            ImGui::Text("unknown2: %s",     (gmlObject_ptr->unknown2    == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown2);
            ImGui::Text("unknown3: %s",     (gmlObject_ptr->unknown3    == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown3);
            ImGui::Text("func1: %s",        (gmlObject_ptr->func1       == nullptr) ? "NULL" : (char*)gmlObject_ptr->func1);
            ImGui::Text("func2: %s",        (gmlObject_ptr->func2       == nullptr) ? "NULL" : (char*)gmlObject_ptr->func2);
            // ImGui::Text("unknown4: %s",     (gmlObject_ptr->unknown4    == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown4); // ! pointer
            ImGui::Text("func3: %s",        (gmlObject_ptr->func3       == nullptr) ? "NULL" : (char*)gmlObject_ptr->func3);
            // ImGui::Text("var1: %u",         gmlObject_ptr->var1);
            // ImGui::Text("var2: %u",         gmlObject_ptr->var2);
            ImGui::Text("func4: %s",        (gmlObject_ptr->func4       == nullptr) ? "NULL" : (char*)gmlObject_ptr->func4);
            // ImGui::Text("var3: %u",         gmlObject_ptr->var3);
            // ImGui::Text("var4: %u",         gmlObject_ptr->var4);
            ImGui::Text("unknown5: %s",     (gmlObject_ptr->unknown5    == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown5);
            ImGui::Text("func5: %s",        (gmlObject_ptr->func5       == nullptr) ? "NULL" : (char*)gmlObject_ptr->func5);
            ImGui::Text("unknown6: %s",     (gmlObject_ptr->unknown6    == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown6);
            ImGui::Text("id: %i",            gmlObject_ptr->id);
            //ImGui::Text("var5: %u",          gmlObject_ptr->var5);
            // ImGui::Text("unknown7: %s",     (gmlObject_ptr->unknown7    == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown7);
            // ImGui::Text("unknown8: %s",     (gmlObject_ptr->unknown8    == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown8);
            // ImGui::Text("unknown9: %s",     (gmlObject_ptr->unknown9    == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown9);
            // ImGui::Text("unknown10: %s",    (gmlObject_ptr->unknown10   == nullptr) ? "NULL" : (char*)gmlObject_ptr->unknown10);
            // ImGui::Text("func6: %s",        (gmlObject_ptr->func6       == nullptr) ? "NULL" : (char*)gmlObject_ptr->func6);
            // ImGui::Text("func7: %s",        (gmlObject_ptr->func7       == nullptr) ? "NULL" : (char*)gmlObject_ptr->func7);
            // ImGui::Text("func8: %s",        (gmlObject_ptr->func8 == nullptr) ? "NULL" : (char*)gmlObject_ptr->func8);
        }
        

        // ImGui::Checkbox("Demo Window", &show_demo_window);
        ImGui::End();
    }

    if (show_dump) {
        ImGui::Begin("MEMORY DUMP", &show_dump, ImGuiWindowFlags_NoScrollbar);
        // if (data_ptr != NULL) mem_edit.DrawWindow("MEMORY DUMP", data_ptr, mem_size);
        mem_edit.DrawContents(data_ptr, mem_size);
        ImGui::End();
    }

    if (show_logger) {
        ImGui::Begin("CONSOLE LOG", &show_logger);
        ImGui::BeginChild("LOG_BODY", ImVec2(0, 150.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (int i = 0; i < logger_list.size(); i++) ImGui::Text("%s", logger_list[i].c_str());
        ImGui::EndChild();
        if (ImGui::Button("CLEAR")) logger_list.clear();
        ImGui::End();
    }

    //if (inst_data != NULL) mem_edit.DrawWindow("OBJ 2 Dump", inst_data, mem_size);
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    draw_data->FramebufferScale = { 1, 1 };
    // [[gl_view openGLContext] makeCurrentContext];

    //glClearColor(0.0, 1.0, 0.5, 1.0);
    //glClear(GL_COLOR_BUFFER_BIT);
    // GLint m_viewport[4];
    // glGetIntegerv(GL_VIEWPORT, m_viewport);
    //NSLog(@"Viewport: %d, %d, %d, %d", m_viewport[0], m_viewport[1], m_viewport[2], m_viewport[3]);

    // GLsizei width  = (GLsizei)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    // GLsizei height = (GLsizei)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    // glViewport(0, 0, width, height);

    ImGui_ImplOpenGL3_RenderDrawData(draw_data);
    // NSLog(@"render (%d, %d)", width, height);

    // [[gl_view openGLContext] flushBuffer];
}


void gui_term() {
    printf("objc gui term\n");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplOSX_Shutdown();
    ImGui::DestroyContext();
}

void gui_logger_insert(const char* msg) {
    logger_list.insert(logger_list.begin(), std::string(msg));
    std::cout << msg << std::endl;
}

void gui_logger_insert(std::string msg) {
    logger_list.insert(logger_list.begin(), msg);
    std::cout << msg << std::endl;
}