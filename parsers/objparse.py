# this is a simple .html to .h parser
# uses Yossarian's objects.html (special thanks for this file)
# usage:
#   python3 -m pip install bs4
#   python3 objp.py objects.html
# it generates objects.h
import sys
from bs4 import BeautifulSoup

objectList = []
class Object():
    id: str
    name: str
    sprite_id: str
    depth: str
    parent_id: str
    mask_id: str
    solid: str
    visible: str
    persistent: str

    def get(self):
        obj = self
        return f"{{ {obj.id}, \"{obj.name}\", {obj.sprite_id}, {obj.depth}, {obj.parent_id}, \
{obj.mask_id}, {obj.solid}, {obj.visible}, {obj.persistent} }},"
        
    def __repr__(self):
        return self.get(self)
    def __str__(self):
        return self.get(self)

def writeFile(l):
    print("savin...")
    file = open("./objects.h", 'w')
    file.write(f"""
#ifndef HLM2_OBJECTS_H
#define HLM2_OBJECTS_H

#ifndef __cplusplus
#define nullptr ((void*)0)
#endif

#include <stdbool.h>

typedef struct {{
    int id;
    const char* name;
    int sprite_id, depth, parent_id, mask_sprite_id;
    bool solid, visible, persistent;
}} objectTable;

const unsigned int objListSize = {len(l)};
const objectTable objList[] = {{
""")
    for item in l:
        file.write(f"   {item.get()}\n")
    file.write("""};

inline const objectTable *getObjectByID(int id) {
    for (int i = 0; i < objListSize; i++) {
        if (objList[i].id == id) return &objList[i];
    }
    return nullptr;
}

#endif // HLM2_OBJECTS_H""")
    file.close()

def parseHTML(t):
    print("parsin...")
    soup = BeautifulSoup(t, 'lxml')
    trs = soup.find('table').find_all('tr')

    for tr in trs:
        obj = Object()
        tds = tr.find_all('td')
        obj.id          = tds[0].text
        obj.name        = tds[1].text.split("/")[-1]
        obj.sprite_id   = tds[2].text
        obj.depth       = tds[3].text
        obj.parent_id   = tds[4].text
        obj.mask_id     = tds[5].text
        obj.solid       = tds[6].text.lower()
        obj.visible     = tds[7].text.lower()
        obj.persistent  = tds[8].text.lower()
        objectList.append(obj)
    
    objectList.pop(0)
    writeFile(objectList)

def openFile(path):
    print(f"openin {path}...")
    try:
        file = open(path, 'r')
    except:
        print(f"ERROR: unable to open {path}")
        exit(1)

    line = file.readline()
    text = line
    while line:
        line = file.readline()
        text += line

    parseHTML(text)
    file.close()

if __name__ == "__main__":
    argc = len(sys.argv)
    if argc > 1:
        for i, arg in enumerate(sys.argv):
            if i >= 1:
                openFile(arg)
                print("done")
    else:
        print("python3 objp.py <file>\noutput: objects.h")