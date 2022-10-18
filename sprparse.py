# this is a simple .html to .h parser
# uses Yossarian's sprites.html (special thanks for this file)
# usage:
#   python3 -m pip install bs4
#   python3 sprparse.py sprites.html
# it generates sprites.h
import sys
from bs4 import BeautifulSoup

spriteList = []
class Sprite():
    id: str
    name: str
    width: str
    height: str
    count: str

    def get(self):
        return f"{{ {self.id}, \"{self.name}\", {self.width}, {self.height}, {self.count} }},"
        
    def __repr__(self):
        return self.get(self)
    def __str__(self):
        return self.get(self)

def writeFile(l):
    print("savin...")
    file = open("./sprites.h", 'w')
    file.write(f"""
#ifndef HLM2_SPRITES_H
#define HLM2_SPRITES_H

#ifndef __cplusplus
#define nullptr ((void*)0)
#endif

#include <stdbool.h>

typedef struct {{
    int id;
    const char* name;
    int width, height, count;
}} spriteTable;

const unsigned int sprListSize = {len(l)};
const spriteTable sprList[] = {{
""")
    for item in l:
        file.write(f"   {item.get()}\n")
    file.write("""};

inline const spriteTable *getSpriteByID(int id) {
    for (int i = 0; i < sprListSize; i++) {
        if (sprList[i].id == id) return &sprList[i];
    }
    return nullptr;
}

#endif // HLM2_SPRITES_H""")
    file.close()

def parseHTML(t):
    print("parsin...")
    soup = BeautifulSoup(t, 'lxml')
    trs = soup.find('table').find_all('tr')

    for tr in trs:
        spr = Sprite()
        tds = tr.find_all('td')
        spr.id          = tds[0].text
        spr.name        = tds[1].text.split("/")[-1]
        spr.width       = tds[2].text
        spr.height      = tds[3].text
        spr.count       = tds[4].text
        spriteList.append(spr)
    
    spriteList.pop(0)
    writeFile(spriteList)

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
        print("python3 sprparse.py <file>\noutput: sprites.h")