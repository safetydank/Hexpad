#pragma once
#include <string>
#include <vector>

#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Camera.h"

#include "GuiController.h"

#include "boost/unordered_set.hpp"
#include "boost/foreach.hpp"
#define FOREACH BOOST_FOREACH

namespace war {

typedef ci::Vec2i HexCoord;

enum HexDir {
    NORTHWEST = 0,
    NORTH     = 1,
    NORTHEAST = 2,
    SOUTHEAST = 3,
    SOUTH     = 4,
    SOUTHWEST = 5
};

struct HexEdges {
    //  
    HexCoord mHex;
    //  Edge data stored in lower 6 bits of a byte
    unsigned char mEdges;

    HexEdges(HexCoord& hex) : mEdges(0), mHex(hex) { 
    }

    void addEdge(unsigned int edge) { mEdges |= 1 << edge; }

    //  Returns a vector of HexDir edges
    std::vector<HexDir> getEdges() {
        std::vector<HexDir> edges;
        for (int i=0; i < 6; ++i) {
            if ((1 << i) & mEdges) {
                edges.push_back(static_cast<HexDir>(i));
            }
        }
        return edges;
    }

    HexCoord getHexCoord() { return mHex; }
    bool hasEdge(HexDir edge) {
        return (mEdges & (1 << static_cast<unsigned int>(edge)));
    }

    void reset() { mEdges = 0; }
};

//  Adjacency check results
struct HexAdjacent
{
    HexCoord nw;
    HexCoord n;
    HexCoord ne;
    HexCoord se;
    HexCoord s;
    HexCoord sw;

    HexCoord getAdjacent(HexDir dir) {
        switch(dir) {
            case NORTHWEST:
                return nw;
                break;
            case NORTH:
                return n;
                break;
            case NORTHEAST:
                return ne;
                break;
            case SOUTHEAST:
                return se;
                break;
            case SOUTH:
                return s;
                break;
            case SOUTHWEST:
                return sw;
                break;
            default:
                break;
        }
    }
    std::vector<HexCoord> toVector();
    std::string toString();
};

/** 
  * A regular hexagon grid
  *
  * Models a hexagon grid as an isometric projection of cubes on the x+y+z=0 plane.
  * Based on http://www-cs-students.stanford.edu/~amitp/Articles/Hexagon2.html
  *
*/
class HexGrid 
{

private:
    double mXSpacing;
    double mYSpacing;

public:
    HexGrid(double xspacing=1.0, double yspacing=1.0);
    ~HexGrid() { };

    void setSpacing(double xspacing, double yspacing);

    HexCoord  WorldToHex(ci::Vec3f worldPos);
    ci::Vec3f HexToWorld(HexCoord hexPos, bool scale=true);

    HexAdjacent adjacent(HexCoord pos);
};

class HexCell
{
public:
    HexCell();
    ~HexCell();

    void setPos(HexCoord& pos);

    void       setColor(ci::ColorA& color);
    ci::ColorA& getColor();
    int  getLand();
    void setLand(int land);
    int  getOwner();
    void setOwner(int id);

private:
    HexCoord   mPos;
    ci::ColorA mColor;
    int        mLand;  //  0 for sea, 1 for land
    int        mOwner; //  player owner ID
};

// XXX replace with Territory
class HexRegion
{
private:
    std::vector<HexCoord> mHexes;

public:
    HexRegion() { }
    ~HexRegion() { }

    void addHexes(std::vector<HexCoord>& hexes);
    void clear();
};

//  Predicate check for connected cells belonging to the same player
struct ConnectedByOwner
{
    int mOwner;

    ConnectedByOwner(int ownerId) : mOwner(ownerId) { }
    bool operator()(HexCell& cell) {
        return (mOwner >=0 && cell.getOwner() == mOwner);
    }
};

//  Records connectivity data for a region
//  XXX replace the vector returned by connected() with this
struct HexConnectivity
{
    std::vector<HexCoord> cells;
    //  cells neighbouring non-matching cells
    std::vector<HexCoord> borderCells;
};

//  a pair used in hex search, tracks a candidate coord and the originating
//  hex (HexParent)
typedef std::pair<HexCoord, HexCoord> HexParent;

class HexMap
{
private:
    HexGrid& mHexGrid;
    ci::Vec2i mSize;
    HexCell** mCells;

public:
    HexMap(HexGrid& grid, int width, int height);
    ~HexMap();

