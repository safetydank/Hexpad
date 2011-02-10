#include "Hex.h"

#include <string>
#include <sstream>

using namespace ci;
using namespace ci::app;
using namespace war;

using std::string;
using std::vector;
using boost::shared_ptr;
using boost::unordered_set;

HexGrid::HexGrid(double xspacing, double yspacing) 
    : mXSpacing(xspacing), mYSpacing(yspacing) { }

void HexGrid::setSpacing(double xspacing, double yspacing) 
{
    mXSpacing = xspacing;
    mYSpacing = yspacing;
}

HexCoord HexGrid::WorldToHex(Vec3f worldPos)
{
    double x = worldPos.x / mXSpacing;
    double y = worldPos.y / mYSpacing;
    double z = -0.5*x - y;
           y = y - 0.5*x;

    int ix = static_cast<int>(floor(x+0.5));
    int iy = static_cast<int>(floor(y+0.5));
    int iz = static_cast<int>(floor(z+0.5));
    int s = ix+iy+iz;
    if (s)
    {
        double abs_dx = fabs(ix-x);
        double abs_dy = fabs(iy-y);
        double abs_dz = fabs(iz-z); 
        if (abs_dx >= abs_dy && abs_dx >= abs_dz)
            ix -= s;
        else if (abs_dy >= abs_dx && abs_dy >= abs_dz)
            iy -= s;
        else
            iz -= s;
    }
    iy = (((s = iy - iz) < 0) ? s - 1 + ((ix+1) & 1) : s + 1 - (ix & 1)) / 2;

    return HexCoord(ix, iy);
}

Vec3f HexGrid::HexToWorld(HexCoord hexPos, bool scale)
{
    float x = hexPos.x * float(scale ? mXSpacing : 1.0f);
    float yoffset = -0.5f * ((hexPos.x & 1) ? hexPos.x-1 : hexPos.x);
    float y = (hexPos.y + 0.5f * hexPos.x + yoffset) * float(scale ? mYSpacing : 1.0f);
    return Vec3f(x, y, 0);
}

//  Returns neighbours in order nw, n, ne, se, s, sw
HexAdjacent HexGrid::adjacent(HexCoord pos)
{
    HexAdjacent result;

    if (pos.x % 2) {
        // odd column
        result.nw = HexCoord(pos.x-1, pos.y+1);
        result.n  = HexCoord(pos.x, pos.y+1);
        result.ne = HexCoord(pos.x+1, pos.y+1);
        result.se = HexCoord(pos.x+1, pos.y);
        result.s  = HexCoord(pos.x, pos.y-1);
        result.sw = HexCoord(pos.x-1, pos.y);
    }
    else {
        // even column
        result.nw = HexCoord(pos.x-1, pos.y);
        result.n  = HexCoord(pos.x, pos.y+1);
        result.ne = HexCoord(pos.x+1, pos.y);
        result.se = HexCoord(pos.x+1, pos.y-1);
        result.s  = HexCoord(pos.x, pos.y-1);
        result.sw = HexCoord(pos.x-1, pos.y-1);
    }

    return result;
}

vector<HexCoord> HexAdjacent::toVector()
{
    vector<HexCoord> ret;
    ret.push_back(nw);
    ret.push_back(n);
    ret.push_back(ne);
    ret.push_back(se);
    ret.push_back(s);
    ret.push_back(sw);
    return ret;
}

string HexAdjacent::toString()
{
    std::stringstream ss;
    ss << "nw " << nw << " n " << n << " ne " << ne;
    ss << " sw " << sw << " s " << s << " se " << se;

    return ss.str();
}


HexCell::HexCell()
{
    mLand = 0;
}

HexCell::~HexCell()
{ 
}

void HexCell::setPos(HexCoord& pos) 
{
    mPos = pos;
}

void HexCell::setColor(ColorA& color) 
{
    mColor = color;
}

ColorA& HexCell::getColor()
{
    return mColor;
}

int HexCell::getLand()
{
    return mLand;
}

void HexCell::setLand(int land)
{
    mLand = land;
}

int HexCell::getOwner()
{
    return mOwner;
}

void HexCell::setOwner(int id)
{
    mOwner = id;
}

HexMap::HexMap(HexGrid& grid, int width, int height) : mHexGrid(grid)
{ 
    mSize.x = width;
    mSize.y = height;

    mCells = new HexCell*[width];
    for (int i=0; i < mSize.x; ++i ) {
        for (int j=0; j < mSize.y; ++j) {
            mCells[i] = new HexCell[mSize.y];
        }
    }
    clear();
}

HexMap::~HexMap()
{
    for (int i=0; i < mSize.x; ++i) {
        delete [] mCells[i];
    }
}

HexCell& HexMap::at(HexCoord& pos)
{
    assert(pos.x >=0 && pos.x < mSize.x && pos.y >= 0 && pos.y < mSize.y);
    return mCells[pos.x][pos.y];
}

Vec2i HexMap::getSize()
{
    return mSize;
}

//  Check position lies on hex map
bool HexMap::isValid(HexCoord& pos)
{
    return (pos.x >= 0 && pos.y >= 0 && pos.x < mSize.x && pos.y < mSize.y);
}

