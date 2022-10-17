// rm -rf hook.dylib && make hook.dylib
/*
// TODO: разобраться как вызывать member class function, если ты находишься в хуке функции класса
// TODO: Понять как мне узнать размер класса, зная только его указатель. UPD: никак
*/

// ! После струткуры Object Instance идут переменные // его скрипты

#include <_types/_uint16_t.h>
#include <iostream>
#include <string>
#include <sstream>
// #include <cmath>
// #include <random>
#include <iomanip>

// friendship ended with mach_override
// now fishhook is my best friend 😎🤝🪝
#include "fishhook.h"
#include "objects.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
// #include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreVideo/CoreVideo.h>
// #include <Cocoa/Cocoa.h>
// #include <OpenGL/gl.h>
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include <SDL2/SDL.h>
// #include <SDL_opengl.h>

#include "gui.h"
#include "gml.h"


template< typename T >
std::string hex(T i) {
  std::stringstream stream;
  stream << "0x" 
         << std::setfill ('0') << std::setw(sizeof(T)*2) 
         << std::hex << i;
  return stream.str();
}

unsigned long malloc_skip = 0;
unsigned long malloc_skip_max1 = 1374863; // 1377863
unsigned long malloc_skip_max2 = 1377363; // 1378363

// unsigned long object_number = -1;
void *obj_data, *inst_data;

unsigned long   object_number       = 0;
unsigned long   list_inst_number    = 0;
bool            gl_check            = false;
unsigned long   gl_draw_calls       = 0;

GLuint shaderProgram;
GLuint vertexArrayObject;
GLuint vertexBuffer;

GLint positionUniform;
GLint colourAttribute;
GLint positionAttribute;

//
void *data_ptr = NULL;
size_t data_size = 0;

size_t obj_size = 0xB0;//0x78 * 4;//0xB0 * 2;
void *obj_ptr1 = NULL;
void *obj_ptr2 = NULL;

size_t inst_size = 0x1300;
void *inst_ptr[2] = {NULL};

void hookEntryPoint(void) __attribute__ ((visibility("default"))) __attribute__ ((constructor));
void hookDestructor(void) __attribute__ ((visibility("default"))) __attribute__ ((destructor));

// ******************************** macOS HOOKS ********************************
// ! malloc
static void *(*orig_malloc)(size_t);
void *hook_malloc(size_t size) {
    // if (malloc_skip > malloc_skip_max1
    // &&  malloc_skip < malloc_skip_max2) std::cout << "malloc hook: " << size << std::endl;
    // if (malloc_skip < malloc_skip_max2) malloc_skip++;
    return orig_malloc(size);
}

// !
static CGLError (*orig_CGLFlushDrawable)(CGLContextObj);
CGLError hook_CGLFlushDrawable(CGLContextObj ctx) {
    // CGLContextObj ctx = CGLGetCurrentContext();
    // std::cout << "CGLFlushDrawable hook" << ctx << std::endl;
    //if (gui.inited) gui.render();

    //gui_update();
    //gl_prikol_render();
    
    // if (!gl_prikol) gl_prikol_init(); else gl_prikol_render();
    // glFlush();
    // printf("lol\n");
    gui_update(data_ptr, data_size);
    CGLError result = orig_CGLFlushDrawable(ctx);
    
    // ! покажет зеленый экран на секунду если выйду через command+Q
    // if (!gl_prikol) gl_prikol_init(); else gl_prikol_render();
    //glFlush();

    // std::cout << "Draw Calls: " << draw_calls << std::endl; draw_calls = 0;
    // CGLError result = kCGLNoError;
    gl_draw_calls = 0;
    return result;
}

static CGDirectDisplayID (*orig_CGMainDisplayID)();
CGDirectDisplayID hook_CGMainDisplayID() {
    // CGLContextObj ctx = CGLGetCurrentContext();
    CGDirectDisplayID id = orig_CGMainDisplayID();
    // std::cout << "CGMainDisplayID hook: " << id << std::endl;
    return id;
}

// ! disable mouselock
static CGError (*orig_CGWarpMouseCursorPosition)(CGPoint);
CGError hook_CGWarpMouseCursorPosition(CGPoint newPos) {
    // std::cout << "CGWarpMouseCursorPosition hook: " << newPos.x << ", " << newPos.y << std::endl;
    // CGError result = orig_CGWarpMouseCursorPosition(newPos);
    CGError result = kCGErrorSuccess;
    return result;
}

// ! removes mouse hide
static CGError (*orig_CGDisplayHideCursor)(CGDirectDisplayID);
CGError hook_CGDisplayHideCursor(CGDirectDisplayID display) {
    // std::cout << "CGDisplayHideCursor hook: " << display << std::endl;
    CGError result = kCGErrorSuccess;
    CGDisplayShowCursor(display);
    return result;//orig_CGDisplayHideCursor(display);
}