    HexCell& at(HexCoord& pos);
    ci::Vec2i getSize();

    //  Find all connected land cells belonging to the same player
    //  predicate -- a functor taking a HexCell or reference as its only argument
    template <typename T>
    // std::vector<HexCoord> connected(HexCoord pos, T predicate)
    HexConnectivity connected(HexCoord pos, T predicate)
    {
        HexConnectivity conn;

        boost::unordered_set<HexCoord> search;
        boost::unordered_set<HexCoord> checked;
        boost::unordered_set<HexCoord> matched;
        boost::unordered_set<HexCoord> borders;

        // int owner = at(pos).getOwner();
        search.emplace(pos);
        while (!search.empty()) {
            // HexParent checkParent = *(search.begin());
            HexCoord check = *(search.begin());
            // HexCoord& check = checkParent.first;

            if (predicate(at(check)) == true) {
                // match
                conn.cells.push_back(check);            
                matched.emplace(check);

                std::vector<HexCoord> adjacent = mHexGrid.adjacent(check).toVector();
                FOREACH (HexCoord coord, adjacent) {
                    if (isValid(coord) && checked.find(coord) == checked.end()) {
                        //  queue this cell and track its origin (parent)
                        search.emplace(coord);
                    }
                }
            }
            else {
                //  XXX slightly inefficient, would be better to selectively check adjacent
                //  cells based on matched and non-matches
                std::vector<HexCoord> adjacent = mHexGrid.adjacent(check).toVector();
                FOREACH (HexCoord coord, adjacent) {
                    if (predicate(at(coord))) {
                        borders.emplace(coord);
                    }
                }
            }
            checked.emplace(check);
            search.erase(check);
        }

        conn.cells       = std::vector<HexCoord>(matched.begin(), matched.end());
        conn.borderCells = std::vector<HexCoord>(borders.begin(), borders.end());
        return conn;
    }

    std::vector<HexEdges> extractEdges(HexConnectivity& conn);

    //  XXX should move to WarGame
    //std::vector<int> countHexes();

    //  Check position lies within hex map
    bool isValid(HexCoord& pos);
    HexGrid& hexGrid() { return mHexGrid; }

    // obsolete?  scan territory for regions
    // std::vector<HexRegion> regions();

    //  clear all map tiles
    void clear();

};
typedef boost::shared_ptr<HexMap> HexMapPtr;

class HexRender
{
private:
    ci::Vec2i         mWindowSize;
    ci::gl::VboMesh   mHexMesh;
    ci::gl::VboMesh   mHexOutlineMesh;
    ci::CameraPersp   mCamera;
    ci::Vec3f         mCameraTo;

    //  Frustum culling
    HexCoord         mBottomLeft;
    HexCoord         mTopRight;

    ci::gl::GlslProg  mShader;

    HexMap&  mHexMap;
    HexGrid& mHexGrid;

    HexCoord mSelectedHex;

    void generateMeshes();

public:
    HexRender(HexMap& map);
    ~HexRender(); 

    void setup(ci::Vec2i wsize);
    void update();
    
    void drawHexes();
    void drawSelection();

    ///  Cast a ray from camera projection plane (u,v) onto hex grid's plane
    ci::Vec3f raycastHexPlane(float u, float v);

    void setSelectedHex(HexCoord loc) { mSelectedHex = loc; }
    HexCoord getSelectedHex() { return mSelectedHex; }

    ci::Vec3f& getCameraTo() { return mCameraTo; }
    void setCameraTo(ci::Vec3f& cameraTo);

    ci::Camera& getCamera();
};
typedef boost::shared_ptr<HexRender> HexRenderPtr;

} // namespace
