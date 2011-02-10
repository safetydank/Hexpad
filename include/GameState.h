#include "WarGame.h"
#include "StateManager.h"
#include "GuiController.h"

namespace war
{

class GameState : public State
{
public:
    GameState(StateManager& manager, Shared& shared);
    ~GameState();

    void enter();
    void leave();
    void update();
    void draw();

    virtual void keyDown(ci::app::KeyEvent event);    
    virtual void mouseWheel(ci::app::MouseEvent event);

private:
    //  Camera drag panning
    bool mDragStart;
    ci::Vec2f mDragOrigin;
    ci::Vec2f mDragEyeOrigin;

    WargameClient mClient;

    //  current player index
    int mPlayer;

    //  GUI 
    std::vector<GuiLabelWidgetPtr> mPlayerLabels;
};

}