// vector<HexCoord> HexMap::connected(HexCoord pos)
// {
//     unordered_set<HexCoord> search;
//     unordered_set<HexCoord> checked;
//     std::vector<HexCoord> result;
// 
//     int owner = at(pos).getOwner();
//     search.emplace(pos);
//     while (!search.empty()) {
//         HexCoord check = *(search.begin());
//         if (at(check).getOwner() == owner) {
//             result.push_back(check);            
//             vector<HexCoord> adjacent = mHexGrid.adjacent(check).toVector();
//             FOREACH (HexCoord coord, adjacent) {
//                 if (isValid(coord) 
//                         && checked.find(coord) == checked.end() 
//                         && at(coord).getOwner() >= 0) {
//                     search.emplace(coord);
//                 }
//             }
//         }
//         checked.emplace(check);
//         search.erase(check);
//     }
// 
//     return result;
// }

void HexRegion::addHexes(vector<HexCoord>& hexes)
{
    for (vector<HexCoord>::iterator it=hexes.begin(); it != hexes.end(); ++it) {
        mHexes.push_back(*it);
    }
}

// std::vector<HexRegion> HexMap::regions()
// {
//     unordered_set<HexCoord> search;
//     unordered_set<HexCoord> checked;
//     vector<HexRegion> regions;
// 
//     HexCoord pos;
//     for (int ix=0; ix < mSize.x; ++ix) {
//         for (int iy=0; iy < mSize.y; ++iy) {
//             pos = HexCoord(ix, iy);
//             if (at(pos).getLand()) {
//                 search.emplace(pos);
//             }
//         }
//     }
// 
//     while (search.begin() != search.end()) {
//         HexCoord check = *(search.begin());
// 
//         vector<HexCoord> conn = connected(check);
//         for (vector<HexCoord>::iterator it = conn.begin(); it != conn.end(); ++it) {
//             search.erase(*it);
//         }
// 
//         HexRegion region;
//         region.addHexes(conn);
//         regions.push_back(region);        
// 
//         //  XXX not required, already erased above
//         search.erase(check);
//     }
// 
//     return regions;
// }

void HexMap::clear()
{
    for (int i=0; i < mSize.x; ++i ) {
        for (int j=0; j < mSize.y; ++j) {
            HexCell& cell = at(HexCoord(i,j));
            cell.setPos(HexCoord(i,j));
            cell.setColor(ColorA(0.15f, 0.15f, 0.15f, 1.0f));
            cell.setOwner(-1);
        }
    }
}

//vector<int> HexMap::countHexes()
//{
//    vector<int> counts();
//
//    for (int ix=0; ix < mSize.x; ++ix) {
//        for (int iy=0; iy < mSize.y; ++iy) {
//            HexCell& cell = at(ix, iy);
//            if (cell.getLand()) {
//            }
//        }
//    }
//}


vector<HexEdges> HexMap::extractEdges(HexConnectivity& conn)
{
    //  Extract a border loop as a vector of HexEdges
    vector<HexEdges> result;
    unordered_set<HexCoord> borderCells(conn.borderCells.begin(), conn.borderCells.end());

    HexCoord start = *(conn.borderCells.begin());
    HexCoord coord = start;
    int owner = at(coord).getOwner();

    ColorA color(1.0f,1.0f,1.0f,1.0f);
    do {
        // vector<HexCoord> adjacents = mHexGrid.adjacent(coord).toVector();
        HexAdjacent adjacents = mHexGrid.adjacent(coord);

        HexEdges edges(coord);

        // FOREACH (HexCoord adjacent, adjacents) {
        for (int i=0; i < 6; ++i) {
            HexDir dir = static_cast<HexDir>(i);
            HexCoord adjacent = adjacents.getAdjacent(dir);
            if (isValid(adjacent) && at(adjacent).getOwner() != owner) { 
               //  a border edge
               edges.addEdge(dir);
            }
        }
        result.push_back(edges);
        at(edges.getHexCoord()).setColor(color);
        color.a *= 0.8f;

        //  Choose the next hex in the border loop
        HexCoord nextHex;

        //  eligible next cells based on the border edges
        vector<HexDir> validDirs;
        for (int i=0; i < 6; ++i) {
            HexDir edge = static_cast<HexDir>((i+1) % 6);
            if (edges.hasEdge(edge)) {
                validDirs.push_back(static_cast<HexDir>(edge == 0 ? 5 : edge-1));
                validDirs.push_back(static_cast<HexDir>(edge == 5 ? 0 : edge+1));
            }
        }

        FOREACH (HexDir validDir, validDirs) {
            HexCoord adjacent = adjacents.getAdjacent(validDir);
            // test for an untouched border cell
            if (borderCells.find(adjacent) != borderCells.end()) {
               nextHex = adjacent;
               borderCells.erase(adjacent);
               break;
            }
        }
        // assert (nextHex != coord);
        coord = nextHex;

    } while (coord != start && result.size() < conn.borderCells.size()); // XXX DEBUGGING TERMINATE CONDITION

    // XXX assert pending queue is empty
    
    return result;
}

