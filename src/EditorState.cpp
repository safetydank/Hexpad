#include "EditorState.h"
#include "WarGame.h"
#include "cinder/Vector.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"

#include "boost/unordered_set.hpp"

#include <string>
#include <vector>
#include <algorithm>
#include <utility>

using namespace ci;
using namespace ci::app;
using namespace war;
using std::vector;
using std::string;
using std::find;

static Rand random;

#define Game      GG.warGame
#define Gui       GG.gui
#define HexRender GG.hexRender
#define Map       GG.hexMap

EditorState::EditorState(StateManager& manager, Shared& shared) : State(manager, shared)
{
}

EditorState::~EditorState()
{
}

void EditorState::enter()
{
    GuiLabelData labelData;
    labelData.FontSize = 15.0f;
    mLabel = Gui.createLabel(labelData, false);

    GuiQuadData boxQuad;
    boxQuad.Color = ColorA(0, 0, 0, 0.8f);
    GuiBoxData debugLabelBox;
    debugLabelBox.Padding = 2.0f;

    mLabelBox = Gui.createBox(boxQuad, debugLabelBox, true);
    mLabelBox->addChild(mLabel);

    // camera
    Vec3f cameraTo = GG.hexGrid.HexToWorld(HexCoord(32, 16));
    // XXX
    // Vec3f eye = cameraTo;
    cameraTo.z = 50.0f;
    HexRender.getCamera().setEyePoint(cameraTo);
    HexRender.setCameraTo(cameraTo);
}

void EditorState::leave()
{
    // mLabelBox->detach();
    Gui.detachAll();
}

void EditorState::update()
{
    Vec2f mousePos = GG.mouse.getPos();
    Vec3f planeHit = HexRender.raycastHexPlane(mousePos.x, mousePos.y);

    //  Pan the camera
    if (GG.mouse.getRight() == PRESSED) {
        if (!mDragStart) {
            mDragOrigin = mousePos;
            mDragEyeOrigin = Vec2f(HexRender.getCamera().getEyePoint());
            mDragStart = true;
        }

        Vec2f stroke = (mousePos - mDragOrigin) * -48.0f;
        stroke.x *= GG.mouse.getAspectRatio();
        Vec2f dragTo = stroke + mDragEyeOrigin;
        Camera& camera = HexRender.getCamera();
        Vec3f cameraTo(dragTo.x, dragTo.y, camera.getEyePoint().z);

        HexRender.setCameraTo(cameraTo);
    }
    else {
        mDragStart = false;
    }

    //  Set land
    if (GG.mouse.getLeft() == PRESSED) {
        HexCoord selectedHex = GG.hexGrid.WorldToHex(planeHit);
        if (Map.isValid(selectedHex)) {
            Map.at(selectedHex).setColor(ColorA(1.0f, 1.0f, 0.8f, 1.0f));
            Map.at(selectedHex).setLand(1);
        }
    }

    std::stringstream ss;
    HexCoord selectedHex = GG.hexGrid.WorldToHex(planeHit);
    HexRender.setSelectedHex(selectedHex);
    ss << "Hex:" << selectedHex << " Owner: " << Map.at(selectedHex).getOwner(); // << " World: " << planeHit;

    GuiLabelData& labelData = mLabel->getData();
    labelData.Text = ss.str();

    HexRender.update();
}

void EditorState::draw()
{
    gl::clear( Color( 0.3f, 0.3f, 0.3f ) );
    HexRender.drawHexes();
    HexRender.drawSelection();
}

void EditorState::mouseWheel(MouseEvent event)
{
    Vec3f cameraTo = HexRender.getCameraTo();
    cameraTo.z = cameraTo.z - event.getWheelIncrement() * 5.0f;
    HexRender.setCameraTo(cameraTo);
}

