#include "GameState.h"
#include "WarGame.h"
#include "cinder/Vector.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"

#include <string>
#include <vector>

using namespace ci;
using namespace ci::app;
using namespace war;
using std::vector;
using std::string;

#define Game      GG.warGame
#define Gui       GG.gui
#define HexRender GG.hexRender

static Rand random;

GameState::GameState(StateManager& manager, Shared& shared) 
: State(manager, shared), mPlayer(0)
{
}

GameState::~GameState()
{
}

void GameState::enter()
{
    Game.reset();
    vector<string> names;
    names.push_back("Dan");
    names.push_back("Abe");
    names.push_back("Tim");
    names.push_back("Mickey");
    names.push_back("Zen");

    float yoffset = 0;
    int i = 0;
    FOREACH (string name, names) {
        Player& player = Game.addPlayer(name);

        GuiLabelData labelData;
        labelData.FontSize = 15.0f;
        labelData.Text = player.getName();
        // labelData.FgColor = player.getColor();
        GuiLabelWidgetPtr label = Gui.createLabel(labelData, true);
        label->setPos(Vec2f(0, yoffset));
        //  XXX bleah - label has to be updated to refresh size
        label->update();
        yoffset += label->getSize().y;
        mPlayerLabels.push_back(label);
        ++i;
    }

    //  Color territories
    Vec2i mapSize = GG.hexMap.getSize();

    int ownerId=0;
    vector<Territory>& territories = Game.getTerritories();
    int playerCount = Game.getPlayers().size();

    // for (vector<Territory>::iterator it=territories.begin(); it != territories.end(); ++i, ++it) {
    FOREACH (Territory& terr, territories) {
        Player& owner = Game.getPlayers()[ ownerId % playerCount ];
        // Territory& terr = *it;
        Vec3f randv = Rand::randVec3f();

        for (vector<HexCoord>::iterator it = terr.mCells.begin(); 
             it != terr.mCells.end(); ++it) {
            ColorA cellColor(owner.getColor());
            cellColor.a = Rand::randFloat(0.77f, 0.966f);

            GG.hexMap.at(*it).setColor(cellColor);
        }

        ++ownerId;
    }
}

void GameState::leave()
{
    Gui.detachAll();
}

void GameState::update()
{
    Mouse& mouse    = GG.mouse;
    Vec2f  mousePos = mouse.getPos();
    Vec3f  planeHit = GG.hexRender.raycastHexPlane(mousePos.x, mousePos.y);

    HexCoord selectedHex = GG.hexGrid.WorldToHex(planeHit);

    //  XXX copy-pasted from EditorState

    //  Pan the camera
    if (mouse.getRight() == PRESSED) {
        if (!mDragStart) {
            mDragOrigin = mousePos;
            mDragEyeOrigin = Vec2f(GG.hexRender.getCamera().getEyePoint());
            mDragStart = true;
        }

        Vec2f stroke = (mousePos - mDragOrigin) * -48.0f;
        stroke.x *= mouse.getAspectRatio();
        Vec2f dragTo = stroke + mDragEyeOrigin;
        Camera& camera = GG.hexRender.getCamera();
        Vec3f cameraTo(dragTo.x, dragTo.y, camera.getEyePoint().z);

        GG.hexRender.setCameraTo(cameraTo);
    }
    else {
        mDragStart = false;
    }
    
    GG.hexRender.update();
}

void GameState::draw()
{
    gl::clear( Color( 0, 0, 0 ) );
    GG.hexRender.drawHexes();
}

void GameState::keyDown(app::KeyEvent event)
{
    int keycode = event.getCode();

    if (keycode == app::KeyEvent::KEY_ESCAPE) {
        mManager.setActiveState("title");
    }
    else if (keycode == app::KeyEvent::KEY_SPACE) {
        mManager.setActiveState(string("editor"));
    }
}

void GameState::mouseWheel(MouseEvent event)
{
    Vec3f cameraTo = HexRender.getCameraTo();
    cameraTo.z = cameraTo.z - event.getWheelIncrement() * 5.0f;
    HexRender.setCameraTo(cameraTo);
}

