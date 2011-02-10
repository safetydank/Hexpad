#include "WarGame.h"
#include "StateManager.h"

#include "cinder/app/AppBasic.h"

#include <string>
#include <sstream>

using namespace ci;
using namespace ci::app;
using namespace war;

using std::string;
using std::vector;
using boost::shared_ptr;

Player::Player(int id, const string& name, Color& color)
    : mObj(new Obj(id, name, color))
{
}

Player::~Player()
{
}

WarGame::WarGame()
{
}

WarGame::~WarGame()
{
}

Player& WarGame::addPlayer(const string& name)
{
    //  some rgb colors for a grey background (RGB)
    //
    //  139, 8, 22  (red)
    //  15, 188, 33 (green)
    //  48, 30, 195 (blue)
    //  240, 248, 62 (yellow)
    float colors[5][3] = { { 139, 8, 22 },
        { 15, 188, 33 },
        { 48, 30, 195 },
        { 240, 248, 62 },
        { 23, 135, 180 }
    };

    //  convert colors to %
    for (int i=0; i<5; ++i) {
        for (int j=0; j<3; ++j) {
            colors[i][j] = colors[i][j] / 255.0f;
        }
    }

    int playerCount = mPlayers.size();
    float* pcolor = colors[playerCount % 5];
    Player player(playerCount, name, Color(pcolor[0], pcolor[1], pcolor[2]));
    mPlayers.push_back(player);

    return mPlayers.back();
}

vector<Player>& WarGame::getPlayers()
{
    return mPlayers;
}

void WarGame::reset()
{
    mPlayers.clear();
}

HexRender::HexRender(HexMap& map)
    : mHexMap(map), mHexGrid(map.hexGrid())
{
}

HexRender::~HexRender()
{
}

void HexRender::setup(Vec2i wsize)
{
    mWindowSize = wsize;
    mHexGrid.setSpacing(1.5, 1.732050807);

    // gl::enableDepthRead();
    gl::disableDepthRead();
    // gl::enableDepthWrite();
	gl::enableAlphaBlending();

    generateMeshes();

    mCamera.setAspectRatio((float) mWindowSize.x / mWindowSize.y);
	mCamera.lookAt( Vec3f( 0, 0, 30.0f ), Vec3f::zero() );
    mCameraTo = mCamera.getEyePoint();

    gl::Texture::Format format;
    format.setInternalFormat(GL_RGBA_FLOAT32_ATI);
}

void HexRender::generateMeshes()
{
    gl::VboMesh::Layout layout;
    layout.setStaticIndices();
    layout.setStaticPositions();

    // XXX switch to 4-triangle hexes
    mHexMesh = gl::VboMesh(7, 8, layout, GL_TRIANGLE_FAN);

    vector<uint32_t> indices;
    vector<Vec3f>    positions;

    for (int i=0; i < 7; ++i) {
        indices.push_back(i);
        positions.push_back(i == 0 ? Vec3f(0, 0, 0) : Vec3f(float(cos(i*M_PI/3)), float(sin(i*M_PI/3)), 0));
    }
    indices.push_back(1);

    mHexMesh.bufferIndices( indices );
    mHexMesh.bufferPositions( positions );

    mHexOutlineMesh = gl::VboMesh( 7, 6, mHexMesh.getLayout(), GL_LINE_LOOP, NULL, &mHexMesh.getStaticVbo(), NULL );
    indices.clear();
    assert(indices.empty());
    for (int i=1; i < 1+6; ++i) {
        indices.push_back(i);        
    }
    mHexOutlineMesh.bufferIndices( indices );
}

Vec3f HexRender::raycastHexPlane(float u, float v)
{
    // Calculate camera ray intersection with z=0 hexagon plane 
    Ray ray = mCamera.generateRay(u, v, mCamera.getAspectRatio());
    float t = -ray.getOrigin().z / ray.getDirection().z;
    Vec3f planeHit = ray.calcPosition(t);
    return planeHit;
}