void EditorState::keyDown(ci::app::KeyEvent event)
{
    // XXX duplicate raycast
    Vec2f pos = GG.mouse.getPos();
    Vec3f planeHit = HexRender.raycastHexPlane(pos.x, pos.y);
    HexCoord selectedHex = GG.hexGrid.WorldToHex(planeHit);

    int keycode = event.getCode();

    Vec3f& cameraTo = HexRender.getCameraTo();
    if (keycode == app::KeyEvent::KEY_ESCAPE) {
        mManager.setActiveState("title");
    }
    else if (keycode == app::KeyEvent::KEY_UP) {
        cameraTo += Vec3f(0, 2.0f, 0);
    }
    else if (keycode == app::KeyEvent::KEY_DOWN) {
        cameraTo += Vec3f(0, -2.0f, 0);
    }   
    else if (keycode == app::KeyEvent::KEY_LEFT) {
        cameraTo += Vec3f(-2.0f, 0, 0);
    }
    else if (keycode == app::KeyEvent::KEY_RIGHT) {
        cameraTo += Vec3f(2.0f, 0, 0);
    }
    else if (keycode == app::KeyEvent::KEY_g) {
        generate();
    }
    else if (keycode == app::KeyEvent::KEY_DELETE) {
        Map.at(selectedHex).setLand(0);
    }
    else if (keycode == app::KeyEvent::KEY_SPACE) {
        mManager.setActiveState(string("game"));
    }
    else if (keycode == app::KeyEvent::KEY_c) {
        ConnectedByOwner byOwner(Map.at(selectedHex).getOwner());
        HexConnectivity connected = Map.connected(selectedHex, byOwner);

        // for (vector<HexCoord>::iterator it = connected.cells.begin(); it != connected.end(); ++it) {
        FOREACH (HexCoord coord, connected.cells) {
            HexCell& cell = Map.at(coord);
            ColorA cellColor = cell.getColor();
            cellColor.r = 0.5f * cellColor.r;
            cellColor.g = 0.5f * cellColor.g;
            cellColor.b = 0.5f * cellColor.b;
            cell.setColor(cellColor);
        }

        FOREACH (HexCoord coord, connected.borderCells) {
            HexCell& cell = Map.at(coord);
            cell.setColor(ColorA(1,1,0,1));
        }
        vector<HexEdges> edges = Map.extractEdges(connected);

    }
    else if (keycode == app::KeyEvent::KEY_BACKSPACE) {
        Map.clear();
    }
    else if (keycode == app::KeyEvent::KEY_1) {
        Map.at(selectedHex).setColor(ColorA(1.0f, 0, 0, 1.0f));
    }
    else if (keycode == app::KeyEvent::KEY_2) {
        Map.at(selectedHex).setColor(ColorA(0, 0, 1.0f, 1.0f));
    }
}

