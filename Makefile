# bear -- make
# STRIP = strip -sgxX
INAME = ./
# INAME = /Users/alepacho/Public/hlml/
STD = -std=c++11

# `sdl2-config --libs`
LIBS = -framework SDL2 -framework OpenGL -framework Cocoa -F/Library/Frameworks -framework GameController
# -L./mach_override/build -lmach_override_64 
LPATH = ./obj/
IPATH = -I./imgui -I./fishhook -I./ObjectiveHooker -I/Library/Frameworks/SDL2.framework/Headers
# -I./mach_override 
RPATH = -rpath @executable_path/
SRC = ./src/hook.cpp 

# -install_name $(INAME)$@ 
# 	clang -c -o $(LPATH)ObjectiveHooker.o ./ObjectiveHooker/ObjectiveHooker.m

all: clear macos 

AutoHook.o:
	mkdir -p $(LPATH)
	clang -c -o $(LPATH)AutoHook.o ./src/AutoHook.m -fmodules

gui_mac:
	mkdir -p $(LPATH)
	clang++ $(STD) $(IPATH) -Wno-deprecated -c -o $(LPATH)gui.o -Xclang -x -Xclang objc++ ./src/gui_mac.mm

clear_imgui:
	rm -rf $(LPATH)imgui/imgui.o $(LPATH)imgui/imgui_demo.o $(LPATH)imgui/imgui_draw.o
	rm -rf $(LPATH)imgui/imgui_tables.o $(LPATH)imgui/imgui_widgets.o
	rm -rf $(LPATH)imgui/imgui_impl_opengl3.o $(LPATH)imgui/imgui_impl_osx.o $(LPATH)imgui/imgui_impl_sdl.o 

imgui_all: clear_imgui
	mkdir -p $(LPATH)/imgui
	clang++ $(STD) $(IPATH) -c -o $(LPATH)imgui/imgui.o ./imgui/imgui.cpp
	clang++ $(STD) $(IPATH) -c -o $(LPATH)imgui/imgui_demo.o ./imgui/imgui_demo.cpp
	clang++ $(STD) $(IPATH) -c -o $(LPATH)imgui/imgui_draw.o ./imgui/imgui_draw.cpp
	clang++ $(STD) $(IPATH) -c -o $(LPATH)imgui/imgui_tables.o ./imgui/imgui_tables.cpp
	clang++ $(STD) $(IPATH) -c -o $(LPATH)imgui/imgui_widgets.o ./imgui/imgui_widgets.cpp
	clang++ $(STD) $(IPATH) -c -o $(LPATH)imgui/imgui_impl_osx.o -Xclang -x -Xclang objc++ ./imgui/backends/imgui_impl_osx.mm
	clang++ $(STD) $(IPATH) -c -o $(LPATH)imgui/imgui_impl_opengl3.o ./imgui/backends/imgui_impl_opengl3.cpp
	clang++ $(STD) $(IPATH) -c -o $(LPATH)imgui/imgui_impl_sdl.o -I/Library/Frameworks/SDL2.framework/Headers ./imgui/backends/imgui_impl_sdl.cpp
	ar cr $(LPATH)libimgui_all.a $(LPATH)imgui/*.o

fishhook.o:
	mkdir -p $(LPATH)
	clang ./fishhook/fishhook.c -c -o $(LPATH)fishhook.o

hook: 
	clang++ $(SRC) $(LPATH)fishhook.o $(LPATH)gui.o $(LPATH)AutoHook.o -L$(LPATH) -limgui_all $(STD) -o $(INAME)hook.dylib -dynamiclib -ldl $(LIBS) $(IPATH) $(RPATH) -install_name "@loader_path/hook.dylib" 
	install_name_tool -id @rpath/hook.dylib $(INAME)hook.dylib

macos: fishhook.o AutoHook.o imgui_all gui_mac hook
	cp $(INAME)hook.dylib ./HotlineMiami2.app/Contents/MacOS/hook.dylib
	cp $(INAME)hlm2mod.sh ./HotlineMiami2.app/Contents/MacOS/hlm2mod.sh

# тупо перемещает либу с 644 настройками
imacos: hook
	install -m0644 $< "$(INAME)"

clear:
	rm -rf hook.dylib fishhook.o $(LPATH)



# imacos