// ! вроде как угол камеры меняет, но по факту разницы не заметил
// ? void gml::GMLObjectInstance::SetDirection(double)
// ? _ZN3gml17GMLObjectInstance12SetDirectionEd
static void *(*orig_ZN3gml17GMLObjectInstance12SetDirectionEd)(void*, int, int, int, int, int, int, int, double);
void *hook_ZN3gml17GMLObjectInstance12SetDirectionEd(void* object, 
    int idk1, int idk2, int idk3, int idk4, int idk5, int idk6, int idk7, double direction) {
    // std::ostream hexcout(std::cout.rdbuf());
    // std::cout << "gml::GMLObjectInstance::SetDirection:";
    // hexcout << object << ", ";
    // std::cout << idk1 << ", " << idk2 << ", " << idk3 << ", " << idk4 << ", ";
    // std::cout << idk5 << ", " << idk6 << ", " << idk7 << ", " << direction << ";" << std::endl;
    // hexdump(object, 512); 
    return orig_ZN3gml17GMLObjectInstance12SetDirectionEd(object, 
        idk1, idk2, idk3, idk4, idk5, idk6, idk7, 0); // direction
}

// #define	RAND_MAX	0x7fffffff
// ? gml::PGMLVar is a uint32, I guess
// ZZN3gml6randomERKNS_7PGMLVarEE15oneOverRAND_MAX
// gml::random(gml::PGMLVar const&)::oneOverRAND_MAX
// passing a reference of gml::PGMLVar
static uint32 (*orig_ZZN3gml6randomERKNS_7PGMLVarEE15oneOverRAND_MAX)(uint32 const&);
uint32 hook_ZZN3gml6randomERKNS_7PGMLVarEE15oneOverRAND_MAX(uint32 const &value) {
    std::stringstream str; str << "gml::random(" << value << ")";
    gui_logger_insert(str.str());
    return 0;//orig_CGDisplayHideCursor(display);
}

// 0001f0: 00 00 00 00 00 00 00 00 28 3a 4e 6f e5 7f 00 00  ........(:No....
// (:
// ! gml::GMLObjectInstance::GMLObjectInstance(gml::GMLObject*, int)
// ! currently doesn't work
// ! именно тут создаются нужные объекты
// ? 00000001027b2974 t __ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi           
// ? gml::GMLObjectInstance::GMLObjectInstance(gml::GMLObject*, int)
// gml::GMLObjectInstance *
// TODO: понять как вызвать конструктор внутри хука
// * понял! прикол был в первой функции!!! это указатель на ObjectInstance!!!
static void (*orig_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi)(void*, void*, int);
void hook_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi(void *instance, void *object, int idk) {
    // printf("NULL: %ld\n", (void*)NULL);
    object_number++;
    uint32_t object_id = (uint32_t)*((uint32_t*)object + 0x70 / 2 / 2);
    const objectTable *obj = getObjectByID(object_id);
    if (object_id != 48 && object_id != 46) {
        std::string object_name = (obj != nullptr) ? obj->name : "unknown";

        // data_ptr = object; data_size = inst_size;
        std::stringstream str;
        str << "GMLObjectInstance[" << hex(instance) << "]: " << hex(object);
        str << " " << object_name << "(" << object_id << ")" << ", " << idk << ";";
        gui_logger_insert(str.str());
    }
    // std::cout << "GMLObject: " << object_number << std::endl;
    // std::cout << "GMLObject size: " << sizeof(gml::GMLObject) << std::endl;
    // std::cout << "GMLObject[";
    // void *po = ((char*)object + 0);
    // hexcout << po;
    // std::cout << "]: " << (char *)po << std::endl;
    
    
    /*
    if (obj_ptr1 == NULL) {
        obj_ptr1 = malloc(0x6c);
        orig_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi(instance, obj_ptr1, idk);
        std::cout << "it works!\n";
        // ! скипает заставки, а также убирает фон в меню 
    } else {
        if (object_number == 1) {
            // по идее должно убрать меню
            obj_ptr2 = malloc(0x6c);
            orig_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi(instance, obj_ptr2, idk);
            std::cout << "it works too!\n";
        } else {
            // dump(object, 16);
            hexdump(object, 0x6c); 
            // dumpobject(&object, 0x000120);
            // hexdump(object, 256); 
            //gml::GMLObjectInstance *result =  
            orig_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi(instance, object, idk);
            std::cout << "++++++++++++++++++++++++++++++++++++++++++++++++ result\n";
        }
        //return result;
    }
    */

    // std::cout << "instance\n";
    // hexdump(instance, 1024); 

    // // ! иногда вылетает 11, помогает перекомпиляция
    // std::cout << "Instance check\n";
    size_t msize = 1024;//0xE0;//0x70;//0x6e;
    if (object_id == 0x00000000) { // objTitleScreen
        gui_logger_insert("found objTitleScreen");
        // *((uint32_t*)object + 0x70 / 2 / 2) = (uint32_t)38;
        // data_ptr = instance; data_size = inst_size;
        // data_ptr = object; data_size = obj_size;
    } else if (object_id == 0x00000023) { // если это свинота ебаная
        // 0x0580
        // obj_ptr2 = malloc(msize);
        //memcpy(obj_ptr2, object, msize);
        
        //obj_data = obj_ptr2;
        //inst_ptr[1] = instance;
    } else if (object_id == 0x000008FD) {   // objDevolverAbstraction
        // *((uint32_t*)object + 0x70 / 2 / 2) = (uint32_t)6;
        // gui_logger_insert(std::to_string(gmlObject->id));
        // data_ptr = instance; data_size = inst_size;
        // data_ptr = object; data_size = obj_size;
    } else if (object_id == 0x000000B1) {   // objShotgun
        //*((uint64_t*)instance + 0x0340 / 2 / 2 / 2) = (double)420;
        // data_ptr = instance; data_size = inst_size;
    } else if (object_id == 0x000003CE) {   // objNewBGNormal
        // *((uint32_t*)object + 0x70 / 2 / 2) = (uint32_t)38;
    } else {
        /*
        if (object_number == 0) {
            // 2301 objDevolverAbstraction  (0x0182)
            // instance_id  34505       (0x86c9)
            // object_id    2301        (0x08fd)
            // 0x80 x2c
            
            obj_ptr1 = malloc(msize);//calloc(msize, sizeof(char *)); // 0x6c... походу таки 110 (0x6e)
            memcpy(obj_ptr1, object, msize);
            // 000070: fd 08 00 00
            // 0x0431 - objPause
            // 234          maskMenu
            // 35   0x0023  objPigButcher       // amogus 
            // 173          knife
            // 662          jungle bkg   (работает лишь пару секунд)
            // 291          demo restart
            // 74   0x004A  objFridge
            // 1598         objOceanDay         // ! работает с артефактами
            // 1596         objOcean            // ! также
            // 1642         objNightmareLight
            // 1282         objSewerDrip
            // 1186         objLSDBlend
            // 292          objGameOverBackground
            // 38           objEffector
            *((uint16_t*)object + 0x38) = (uint16_t)6;
            obj_data = obj_ptr1;
            inst_ptr[0] = instance;
            data_ptr = instance; data_size = inst_size;

            // printf("%02x\n", *((uint16_t*)object + 16));
            //memcpy(obj_ptr1, object, msize);
            //obj_ptr1 = (char*)74;
            // если ввести 0x4, то будет лишь холодильник на фоне 
            // 74	/Furniture/objFridge	149	1	39	-1	True	True	False
            // 0x6 – раковина
        } else if (object_number == 1) {
            // obj_ptr2 = malloc(msize);
            // memcpy(obj_ptr2, object, msize);
            // // *((uint8_t*)object + 0x70) = (uint16_t)0x0070;
            // obj_data = obj_ptr2;
            // inst_ptr[1] = instance;
            // data_ptr = instance; data_size = inst_size;
            // inst_data = obj_ptr2;
        }
        */
    }

    // надо найти 240 (0xF0) и 112 (0x70)
    orig_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi(instance, object, idk);
    // if (object_id == 0x000000B1) {   // objShotgun
    //     //*((uint64_t*)instance + 0x0340 / 2 / 2 / 2) = (double)420.0;
    //     //std::cout << "found objShotgun\n";
    //     
    //     //data_ptr = instance; data_size = inst_size;
    // }
}