void EditorState::generate()
{
    Map.clear();
    //  clear all the territories
    Game.getTerritories().clear();

    //  City positions
    HexCoord start(32, 16);

    //  5 players, 2 cities each first
    int cities = 10;

    vector<HexCoord> positions;
    int gw = ceil(cities / 3.0f);
    int gh = ceil(cities / 4.0f);

    //  Spread out cities in a perturbed grid
    int cityCount = cities;
    for (int i=0; i < gw; ++i) {
        for (int j=0; j < gh; ++j ) {
            //  spacing of (7,7)
            //  XXX gw factors are completely fudged
            HexCoord offset = HexCoord(32, 16) - (HexCoord(gw*6.5, gh*2.5) / 2);
            HexCoord perturb = HexCoord(Rand::randInt(-2, 4), Rand::randInt(-2, 4));
            HexCoord pos = HexCoord(i*7, j*7) + offset + perturb;
            positions.push_back(pos);
            if (--cityCount == 0) {
                break;
            }
        }
    }

    // //  random land generation
    // {
    //     Vec2i mapSize = Map.getSize();
    //     for (int i=0; i < mapSize.x; ++i ) {
    //         for (int j=0; j < mapSize.y; ++j) {
    //             if (Rand::randFloat() < 0.87f) {
    //                 HexCell& cell = Map.at(HexCoord(i,j));
    //                 cell.setColor(Color(0.45f, 0.45f, 0.45f));
    //             }
    //         }
    //     }
    // }

    //  Add territories to the game
    vector<Territory>& territories = Game.getTerritories();
    int owner = 1;
    FOREACH (HexCoord& coord, positions) {
        Territory terr(coord);
        terr.addCell(coord);
        territories.push_back(terr);
        HexCell& cell = Map.at(coord);
        cell.setOwner(owner++);
    }

    //  XXX build up territories
    owner = 1;
    FOREACH (Territory& terr, territories) {
        // Starting cell
        HexCoord start = *(terr.mCells.begin());
        assert(!terr.mCells.empty());

        // Mark all adjacent cells
        HexAdjacent adj = GG.hexGrid.adjacent(start);
        for (int terrOffset=0; terrOffset < 6; ++terrOffset) {
            HexCoord newCell = *(((HexCoord*)&adj) + terrOffset);
            if (Map.isValid(newCell)) {
                terr.addCell(newCell);
                Map.at(newCell).setOwner(owner);
            }
        }

        // Stamp on a random cell
        int attempts = 500;
        while (terr.mCells.size() < 48) {
            int iEdge = Rand::randInt(1, terr.mCells.size());
            HexCoord edgeCell = terr.mCells[iEdge];
            // int terrOffset = Rand::randInt(0, 6);
            adj = GG.hexGrid.adjacent(edgeCell);

            vector<int> stamp;
            switch (Rand::randInt(0, 4)) {
                case 0:
                    //  nw, ne
                    stamp.push_back(NORTHWEST);
                    stamp.push_back(SOUTHWEST);
                    break;
                case 1:
                    // sw, se
                    stamp.push_back(NORTHEAST);
                    stamp.push_back(SOUTHEAST);
                    break;
                // case 2:
                //     // n, s
                //     stamp.push_back(NORTH);
                //     stamp.push_back(NORTHEAST);
                //     break;
                // case 3:
                //     stamp.push_back(NORTH);
                //     stamp.push_back(NORTHWEST);
                //     break;
                // case 5:
                //     stamp.push_back(SOUTH);
                //     stamp.push_back(SOUTHEAST);
                //     break;
                // case 6:
                //     stamp.push_back(SOUTH);
                //     stamp.push_back(SOUTHWEST);
                //     break;
                default:
                    // all neighbours
                    for (int i=0; i < 6; ++i)
                        stamp.push_back(i);
                    break;
            }

            FOREACH (int terrOffset, stamp) {
                HexCoord newCell = *(((HexCoord*)&adj) + terrOffset);
                if (Map.isValid(newCell) && !terr.contains(newCell) 
                        && Map.at(newCell).getOwner() < 0) {
                    terr.addCell(newCell);
                    Map.at(newCell).setOwner(owner);
                }
            }

            if (--attempts == 0) {
                //  just give up if we can't fill the territory (no biggie?)
                break;
            }
        }

        Map.at(start).setOwner(owner);
        ++owner;
    }

    // color them randomly
    FOREACH (Territory& terr, territories) {
        // Territory& terr = *(territories.begin()); 
        Vec3f randv = Rand::randVec3f();
        ColorA color(abs(randv.x), abs(randv.y), abs(randv.z), 1.0f);

        FOREACH (HexCoord& coord, terr.mCells) {
            Map.at(coord).setColor(color);
        }
    }

    // HexCoord pos = start;
    // for (int c=cities; c > 0; --c) {
    //     if (Map.isValid(pos)) {
    //         HexCell& cell = Map.at(pos);
    //         cell.setColor(Color(0, 0, 0));
    //     }
    //     //  Generate a random offset
    //     Vec2f offset = Rand::randVec2f() * 6.0f;
    //     HexCoord hexOffset(int(offset.x), int(offset.y));
    //     //  offset pos by a random vector
    //     pos += hexOffset;
    // }    
}
