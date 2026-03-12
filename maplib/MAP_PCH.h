#ifndef MAP_PCH_H
#define MAP_PCH_H

#include <windows.h>
#include <fstream>
#include <vector>
#include <map>
#include <list>

typedef unsigned short WORD;
typedef unsigned int DINT;
using std::vector;
using std::map;
using std::list;
using std::ifstream;
using std::ofstream;



typedef struct GAME_POINT_2S
{
    uint16_t x;
    uint16_t y;

} GAME_POINT_2S;

#define OBJECTID_NULL         0
#define SECTORPOSITION_NULL  0



#endif // !MAP_PCH_H