void HexRender::update()
{
    //Vec3f planeHit = raycastHexPlane(mMouseLocNorm.x, mMouseLocNorm.y);
    //mSelectedHex = mHexGrid.WorldToHex(planeHit);

    gl::setMatrices(mCamera);
    mTopRight = mHexGrid.WorldToHex(raycastHexPlane(1.0f, 1.0f));
    mBottomLeft = mHexGrid.WorldToHex(raycastHexPlane(0, 0));

    Vec3f dir = (mCameraTo - mCamera.getEyePoint()) * 0.05f;
    Vec3f eyePoint = mCamera.getEyePoint();
    eyePoint += dir;
    mCamera.setEyePoint(eyePoint);
}

void HexRender::drawHexes()
{
    Vec2i mapSize = mHexMap.getSize();

    for (int ix=mBottomLeft.x-1; ix <= mTopRight.x+1; ++ix) {
        for (int iy=mBottomLeft.y-1; iy <= mTopRight.y+1; ++iy) {

            ColorA cellColor;
            HexCoord loc(ix, iy);
            if (!mHexMap.isValid(loc)) {
                // cellColor = ColorA(0.2, 0.2, 0.2);
                continue;
            }
            else {
                cellColor = mHexMap.at(loc).getColor();
            }

            gl::pushMatrices();
            gl::color(cellColor);
            gl::translate(mHexGrid.HexToWorld(HexCoord(ix, iy)));
            gl::draw(mHexMesh);

            // if (mHexMap.isValid(loc)) {
            //     gl::color(ColorA(0, 0, 0, 0.6));
            //     gl::draw(mHexOutlineMesh);
            // }

            gl::popMatrices();
        }
    }
}

void HexRender::setCameraTo(Vec3f& cameraTo)
{
    mCameraTo = cameraTo;
}

Camera& HexRender::getCamera()
{
    return mCamera;
}

void HexRender::drawSelection()
{
    //  Draw highlighted hex
    if (mHexMap.isValid(mSelectedHex)) {
        glLineWidth(3.0f);
        gl::pushMatrices();
        gl::color(ColorA(1.0f, 1.0f, 0, 0.5f + 0.5f * float(abs(sin(2.5*app::getElapsedSeconds())))));
        gl::translate(mHexGrid.HexToWorld(mSelectedHex));
        gl::draw(mHexOutlineMesh);
        gl::popMatrices();
        glLineWidth(1.0f);
    }
}

void Mouse::mouseMove(MouseEvent event)
{
	mScreenPos = Vec2f(float(event.getX()), float(event.getY()));
    mPos = Vec2f(mScreenPos.x / float(mWindowSize.x), 1.0f - (mScreenPos.y / float(mWindowSize.y)));
}

void Mouse::mouseDown(MouseEvent event)
{
    if (event.isLeftDown()) {
        mLeft = (mLeft == PRESSED ? DOWN : PRESSED);
        if (mLeft == PRESSED) {
            // XXX store drag origin
        }
    }
    else if (event.isRightDown()) {
        mRight = (mRight == PRESSED ? DOWN : PRESSED);
            // XXX store drag origin
    }
}

void Mouse::mouseUp(MouseEvent event)
{
    if (event.isLeft()) {
        mLeft = UP;
    }
    else if (event.isRight()) {
        mRight = UP;
    }
}

void Mouse::mouseDrag(ci::app::MouseEvent event)
{
	mScreenPos = Vec2f(float(event.getX()), float(event.getY()));
    mPos = Vec2f(mScreenPos.x / float(mWindowSize.x), 1.0f - (mScreenPos.y / float(mWindowSize.y)));
}

void Mouse::mouseWheel(ci::app::MouseEvent event)
{
}

Shared::Shared(HexMap& hexmap, HexGrid& hexgrid, HexRender& hexrender, GuiController& gui, GuiFactory& factory, Mouse& mouse, WarGame& wargame, GuiConsolePtr console)
    : hexMap(hexmap), hexGrid(hexgrid), hexRender(hexrender), gui(gui), guiFactory(factory), mouse(mouse), warGame(wargame), console(console)
{
}


WargameServer::WargameServer()
{
}

WargameServer::~WargameServer()
{
}

WargameClient::WargameClient()
{
}

WargameClient::~WargameClient()
{
}