// ! gml::GMLResources<gml::GMLObjectInstance>::destroyInstance()
// ! вызывается только при выходе, юзлес хуйня
static void *(*orig_ZN3gml12GMLResourcesINS_17GMLObjectInstanceEE15destroyInstanceEv)(void*);
void *hook_ZN3gml12GMLResourcesINS_17GMLObjectInstanceEE15destroyInstanceEv(void* idk) {
    std::stringstream str;
    str << "gml::GMLResources<gml::GMLObjectInstance>::destroyInstance(" << idk << ")";
    gui_logger_insert(str.str());
    return orig_ZN3gml12GMLResourcesINS_17GMLObjectInstanceEE15destroyInstanceEv(idk);
}

// ! gml::GMLResources<gml::GMLRoom>::destroyInstance()
// ! вызывается только при выходе, юзлес хуйня 2
static void *(*orig_ZN3gml12GMLResourcesINS_7GMLRoomEE15destroyInstanceEv)(void*);
void *hook_ZN3gml12GMLResourcesINS_7GMLRoomEE15destroyInstanceEv(void* idk) {
    std::stringstream str;
    str << "gml::GMLResources<gml::GMLRoom>::destroyInstance(" << idk << ")";
    gui_logger_insert(str.str());
    return orig_ZN3gml12GMLResourcesINS_7GMLRoomEE15destroyInstanceEv(idk);
}

static void *(*orig_ZNSt3__113__vector_baseIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEED2Ev)(void*, void *);
void *hook_ZNSt3__113__vector_baseIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEED2Ev(void* self, void* ptr) {
    std::stringstream str;
    str << "__vector_base<gml::GMLObjectInstance*, allocator<gml::GMLObjectInstance*> >::~__vector_base(" << ptr << ")";
    gui_logger_insert(str.str());
    return orig_ZNSt3__113__vector_baseIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEED2Ev(self, ptr);
}

