#ifndef HLM2_GML_H
#define HLM2_GML_H

#include <iostream>

#include <ctype.h>
#include <stdio.h>

#include "objects.h"

namespace gml {
    // ! размер 0x1300 (4864)
    class GMLObjectInstance {
        void *costructor;
        // uint32_t* unknownPtrA;
    };

    // // ! размер 0xB0 (176)
    // ! размер 0x74 (116)
    struct GMLObject {
        void *costructor;
        void *unknown1, *unknown2, *unknown3;
        void *func1, *func2;
        void *unknown4;                                                                             // 0x30
        void *func3;                                                                                // 0x38
        uint32_t var1, var2;
        void *func4;
        uint32_t var3, var4;
        void *unknown5, *func5, *unknown6;
        uint32_t id; //, var5;
        // void *unknown7;
        // void *unknown8, *unknown9, *unknown10;
        // void *func6;
        // void *func7;                                                                                // 0xA8
        // void *func8;                                                                                // 0xB0

        // char *sprite_index;
        // int depth;
        // char *parent;
        // char *mask_sprite;
        // bool solid, visible;
        // bool persistent, prority;
        
        //uint32_t* id;//, sprite_index, depth;//, parent, mask_sprite, visible, pesistent;
        //uint64_t* priority;
        //uint8_t data[13];
        //uint8_t* data[0x000090 / 8];
        // 000d20
        // 4d 61 69 6e 54 68 72 64: MainThrd ! ThreadSetDebugName
        // uint32_t* unknownPtr[15];        
    };
}

/*
std::ostream& operator<< (std::ostream &out, gml::GMLObject const& data) {
    out << data.getmpg() << ':';
    out << data.getcylinders() << ':';
    // and so on... 
    return out;
}
*/

#endif // HLM2_GML_H