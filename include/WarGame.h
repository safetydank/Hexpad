#pragma once

#include <string>
#include <vector>

#include "Hex.h"

#include "cinder/app/KeyEvent.h"
#include "cinder/app/MouseEvent.h"
#include "cinder/Color.h"
#include "cinder/Vector.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/Camera.h"

#include "GuiController.h"

#include "boost/foreach.hpp"
#define FOREACH BOOST_FOREACH

// XXX network packet types need a home
#include "MessageIdentifiers.h"
enum {
    ID_START_GAME = ID_USER_PACKET_ENUM,
};

namespace war {

class Soldier
{
public:
    Soldier();
    ~Soldier();

private:
    int mLevel;
};

class Player
{
public:
    Player(int id, const std::string& name, ci::Color& color);
    ~Player();

    ci::Color& getColor() { return mObj->mColor; }
    std::string getName() { return mObj->mName; }

private:
    struct Obj {
        Obj(int playerID, const std::string& name, ci::Color color) :
            mPlayerID(playerID), mName(name), mColor(color) { };
        int    mPlayerID;
        std::string mName;
        ci::Color mColor;
    };

    boost::shared_ptr<Obj> mObj;

public:
 	//@{
	//! Emulates shared_ptr-like behavior
	Player( const Player &other ) { mObj = other.mObj; }	
	Player& operator=( const Player &other ) { mObj = other.mObj; return *this; }
	bool operator==( const Player &other ) { return mObj == other.mObj; }
    typedef boost::shared_ptr<Obj> Player::*unspecified_bool_type;
	operator unspecified_bool_type() { return ( mObj.get() == 0 ) ? 0 : &Player::mObj; }
	void reset() { mObj.reset(); }
	//@}	
};

struct Territory
{
    HexCoord mOrigin;
    std::vector<HexCoord> mCells;

    Territory(HexCoord origin) : mOrigin(origin) { }

    void addCell(HexCoord& coord) {
        mCells.push_back(coord);
    }
    bool contains(HexCoord& coord) {
        return (find(mCells.begin(), mCells.end(), coord) != mCells.end());
    }

    HexCoord getOrigin() { return mOrigin; }
};

//  New instance created when a game is started
class WarGame
{
private:
    std::string mName;
    std::vector<Player> mPlayers;
    std::vector<Territory> mTerritories;
    int mTurnPlayer;

public:
    WarGame();
    ~WarGame();

    Player& addPlayer(const std::string& name);
    std::vector<Player>& getPlayers();

    void reset();
    void update();
    void draw();

    // get a reference to the territory list
    std::vector<war::Territory>& getTerritories() { return mTerritories; }
};

typedef enum
{
    UP      = 0,
    PRESSED = 1,
    DOWN    = 2
} ButtonState;

///  Tracks mouse state from incoming events
///  XXX window size member must be updated for window resize to work
class Mouse
{
private:
    ci::Vec2f mWindowSize;
    ci::Vec2f mPos;
    ci::Vec2f mScreenPos;
    
    ButtonState mLeft;
    ButtonState mRight;

public:
    Mouse(ci::Vec2f wsize) : mWindowSize(wsize), mLeft(UP), mRight(UP) { }
    ci::Vec2f getPos() { return mPos; }
    ci::Vec2f getScreenPos() { return mScreenPos; }
    ButtonState getLeft() { return mLeft; }
    ButtonState getRight() { return mRight; }

    float getAspectRatio() { return (mWindowSize.x / mWindowSize.y); }
    ci::Vec2f getWindowSize() { return mWindowSize; }

    void mouseMove(ci::app::MouseEvent event);
    void mouseDown(ci::app::MouseEvent event);
    void mouseUp(ci::app::MouseEvent event);
    void mouseDrag(ci::app::MouseEvent event);
    void mouseWheel(ci::app::MouseEvent event);
};
typedef boost::shared_ptr<Mouse> MousePtr;

class StateManager;

//  Manages network input commands and updates game state (WarGame)
class WargameServer
{
public:
    WargameServer();
    ~WargameServer();

    void update();
    void draw();

private:
    std::string mServerName;
    std::vector<Player> mPlayers;
    int mTurnPlayer;
};
typedef boost::shared_ptr<WargameServer> WargameServerPtr;

//  Manage player input and sends network commands to server
class WargameClient
{
public:
    WargameClient();
    ~WargameClient();

    void update();
    void draw();

private:
    std::string mPlayerName;
    std::vector<Player> mPlayers;
};
typedef boost::shared_ptr<WargameClient> WargameClientPtr;

// shared data between states
struct Shared
{
    HexMap&        hexMap;
    HexGrid&       hexGrid;
    HexRender&     hexRender;
    GuiController& gui;
    GuiFactory&    guiFactory;
    Mouse&         mouse;
    WarGame&       warGame;

    GuiConsolePtr  console;   // XXX have to use a smart ptr here to attach/detach, 
                              // but is there a cyclic dependency between Shared & console?
                              // fix by not using pointers to attach/detach, generate uid's for widgets instead.
    Shared(HexMap& hexmap, HexGrid& hexgrid, HexRender& hexrender, GuiController& gui, GuiFactory& factory, Mouse& mouse, WarGame& wargame, GuiConsolePtr console);
};

}