// ! vector<gml::GMLObjectInstance*, allocator<gml::GMLObjectInstance*> >::allocate(unsigned long)
// ? без понятия пока где эта функция выполняется
static void *(*orig_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE8allocateEm)(void*, unsigned long);
void *hook_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE8allocateEm(void* self, unsigned long size) {
    std::stringstream str;
    str << "vector<gml::GMLObjectInstance*, allocator<gml::GMLObjectInstance*> >::allocate(";
    str << hex(self) << ", " << size << ")";
    gui_logger_insert(str.str());
    return orig_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE8allocateEm(self, size);
}

// ! vector<gml::GMLObject*, allocator<gml::GMLObject*> >::__append(unsigned long)
// // ? без понятия пока где эта функция выполняется
// выполняется паралельно с List
// 1+3+1+1+1067+265+248+252+1+2+18+3+9+6+7+405+21+104 = 2414 (примерно)
static void *(*orig_ZNSt3__16vectorIPN3gml9GMLObjectENS_9allocatorIS3_EEE8__appendEm)(void*, unsigned long);
void *hook_ZNSt3__16vectorIPN3gml9GMLObjectENS_9allocatorIS3_EEE8__appendEm(void* self, unsigned long size) {
    std::stringstream str;
    str << "vector<gml::GMLObject*, allocator<gml::GMLObject*> >::__append(";
    str << hex(self) << ", " << size << ")";
    gui_logger_insert(str.str());
    return orig_ZNSt3__16vectorIPN3gml9GMLObjectENS_9allocatorIS3_EEE8__appendEm(self, size);
}
// orig_ZNSt3__16vectorIPN3gml9GMLObjectENS_9allocatorIS3_EEE8__appendEm

// ! void vector<gml::GMLObjectInstance*, allocator<gml::GMLObjectInstance*> >::__push_back_slow_path<gml::GMLObjectInstance* const&>(gml::GMLObjectInstance* const&)
// * тоже самое что и ObjectInstance конструктор, но без Object
int vector_inst_count = 0;
static void *(*orig_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE21__push_back_slow_pathIRKS3_EEvOT_)(void*, void*); // const &
void *hook_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE21__push_back_slow_pathIRKS3_EEvOT_(void *self, void* instance) {
    std::stringstream str;
    str << "void vector<gml::GMLObjectInstance*, allocator<gml::GMLObjectInstance*> >::__push_back_slow_path<gml::GMLObjectInstance* const&>(";
    str << hex(self) << ", " << instance << ")";
    gui_logger_insert(str.str());

    void* result = orig_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE21__push_back_slow_pathIRKS3_EEvOT_(self, instance);
    // ! хз че там короче
    if (vector_inst_count == 0) {
        // !  лучше указать размер класса, иначе будет bus error 10
        // data_ptr = instance; data_size = 0x1300; // 0x1300; // inst_size; 
    } // else data_size += 0x1300;
    
    vector_inst_count++;
    return result;
}

// ! используется только при выходе из игры
// ? ZN22GameBakerLoadingScreenD2Ev
// ? GameBakerLoadingScreen::~GameBakerLoadingScreen()
static void (*orig_ZN22GameBakerLoadingScreenD2Ev)(void*);
void hook_ZN22GameBakerLoadingScreenD2Ev(void *self) {
    std::stringstream str;
    str << "GameBakerLoadingScreen::~GameBakerLoadingScreen(" << hex(self) << ")";
    gui_logger_insert(str.str());
    orig_ZN22GameBakerLoadingScreenD2Ev(self);
}

// ? List<gml::GMLObjectInstance*>::List()
// вызывается 1999 раз в начале (1941 в версии без редактора)
// ? мб размер 0x50 (80)
static void *(*orig_ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev)(void*);
void *hook_ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev(void* ptr) {
    // unsigned long find_inst = 1894;
    // 1894 - objDevolverAbstraction
    //if (list_inst_number == find_inst) {
        // data_ptr = ptr; data_size = 0x50;
        // printf("before\n");
        // hexdump(ptr, 0x50); 
        // void *func1 = (void*)*((uint64_t*)data_ptr + 3); printf("func1: %s", (char*)func1);
        // void *func2 = (void*)*((uint64_t*)data_ptr + 4); printf("func2: %s", (char*)func2);
        // void *func3 = (void*)*((uint64_t*)data_ptr + 5); printf("func3: %s", (char*)func3);
        // void *func4 = (void*)*((uint64_t*)data_ptr + 6); printf("func4: %s", (char*)func4);
        // void *func5 = (void*)*((uint64_t*)data_ptr + 8); printf("func5: %s", (char*)func5);
    //}
    // std::cout << "List<gml::GMLObjectInstance*>::List() " << list_counter << std::endl; 
    // hexdump(&orig_ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev, 256); 
    //orig_ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev(ptr);
    void *result = orig_ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev(ptr);
    //if (list_inst_number == find_inst) {
        // printf("after\n");
        // hexdump(ptr, 0x50); 
        // void *func1 = (void*)*((uint64_t*)data_ptr + 3); printf("func1: %s\n", (func1 == nullptr) ? "NULL" : (char*)func1);
        // void *func2 = (void*)*((uint64_t*)data_ptr + 4); printf("func2: %s\n", (func2 == nullptr) ? "NULL" : (char*)func2);
        // void *func3 = (void*)*((uint64_t*)data_ptr + 5); printf("func3: %s\n", (func3 == nullptr) ? "NULL" : (char*)func3);
        // void *func4 = (void*)*((uint64_t*)data_ptr + 6); printf("func4: %s\n", (func4 == nullptr) ? "NULL" : (char*)func4);
        // void *func5 = (void*)*((uint64_t*)data_ptr + 8); printf("func5: %s\n", (func5 == nullptr) ? "NULL" : (char*)func5);
        
        //printf("btw\n");
        //hexdump(result, 1024);
    //}
    std::stringstream str;
    str << "List<gml::GMLObjectInstance*>::List(" << hex(ptr) << ") " << list_inst_number; //  // ! почему-то вылетает если указать аргумент
    gui_logger_insert(str.str());
    list_inst_number++;
    return result;
}

