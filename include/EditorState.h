#include "StateManager.h"
#include "GuiController.h"

namespace war
{

class EditorState : public State
{
public:
    EditorState(StateManager& manager, Shared& shared);
    ~EditorState();

    void enter();
    void leave();
    void update();
    void draw();

    void generate();

    virtual void keyDown(ci::app::KeyEvent event);
    virtual void mouseWheel(ci::app::MouseEvent event);

private:
    GuiBoxWidgetPtr mLabelBox;
    GuiLabelWidgetPtr mLabel;

    bool mDragStart;
    ci::Vec2f mDragOrigin;
    ci::Vec2f mDragEyeOrigin;
};

}