// ! тут конструктор тоже не работает (10 bus error)
// ! GameBakerMemory::mallocObjectInstance()::forceInitializeMemory
// ? наверное не объявили саму функцию (тк не могу найти ее в дизассемблере)
static void (*orig_ZZN15GameBakerMemory20mallocObjectInstanceEvE21forceInitializeMemory)(void*);
void hook_ZZN15GameBakerMemory20mallocObjectInstanceEvE21forceInitializeMemory(void *self) {
    std::stringstream str;
    str << "GameBakerMemory::mallocObjectInstance(" << hex(self) << ")::forceInitializeMemory";
    gui_logger_insert(str.str());
    orig_ZZN15GameBakerMemory20mallocObjectInstanceEvE21forceInitializeMemory(self);
}

// ******************************** OpenGL HOOKS ********************************
static void (*orig_glViewport)(GLint, GLint, GLsizei, GLsizei);
void hook_glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    // std::cout << "glViewport hook: " << x << ", " << y << ", " << w << ", " << h << std::endl;
    return orig_glViewport(x, y, w, h);
}

static void (*orig_glShaderSource)(GLint, GLsizei, const GLchar **, const GLint *);
void hook_glShaderSource(GLuint shader, GLsizei count, const GLchar **string, const GLint *length) {
    // std::cout << "glShaderSource hook: " << *string << std::endl;
    return orig_glShaderSource(shader, count, string, length);
}

// ! will enable 34037 -> 0x84F5 -> GL_TEXTURE_RECTANGLE, nothing special here
// static void (*orig_glEnable)(GLenum);
// void hook_glEnable(GLenum cap) {
//     std::cout << "glEnable hook: " << cap << std::endl;
//     orig_glEnable(cap);
// }

// ! аттрибутов обычно 3: in_Position, in_UV и in_Color.
// ! но они могут иногда вырубаться (под конец рендера)
static void (*orig_glEnableVertexAttribArray)(GLuint);
void hook_glEnableVertexAttribArray(GLuint index) {
    // if (check) std::cout << "glEnableVertexAttribArray hook: " << index << std::endl;
    orig_glEnableVertexAttribArray(index);
}

static void (*orig_glDisableVertexAttribArray)(GLuint);
void hook_glDisableVertexAttribArray(GLuint index) {
    // if (check) std::cout << "glDisableVertexAttribArray hook: " << index << std::endl;
    orig_glDisableVertexAttribArray(index);
}

// ! в загрузке:
// * mode обычно 4 (GL_TRIANGLES), но может и 7 (GL_QUADS)
// * first всегда 0
// * count равен 6 (2 треугольника), либо 4 (квадрат)
// ? вероятнее всего квадрат рисуется для вывода application_surface
// ? то есть игра рендерит все в отдельную текстуру, а потом уже на экран
// ! когда показывают аутсорсеров:
// * mode: GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN
// * first 0, 1372, 1112, 1588, 1592, 1600, ... 1640
// ! число каждый раз увеличивается
// * count 4, 6, 92
// ? GL_TRIANGLE_FAN нужен для отрисовки круга (красноватого эффекта по бокам)
// ! если убрать ориг функцию то ничего не будет отрисовываться
// (потому что финальная текстура отрисовывается как 2 треугольника)
static void (*orig_glDrawArrays)(GLenum, GLint, GLsizei);
void hook_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    if (gl_check) {
        std::stringstream str;
        str << "glDrawArrays(" << mode << ", " << first << ", " << count << ")";
        gui_logger_insert(str.str());
    }
    orig_glDrawArrays(mode, first, count);
    gl_draw_calls++;
}

// ! все в основном рисуется через glDrawElements
// ! кроме примитивов (круги линии и тд)
// ! и эффектов
static void (*orig_glDrawElements)(GLenum, GLsizei, GLenum, const GLvoid *);
void hook_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    if (gl_check) {
        std::cout << "glDrawElements hook: " 
            << mode  << ", " 
            << count << ", " 
            << type  << ", " 
            << indices 
            << std::endl;
    }
    orig_glDrawElements(mode, count, type, indices);
    gl_draw_calls++;
}


// ******************************** Test ********************************

static void (*orig_test)();
void hook_test() {
    std::cout << "test hook" << std::endl;
    orig_test();
}

// ******************************** SDL2 HOOKS ********************************
// ! will enable SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER
static int (*orig_SDL_Init)(Uint32);
int hook_SDL_Init(Uint32 flags) {
    std::cout << "SDL_Init hook: " << flags << std::endl;

    //if (SDL_Init(SDL_INIT_VIDEO)) std::cout << SDL_GetError() << std::endl;
    //gui.init();
    // std::cout << "orig_test " << orig_test << std::endl;
    // gui_start();
    return orig_SDL_Init(SDL_INIT_VIDEO | flags);
}

static const char* (*orig_SDL_GetKeyName)(SDL_Keycode);
const char* hook_SDL_GetKeyName(SDL_Keycode key) {
    const char *result = orig_SDL_GetKeyName(key);
    std::cout << "SDL_GetKeyName(" << key << "): " << result << std::endl;
    return result;
}

// ******************************** Hook Entry ********************************
void hookEntryPoint() {
    std::cout << "[GameBaker Modding] Start" << std::endl;

    // 
    // rebind macOS stuff
    // std::cout << "orig_test " << orig_test << std::endl;
    rebinding mac_rebindings[] = {
        { "CGLFlushDrawable", (void *)hook_CGLFlushDrawable, (void **)&orig_CGLFlushDrawable },
        // { "CGMainDisplayID", (void*)hook_CGMainDisplayID, (void **)&orig_CGMainDisplayID },
        { "CGWarpMouseCursorPosition", (void *)hook_CGWarpMouseCursorPosition, 
            (void **)&orig_CGWarpMouseCursorPosition },
        { "CGDisplayHideCursor", (void *)hook_CGDisplayHideCursor, 
            (void **)&orig_CGDisplayHideCursor },
        // { "_ZN3gml17GMLObjectInstance12SetDirectionEd",                                          // ! все еще не меняет угол как надо
        //     (void *)hook_ZN3gml17GMLObjectInstance12SetDirectionEd,
        //     (void **)&orig_ZN3gml17GMLObjectInstance12SetDirectionEd },
        // { "_ZZN3gml6randomERKNS_7PGMLVarEE15oneOverRAND_MAX",
        //     (void *)hook_ZZN3gml6randomERKNS_7PGMLVarEE15oneOverRAND_MAX,
        //     (void **)&orig_ZZN3gml6randomERKNS_7PGMLVarEE15oneOverRAND_MAX },
        // { "_ZN22GameBakerLoadingScreenD2Ev", (void *)hook_ZN22GameBakerLoadingScreenD2Ev,
        //     (void **)&orig_ZN22GameBakerLoadingScreenD2Ev },
        { "_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi",
            (void *)hook_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi,
            (void **)&orig_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi },
        // { "_ZN3gml12GMLResourcesINS_17GMLObjectInstanceEE15destroyInstanceEv",
        //     (void *)hook_ZN3gml12GMLResourcesINS_17GMLObjectInstanceEE15destroyInstanceEv,       // ! вызывается только при выходе
        //     (void **)&orig_ZN3gml12GMLResourcesINS_17GMLObjectInstanceEE15destroyInstanceEv },   // !
        // { "malloc", (void *)hook_malloc, (void **)&orig_malloc },
        // { "_ZZN15GameBakerMemory20mallocObjectInstanceEvE21forceInitializeMemory",               // ! doesn't work (да и скорее всего в начале только вызывается)
        // (void *)hook_ZZN15GameBakerMemory20mallocObjectInstanceEvE21forceInitializeMemory,       // !
        // (void**)&orig_ZZN15GameBakerMemory20mallocObjectInstanceEvE21forceInitializeMemory },    // !
        // { "", (void *)hook_, (void **)&orig_ }
        { "_ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev",
            (void *)hook_ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev,
            (void **)&orig_ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev },
        // { "", (void *)hook_, (void **)&orig_ }
        // { "NSOpenGLView", (void *)hook_test, (void **)&orig_test }
        // { "_ZN3gml12GMLResourcesINS_7GMLRoomEE15destroyInstanceEv",                              // ! вызывается только при выходе
        // (void *)hook_ZN3gml12GMLResourcesINS_7GMLRoomEE15destroyInstanceEv,                      // !
        // (void **)&orig_ZN3gml12GMLResourcesINS_7GMLRoomEE15destroyInstanceEv },                  // !
        { "_ZNSt3__113__vector_baseIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEED2Ev",
        (void*)hook_ZNSt3__113__vector_baseIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEED2Ev,
        (void**)&orig_ZNSt3__113__vector_baseIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEED2Ev }, // ! вызывается только при выходе 4 раза
        { "_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE8allocateEm",
        (void *)hook_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE8allocateEm,
        (void **)&orig_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE8allocateEm },
        // { "_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE21__push_back_slow_pathIRKS3_EEvOT_",
        // (void *)hook_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE21__push_back_slow_pathIRKS3_EEvOT_,
        // (void **)&orig_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE21__push_back_slow_pathIRKS3_EEvOT_ }
        { "_ZNSt3__16vectorIPN3gml9GMLObjectENS_9allocatorIS3_EEE8__appendEm",                      // ! вызывается только в начале один раз
        (void *)hook_ZNSt3__16vectorIPN3gml9GMLObjectENS_9allocatorIS3_EEE8__appendEm,              // ! либо же много раз если будет 2 аргумента
        (void **)&orig_ZNSt3__16vectorIPN3gml9GMLObjectENS_9allocatorIS3_EEE8__appendEm }           // !
        // 
    };
    rebind_symbols(mac_rebindings, 8);
    // 
    // std::cout << "orig_ZN22GameBakerLoadingScreenD2Ev : " <<  orig_ZN22GameBakerLoadingScreenD2Ev << std::endl; // 1 if rebinded, else 0
    // std::cout << "orig_ZN22GameBakerLoadingScreenD2Ev*: " << *orig_ZN22GameBakerLoadingScreenD2Ev << std::endl; // 1 if rebinded, else 0
    // std::cout << "orig_ZN22GameBakerLoadingScreenD2Ev&: " << &orig_ZN22GameBakerLoadingScreenD2Ev << std::endl; // just an address

    // std::cout << "hook_ZN22GameBakerLoadingScreenD2Ev*: " << *hook_ZN22GameBakerLoadingScreenD2Ev << std::endl; // always 1
    // std::cout << "hook_ZN22GameBakerLoadingScreenD2Ev&: " << &hook_ZN22GameBakerLoadingScreenD2Ev << std::endl; // always 1

    // rebind OpenGL functions
    rebinding gl_rebindings[] = {
        { "glViewport", (void *)hook_glViewport, (void **)&orig_glViewport },
        { "glShaderSource", (void *)hook_glShaderSource, (void **)&orig_glShaderSource },
        { "glEnableVertexAttribArray", (void *)hook_glEnableVertexAttribArray, 
        (void **)&orig_glEnableVertexAttribArray },
        { "glDisableVertexAttribArray", (void *)hook_glDisableVertexAttribArray, 
        (void **)&orig_glDisableVertexAttribArray },
        { "glDrawArrays", (void *)hook_glDrawArrays, (void **)&orig_glDrawArrays },
        { "glDrawElements", (void *)hook_glDrawElements, (void **)&orig_glDrawElements }
    //    { "glEnable", (void *)hook_glEnable, (void **)&orig_glEnable }
    }; // void APIENTRY glDrawArrays (GLenum mode, GLint first, GLsizei count)
    rebind_symbols(gl_rebindings, 6);

    // rebind SDL2 functions
    rebinding sdl_rebindings[] = {
        { "SDL_Init", (void*)hook_SDL_Init, (void**)&orig_SDL_Init },
        // { "SDL_GetKeyName", (void*)hook_SDL_GetKeyName, (void**)&orig_SDL_GetKeyName }
    };
    rebind_symbols(sdl_rebindings, 1);

    // check all functions
    // 1 if rebinded, else 0 
    std::cout << "CGLFlushDrawable: " <<  orig_CGLFlushDrawable << std::endl; 
    std::cout << "CGMainDisplayID: " <<  orig_CGMainDisplayID << std::endl; 
    std::cout << "CGWarpMouseCursorPosition: " <<  orig_CGWarpMouseCursorPosition << std::endl; 
    std::cout << "CGDisplayHideCursor: " <<  orig_CGDisplayHideCursor << std::endl; 
    std::cout << "ZN3gml17GMLObjectInstance12SetDirectionEd: " <<  orig_ZN3gml17GMLObjectInstance12SetDirectionEd << std::endl; 
    std::cout << "ZZN3gml6randomERKNS_7PGMLVarEE15oneOverRAND_MAX: " <<  orig_ZZN3gml6randomERKNS_7PGMLVarEE15oneOverRAND_MAX << std::endl; 
    std::cout << "ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi: " <<  orig_ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi << std::endl;
    std::cout << "glViewport: " <<  orig_glViewport << std::endl; 
    std::cout << "glShaderSource: " <<  orig_glShaderSource << std::endl; 
    std::cout << "glEnableVertexAttribArray: " <<  orig_glEnableVertexAttribArray << std::endl; 
    std::cout << "glDisableVertexAttribArray: " <<  orig_glDisableVertexAttribArray << std::endl; 
    std::cout << "glDrawArrays: " <<  orig_glDrawArrays << std::endl; 
    std::cout << "glDrawElements: " <<  orig_glDrawElements << std::endl; 
    std::cout << "SDL_Init: " <<  orig_SDL_Init << std::endl; 
    std::cout << "SDL_GetKeyName: " <<  orig_SDL_GetKeyName << std::endl; 
    std::cout << "gml::GMLResources<gml::GMLRoom>::destroyInstance: " << orig_ZN3gml12GMLResourcesINS_7GMLRoomEE15destroyInstanceEv << std::endl; 
    std::cout << "__vector_base<gml::GMLObjectInstance*, allocator<gml::GMLObjectInstance*> >::~__vector_base: " << orig_ZNSt3__113__vector_baseIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEED2Ev << std::endl; 
    std::cout << "vector<gml::GMLObjectInstance*, allocator<gml::GMLObjectInstance*> >::allocate: " << orig_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE8allocateEm << std::endl; 
    std::cout << "vector<gml::GMLObjectInstance*, allocator<gml::GMLObjectInstance*> >::__push_back_slow_path<gml::GMLObjectInstance* const&>: " << orig_ZNSt3__16vectorIPN3gml17GMLObjectInstanceENS_9allocatorIS3_EEE21__push_back_slow_pathIRKS3_EEvOT_ << std::endl;
    std::cout << "vector<gml::GMLObject*, allocator<gml::GMLObject*> >::__append: " << orig_ZNSt3__16vectorIPN3gml9GMLObjectENS_9allocatorIS3_EEE8__appendEm << std::endl;
    
    // orig_ZNSt3__16vectorIPN3gml9GMLObjectENS_9allocatorIS3_EEE8__appendEm
    gui_init();
}

void hookDestructor() {
    if (obj_ptr1 != NULL) free(obj_ptr1);
    if (obj_ptr2 != NULL) free(obj_ptr2);

    // ссылка на указатель, да
    // for (void* &p : inst_ptr) {   // c++11 dope
    //     if (p != NULL) free(p);
    // }

    gui_term();
}

/* 
! если у функции нету void, значит она что-то должна возвращать
0000000102920cc0 t AgWorkerPool::update()
0000000102920dc6 t AgWorkerPool::AgWorkerPool()
00000001027f8b9e t __ZN3gml17GMLObjectInstance12SetDirectionEd              gml::GMLObjectInstance::SetDirection(double)                        0x111f3db9e 0x10e738b9e
0000000102809c44 t __ZN3gml17GMLObjectInstance8SetSpeedEd                   gml::GMLObjectInstance::SetSpeed(double)
00000001027f88e6 t __ZN3gml17GMLObjectInstance8SetSpeedEdd                  gml::GMLObjectInstance::SetSpeed(double, double)
00000001027b2974 t __ZN3gml17GMLObjectInstanceC2EPNS_9GMLObjectEi           gml::GMLObjectInstance::GMLObjectInstance(gml::GMLObject*, int)
0000000102dea900 D __ZZN3gml6randomERKNS_7PGMLVarEE15oneOverRAND_MAX        gml::random(gml::PGMLVar const&)::oneOverRAND_MAX                   // !

000000010280df10 t __ZN22GameBakerLoadingScreenD2Ev                         GameBakerLoadingScreen::~GameBakerLoadingScreen()

0000000102dea058 D __ZTIN3gml16IGMLIteratorDataE                            typeinfo for gml::IGMLIteratorData
0000000102dea070 D __ZTIN3gml20ValueGMLIteratorDataE                        typeinfo for gml::ValueGMLIteratorData
0000000102dea090 D __ZTIN3gml27InstanceListGMLIteratorDataINS_9GMLObjectEEE typeinfo for gml::InstanceListGMLIteratorData<gml::GMLObject>

00000001028534b8 t __ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev                List<gml::GMLObjectInstance*>::List()
0000000102853642 t __ZN4ListIPN3gml17GMLObjectInstanceEED2Ev                List<gml::GMLObjectInstance*>::~List()

00000001028534b8 t __ZN4ListIPN3gml17GMLObjectInstanceEEC2Ev
0000000102853642 t __ZN4ListIPN3gml17GMLObjectInstanceEED2Ev

0000000102dea7d0 D GameBakerMemory::mallocObject()::forceInitializeMemory
0000000102dead10 D GameBakerMemory::mallocPGMLVar()::forceInitializeMemory
0000000102dead00 D GameBakerMemory::mallocGMLValue()::forceInitializeMemory
0000000102dea6c8 D GameBakerMemory::mallocVarPointers()::forceInitializeMemory
0000000102dea6b8 D GameBakerMemory::mallocObjectInstance()::forceInitializeMemory

0000000102dea7d0 D __ZZN15GameBakerMemory12mallocObjectEvE21forceInitializeMemory
0000000102dead10 D __ZZN15GameBakerMemory13mallocPGMLVarEvE21forceInitializeMemory
0000000102dead00 D __ZZN15GameBakerMemory14mallocGMLValueEvE21forceInitializeMemory
0000000102dea6c8 D __ZZN15GameBakerMemory17mallocVarPointersEvE21forceInitializeMemory
0000000102dea6b8 D __ZZN15GameBakerMemory20mallocObjectInstanceEvE21forceInitializeMemory


0000000102dea778 D __ZN3gml12GMLResourcesINS_17GMLObjectInstanceEE11ms_instanceE
0000000102802f76 t __ZN3gml12GMLResourcesINS_17GMLObjectInstanceEE15destroyInstanceEv
*/