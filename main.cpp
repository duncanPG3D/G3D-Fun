/**
  @file demos/main.cpp

  This is a sample main.cpp to get you started with G3D.  It is
  designed to make writing an application easy.  Although the
  GApp/GApplet infrastructure is helpful for most projects,
  you are not restricted to using it-- choose the level of
  support that is best for your project (see the G3D Map in the
  documentation).

  @author Morgan McGuire, matrix@graphics3d.com
 */

// TODO: Move toolbar buttons with resized window.

#define _WIN32_WINNT 0x0400

#include <G3DAll.h>
#include <initguid.h>
#include <iomanip>
#include "Instance.h"
#include "resource.h"
#include "PhysicalInstance.h"
#include "TextButtonInstance.h"
#include "ImageButtonInstance.h"
#include "DataModelInstance.h"
#include "CameraController.h"
#include "AudioPlayer.h"
#include "Globals.h"
#include "Demo.h"
#include "win32Defines.h"
#include "WindowFunctions.h"
#include <limits.h>
#include <mshtml.h>
#include <exdisp.h>
#include <vector>
#include <string>
#include "ax.h"
#include <cguid.h>
#include "IEBrowser.h"

#if G3D_VER < 61000
	#error Requires G3D 6.10
#endif
HWND hwnd;

DEFINE_GUID(CLSID_G3d, 0xB323F8E0L, 0x2E68, 0x11D0, 0x90, 0xEA, 0x00, 0xAA, 0x00, 0x60, 0xF8, 0x6F);
HRESULT hresult;
OLECHAR dat = ((OLECHAR)"SayHello");
OLECHAR * szMember = &dat;
DISPID dispid;
DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
EXCEPINFO excepinfo;
UINT nArgErr;

static std::string title = "";
static DataModelInstance* dataModel;
GFontRef fntdominant = NULL;
GFontRef fntlighttrek = NULL;
Ray testRay;
static std::string message = "";
static G3D::RealTime messageTime = 0;
static std::string tempPath = "";
static std::string cameraSound = "";
static std::string clickSound = "";
static std::string dingSound = "";
static int cursorid = 0;
static G3D::TextureRef cursor = NULL;
static bool running = true;
static bool mouseMovedBeginMotion = false;
static const int CURSOR = 0;
static const int ARROWS = 1;
static const int RESIZE = 2;
static POINT oldGlobalMouse;
static int mode = CURSOR;
bool dragging = false;
Vector2 oldMouse = Vector2(0,0);
float moveRate = 0.5;
Instance* selectedInstance = NULL;
static const std::string PlaceholderName = "Dynamica";

Demo *usableApp = NULL;

Demo::Demo(const GAppSettings& settings,HWND parentWindow) { //: GApp(settings,window) {
	_hWndMain = parentWindow;

	HMODULE hThisInstance = GetModuleHandle(NULL);

	_hwndToolbox = CreateWindowEx(
		WS_EX_ACCEPTFILES,
		"AX",
		"{8856F961-340A-11D0-A96B-00C04FD705A2}",
		WS_CHILD | WS_VISIBLE,
		0,
		560,
		800,
		60,
		_hWndMain, // parent
		NULL, // menu
		hThisInstance,
		NULL
	);
	
	_hwndRenderer = CreateWindowEx(
		WS_EX_ACCEPTFILES,
		"G3DWindow",
		"3D",
		WS_CHILD,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1,
		1,
		_hWndMain, // parent
		NULL, // menu
		hThisInstance,
		NULL
	);

	Win32Window* window = Win32Window::create(settings.window,_hwndRenderer);
	ShowWindow(_hwndRenderer, SW_SHOW);
	ShowWindow(_hWndMain, SW_SHOW);

	quit=false;
	rightButtonHolding=false;
	mouseOnScreen=false;
	// GApp replacement
	renderDevice = new RenderDevice();
	if (window != NULL) {
	renderDevice->init(window, NULL);
	}
	else
	{
		MessageBox(NULL,"Window not found!","Error",MB_OK);
		return;
	}

    _window = renderDevice->window();
    _window->makeCurrent();

	SetWindowLongPtr(_hWndMain,GWL_USERDATA,(LONG)this);
	SetWindowLongPtr(_hwndRenderer,GWL_USERDATA,(LONG)this);

	IEBrowser* webBrowser = new IEBrowser(_hwndToolbox);
	webBrowser->navigateSyncURL(L"http://scottbeebiwan.tk/g3d/toolbox/");
}

void clearInstances()
{
	delete dataModel;
}

void OnError(int err, std::string msg = "")
{
	//usableApp->window()->setInputCaptureCount(0);
	//usableApp->window()->setMouseVisible(true);
	std::string emsg = "An unexpected error has occured and "+PlaceholderName+" has to quit. We're sorry!" + msg;
	std::string title = PlaceholderName+"Crash";
	clearInstances();
	MessageBox(NULL, emsg.c_str(), title.c_str(), MB_OK);
	exit(err);
}

std::string ExePath() {
    char buffer[MAX_PATH];
	GetModuleFileName( NULL, buffer, MAX_PATH );
	std::string::size_type pos = std::string( buffer ).find_last_of( "\\/" );
	return std::string( buffer ).substr( 0, pos);
}

std::string GetFileInPath(std::string file)
{
	std::string name = ExePath() + file;
	struct stat buf;
    if (stat(name.c_str(), &buf) != -1)
    {
        return name;
    }
	else
		OnError(202, " \r\nFile not found: " + name);
		return NULL;
		
	
}

#include <sstream>
std::string Convert (float number){
    std::ostringstream buff;
    buff<<number;
    return buff.str();   
}


PhysicalInstance* makePart()
{
	PhysicalInstance* part = new PhysicalInstance();
	return part;
}

TextButtonInstance* makeTextButton()
{
	TextButtonInstance* part = new TextButtonInstance();
	return part;
}

ImageButtonInstance* makeImageButton(G3D::TextureRef newImage = NULL, G3D::TextureRef overImage = NULL, G3D::TextureRef downImage = NULL, G3D::TextureRef disableImage = NULL)
{
	ImageButtonInstance* part = new ImageButtonInstance(newImage,overImage, downImage, disableImage);
//	instances.push_back(part);
//	instances_2D.push_back(part);
	return part;
}



class CameraButtonListener : public ButtonListener {
public:
	void onButton1MouseClick(BaseButtonInstance*);
};

void CameraButtonListener::onButton1MouseClick(BaseButtonInstance* button)
{
	AudioPlayer::playSound(cameraSound);
	CoordinateFrame frame = usableApp->cameraController.getCamera()->getCoordinateFrame();
	if(button->name == "CenterCam")
		usableApp->cameraController.centerCamera(selectedInstance);
	else if(button->name == "ZoomIn")
		usableApp->cameraController.Zoom(1);
	else if(button->name == "ZoomOut")
		usableApp->cameraController.Zoom(-1);
	else if(button->name == "PanRight")
		usableApp->cameraController.panRight();
	else if(button->name == "PanLeft")
		usableApp->cameraController.panLeft();
	else if(button->name == "TiltUp")
		usableApp->cameraController.tiltUp();
	else if(button->name == "TiltDown")
		usableApp->cameraController.tiltDown();
}

class GUDButtonListener : public ButtonListener {
public:
	void onButton1MouseClick(BaseButtonInstance*);
};

void GUDButtonListener::onButton1MouseClick(BaseButtonInstance* button)
{
	if(selectedInstance != NULL)
	{
		AudioPlayer::playSound(dingSound);
		if(button->name == "Duplicate")
		{

		}
	}
		
}

class RotateButtonListener : public ButtonListener {
public:
	void onButton1MouseClick(BaseButtonInstance*);
};

void RotateButtonListener::onButton1MouseClick(BaseButtonInstance* button)
{
	if(selectedInstance != NULL)
	{
		AudioPlayer::playSound(clickSound);
		if(selectedInstance->getClassName() == "Part")
		{
			PhysicalInstance* part = (PhysicalInstance*) selectedInstance;
			if(button->name == "Tilt")
				part->setCFrame(part->getCFrame()*Matrix3::fromEulerAnglesXYZ(0,0,toRadians(90)));
			else if(button->name == "Rotate")
				part->setCFrame(part->getCFrame()*Matrix3::fromEulerAnglesXYZ(0,toRadians(90),0));
		}
	}
		
}


void deleteInstance()
{
	if(selectedInstance != NULL)
	{
		if(selectedInstance->getParent() != NULL)
			selectedInstance->getParent()->removeChild(selectedInstance);
		delete selectedInstance;
		selectedInstance = NULL;
		AudioPlayer::playSound(GetFileInPath("/content/sounds/pageturn.wav"));
	}
}


class DeleteListener : public ButtonListener {
public:
	void onButton1MouseClick(BaseButtonInstance*);
};

void DeleteListener::onButton1MouseClick(BaseButtonInstance* button)
{
	deleteInstance();
}




class ModeSelectionListener : public ButtonListener {
public:
	void onButton1MouseClick(BaseButtonInstance*);
};


void ModeSelectionListener::onButton1MouseClick(BaseButtonInstance* button)
{
	//CoordinateFrame frame = usableApp->g3dCamera.getCoordinateFrame();
	
	std::vector<Instance*> instances_2D = dataModel->getGuiRoot()->getAllChildren();
	for(size_t i = 0; i < instances_2D.size(); i++)
		{
			if(instances_2D.at(i)->name == "Cursor" || instances_2D.at(i)->name == "Resize" || instances_2D.at(i)->name == "Arrows")
			{
				BaseButtonInstance* button = (BaseButtonInstance*)instances_2D.at(i);
				button->selected = false;
			}
		}

	button->selected = true;
	if(button->name == "Cursor")
		mode = CURSOR;
	else if(button->name == "Resize")
		mode = RESIZE;
	else if(button->name == "Arrows")
		mode = ARROWS;
}


void Demo::initGUI()
{
	TextButtonInstance* button = makeTextButton();
	button->boxBegin = Vector2(0, -24);
	button->boxEnd = Vector2(80, 0);
	button->floatBottom = true;
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3(0,255,255);
	button->textOutlineColor = Color4::clear();
	button->title = "Hopper";
	button->fontLocationRelativeTo = Vector2(10, 3);
	button->setAllColorsSame();
	
	button = makeTextButton();
	button->boxBegin = Vector2(0, -48);
	button->boxEnd = Vector2(80, -24);
	button->floatBottom = true;
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3(0,255,255);
	button->textOutlineColor = Color4::clear();
	button->title = "Controller";
	button->fontLocationRelativeTo = Vector2(10, 3);
	button->setAllColorsSame();

	button = makeTextButton();
	button->boxBegin = Vector2(0, -72);
	button->boxEnd = Vector2(80, -48);
	button->floatBottom = true;
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3(0,255,255);
	button->textOutlineColor = Color4::clear();
	button->title = "Color";
	button->fontLocationRelativeTo = Vector2(10, 3);
	button->setAllColorsSame();

	button = makeTextButton();
	button->boxBegin = Vector2(0, -96);
	button->boxEnd = Vector2(80, -72);
	button->floatBottom = true;
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3(0,255,255);
	button->textOutlineColor = Color4::clear();
	button->title = "Surface";
	button->fontLocationRelativeTo = Vector2(10, 3);
	button->setAllColorsSame();

	button = makeTextButton();
	button->boxBegin = Vector2(0, -120);
	button->boxEnd = Vector2(80, -96);
	button->floatBottom = true;
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3(0,255,255);
	button->boxOutlineColor = Color3(0,255,255);
	button->title = "Model";
	button->fontLocationRelativeTo = Vector2(10, 3);
	button->setAllColorsSame();

	button = makeTextButton();
	button->boxBegin = Vector2(0, 0);
	button->boxEnd = Vector2(125, 25);
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3::white();
	button->boxColor = Color4::clear();
	button->textOutlineColor = Color4(0.5F,0.5F,0.5F,0.5F);
	button->title = "File";
	button->textSize = 16;
	button->fontLocationRelativeTo = Vector2(10, 0);
	button->setAllColorsSame();

	button = makeTextButton();
	button->boxBegin = Vector2(125, 0);
	button->boxEnd = Vector2(250, 25);
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3::white();
	button->boxColor = Color4::clear();
	button->textOutlineColor = Color4(0.5F,0.5F,0.5F,0.5F);
	button->title = "Edit";
	button->textSize = 16;
	button->fontLocationRelativeTo = Vector2(10, 0);
	button->setAllColorsSame();

	button = makeTextButton();
	button->boxBegin = Vector2(250, 0);
	button->boxEnd = Vector2(375, 25);
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3::white();
	button->boxColor = Color4::clear();
	button->textOutlineColor = Color4(0.5F,0.5F,0.5F,0.5F);
	button->title = "View";
	button->textSize = 16;
	button->fontLocationRelativeTo = Vector2(10, 0);
	button->setAllColorsSame();

	button = makeTextButton();
	button->boxBegin = Vector2(375, 0);
	button->boxEnd = Vector2(500, 25);
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3::white();
	button->boxColor = Color4::clear();
	button->textOutlineColor = Color4(0.5F,0.5F,0.5F,0.5F);
	button->title = "Insert";
	button->textSize = 16;
	button->fontLocationRelativeTo = Vector2(10, 0);
	button->setAllColorsSame();

	button = makeTextButton();
	button->boxBegin = Vector2(500, 0);
	button->boxEnd = Vector2(625, 25);
	button->setParent(dataModel->getGuiRoot());
	button->font = fntlighttrek;
	button->textColor = Color3::white();
	button->boxColor = Color4::clear();
	button->textOutlineColor = Color4(0.5F,0.5F,0.5F,0.5F);
	button->title = "Format";
	button->textSize = 16;
	button->fontLocationRelativeTo = Vector2(10, 0);
	button->setAllColorsSame();



	button = makeTextButton();
	button->boxBegin = Vector2(0,215);
	button->boxEnd = Vector2(80,235);
	button->textOutlineColor = Color4(0.5F,0.5F,0.5F,0.5F);
	button->textColor = Color3::white();
	button->boxColor = Color4::clear();
	button->textSize = 12;
	button->title = "Group";
	button->setAllColorsSame();
	button->font = fntlighttrek;
	button->fontLocationRelativeTo = Vector2(10, 0);
	button->setParent(dataModel->getGuiRoot());


	button = makeTextButton();
	button->boxBegin = Vector2(0,240);
	button->boxEnd = Vector2(80,260);
	button->textOutlineColor = Color4(0.5F,0.5F,0.5F,0.5F);
	button->textColor = Color3::white();
	button->boxColor = Color4::clear();
	button->textSize = 12;
	button->title = "UnGroup";
	button->setAllColorsSame();
	button->font = fntlighttrek;
	button->fontLocationRelativeTo = Vector2(10, 0);
	button->setParent(dataModel->getGuiRoot());

	button = makeTextButton();
	button->boxBegin = Vector2(0,265);
	button->boxEnd = Vector2(80,285);
	button->textOutlineColor = Color4(0.5F,0.5F,0.5F,0.5F);
	button->textColor = Color3::white();
	button->boxColor = Color4::clear();
	button->textSize = 12;
	button->title = "Duplicate";
	button->setAllColorsSame();
	button->font = fntlighttrek;
	button->fontLocationRelativeTo = Vector2(10, 0);
	button->setParent(dataModel->getGuiRoot());
	button->name = "Duplicate";
	button->setButtonListener(new GUDButtonListener());

	ImageButtonInstance* instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/Run.png")),
		Texture::fromFile(GetFileInPath("/content/images/Run_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/Run_dn.png")));
	instance->name = "go";
	instance->size = Vector2(65,65);
	instance->position = Vector2(6.5, 25);
	instance->setParent(dataModel->getGuiRoot());


	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/ArrowTool.png")),
		Texture::fromFile(GetFileInPath("/content/images/ArrowTool_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/ArrowTool_dn.png")),
		Texture::fromFile(GetFileInPath("/content/images/ArrowTool_ds.png")));
	instance->size = Vector2(50,50);
	instance->position = Vector2(15, 90);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "Cursor";
	instance->setButtonListener(new ModeSelectionListener());

	instance = makeImageButton(Texture::fromFile(GetFileInPath("/content/images/ScaleTool.png")),Texture::fromFile(GetFileInPath("/content/images/ScaleTool_ovr.png")),Texture::fromFile(GetFileInPath("/content/images/ScaleTool_dn.png")),Texture::fromFile(GetFileInPath("/content/images/ScaleTool_ds.png")));
	instance->size = Vector2(40,40);
	instance->position = Vector2(0, 140);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "Resize";
	instance->setButtonListener(new ModeSelectionListener());
	

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/MoveTool.png")),
		Texture::fromFile(GetFileInPath("/content/images/MoveTool_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/MoveTool_dn.png")),
		Texture::fromFile(GetFileInPath("/content/images/MoveTool_ds.png")));
	instance->size = Vector2(40,40);
	instance->position = Vector2(40, 140);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "Arrows";
	instance->setButtonListener(new ModeSelectionListener());

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/SelectionRotate.png")),
		Texture::fromFile(GetFileInPath("/content/images/SelectionRotate_ovr.png")),
		NULL,
		Texture::fromFile(GetFileInPath("/content/images/SelectionRotate_ds.png")));
	instance->size = Vector2(30,30);
	instance->position = Vector2(10, 175);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "Rotate";
	instance->setButtonListener(new RotateButtonListener());

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/SelectionTilt.png")),
		Texture::fromFile(GetFileInPath("/content/images/SelectionTilt_ovr.png")),
		NULL,
		Texture::fromFile(GetFileInPath("/content/images/SelectionTilt_ds.png")));
	instance->size = Vector2(30,30);
	instance->position = Vector2(40, 175);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "Tilt";
	instance->setButtonListener(new RotateButtonListener());


	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/Delete.png")),
		Texture::fromFile(GetFileInPath("/content/images/Delete_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/Delete_dn.png")),
		Texture::fromFile(GetFileInPath("/content/images/Delete_ds.png")));
	instance->size = Vector2(40,46);
	instance->position = Vector2(20, 284);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "Delete";
	instance->setButtonListener(new DeleteListener());

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/CameraZoomIn.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraZoomIn_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraZoomIn_dn.png")));
	instance->size = Vector2(34,25);
	instance->floatBottom = true;
	instance->floatRight = true;
	instance->position = Vector2(-77, -90);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "ZoomIn";
	instance->setButtonListener(new CameraButtonListener());

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/CameraZoomOut.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraZoomOut_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraZoomOut_dn.png")));
	instance->size = Vector2(34,26);
	instance->floatBottom = true;
	instance->floatRight = true;
	instance->position = Vector2(-77, -31);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "ZoomOut";
	instance->setButtonListener(new CameraButtonListener());

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/CameraPanLeft.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraPanLeft_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraPanLeft_dn.png")));
	instance->size = Vector2(34,34);
	instance->floatBottom = true;
	instance->floatRight = true;
	instance->position = Vector2(-110, -50);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "PanLeft";
	instance->setButtonListener(new CameraButtonListener());

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/CameraPanRight.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraPanRight_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraPanRight_dn.png")));
	instance->size = Vector2(34,34);
	instance->floatBottom = true;
	instance->floatRight = true;
	instance->position = Vector2(-45, -50);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "PanRight";
	instance->setButtonListener(new CameraButtonListener());

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/CameraCenter.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraCenter_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraCenter_dn.png")));
	instance->size = Vector2(34,20);
	instance->floatBottom = true;
	instance->floatRight = true;
	instance->position = Vector2(-77, -60);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "CenterCam";
	instance->setButtonListener(new CameraButtonListener());

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/CameraTiltUp.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraTiltUp_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraTiltUp_dn.png")));
	instance->size = Vector2(24,24);
	instance->floatBottom = true;
	instance->floatRight = true;
	instance->position = Vector2(-105, -75);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "TiltUp";
	instance->setButtonListener(new CameraButtonListener());

	instance = makeImageButton(
		Texture::fromFile(GetFileInPath("/content/images/CameraTiltDown.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraTiltDown_ovr.png")),
		Texture::fromFile(GetFileInPath("/content/images/CameraTiltDown_dn.png")));
	instance->size = Vector2(24,24);
	instance->floatBottom = true;
	instance->floatRight = true;
	instance->position = Vector2(-40, -75);
	instance->setParent(dataModel->getGuiRoot());
	instance->name = "TiltDown";
	instance->setButtonListener(new CameraButtonListener());
}


void Demo::onInit()  {
	
    // Called before Demo::run() beings
	cameraController.setFrame(Vector3(0,2,10));
	dataModel = new DataModelInstance();
	dataModel->setParent(NULL);
	dataModel->name = "undefined";
	dataModel->font = fntdominant;
	Globals::dataModel = dataModel;
	
	initGUI();

	
	PhysicalInstance* test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3(0.2F,0.3F,1);
	test->setSize(Vector3(24,1,24));
	test->setPosition(Vector3(0,0,0));
	test->setCFrame(test->getCFrame() * Matrix3::fromEulerAnglesXYZ(0,toRadians(40),toRadians(40)));
	

	
	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3(.5F,1,.5F);
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(-10,1,0));
	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3(.5F,1,.5F);
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(10,1,0));

	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3::gray();
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(7,2,0));

	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3::gray();
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(-7,2,0));

	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3::gray();
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(4,3,0));

	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3::gray();
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(-5,3,0));

	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3::gray();
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(1,4,0));

	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3::gray();
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(-3,4,0));

	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3::gray();
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(-2,5,0));
	

	

	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3::gray();
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(0,6,0));

	test = makePart();
	test->setParent(dataModel->getWorkspace());
	test->color = Color3::gray();
	test->setSize(Vector3(4,1,2));
	test->setPosition(Vector3(2,7,0));

	



	//setDesiredFrameRate(60);
	
	
	

    //GApplet::onInit();
	
}






void Demo::onCleanup() {
    clearInstances();
	sky->~Sky();
}




void Demo::onLogic() {
    // Add non-simulation game logic and AI code here
	
	Instance* obj = dataModel->getGuiRoot()->findFirstChild("Delete");
		if(obj != NULL)
		{
			ImageButtonInstance* button = (ImageButtonInstance*)obj;
			if(selectedInstance == NULL)
				button->disabled = true;
			else
				button->disabled = false;	
		}


}


void Demo::onNetwork() {
	// Poll net messages here
}


//double getVectorDistance(Vector3 vector1, Vector3 vector2)
//{
//	return pow(pow((double)vector1.x - (double)vector2.x, 2) + pow((double)vector1.y - (double)vector2.y, 2) + pow((double)vector1.z - (double)vector2.z, 2), 0.5);
//}

Instance* Demo::getSelection()
{
	return selectedInstance;
}
void Demo::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	if(dataModel->name != title)
	{
		title = dataModel->name;
		std::string text = "Game \"" + title + "\"";
		SetWindowText(_hWndMain, text.c_str());
		
	}

	cameraController.update(this);

}

/*double getOSVersion() {
    OSVERSIONINFO osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);
	std::string version = Convert(osvi.dwMajorVersion) + "." + Convert(osvi.dwMinorVersion);
	return ::atof(version.c_str());
}*/

bool IsHolding(int button)
{
	return (GetKeyState(button) >> 1)>0;
}

BOOL GetKPBool(int VK) {
	return (GetKeyState(VK) & 0x8000);
}

void Demo::onUserInput(UserInput* ui) {

	/*
	if(GetHoldKeyState(VK_LCONTROL))
	{
		if(GetHoldKeyState('D'))
		{
			messageTime = System::time();
			if(debugMode())
				message = "Debug Mode Disabled";
			else
				message = "Debug Mode Enabled";
			setDebugMode(!debugMode());
		}
	}
	*/
	if(GetHoldKeyState(VK_F8))
	{
		messageTime = System::time();
		message = "FOV Set to 10";
		
	}
	//}

	//dataModel->mousex = ui->getMouseX();
	//dataModel->mousey = ui->getMouseY();
	dataModel->mouseButton1Down = (GetKeyState(VK_LBUTTON) & 0x100) != 0;

	if (GetHoldKeyState(VK_LBUTTON)) {
		if (dragging) {
		PhysicalInstance* part = (PhysicalInstance*) selectedInstance;
		Ray dragRay = cameraController.getCamera()->worldRay(dataModel->mousex, dataModel->mousey, renderDevice->getViewport());
		std::vector<Instance*> instances = dataModel->getWorkspace()->getAllChildren();
		for(size_t i = 0; i < instances.size(); i++)
			{
				if(instances.at(i)->getClassName() == "Part")
				{
					PhysicalInstance* moveTo = (PhysicalInstance*)instances.at(i);
					float __time = testRay.intersectionTime(moveTo->getBox());
					float __nearest=std::numeric_limits<float>::infinity();
					if (__time != inf()) 
					{
						if (__nearest>__time)
						{
							Vector3 closest = (dragRay.closestPoint(moveTo->getPosition()) * 2);
							part->setPosition(closest);
							//part->setPosition(Vector3(floor(closest.x),part->getPosition().y,floor(closest.z)));
						}
					}
				}
			}
			Sleep(10);
		}
	}
	// Camera KB Handling {
		if (GetKPBool(VK_OEM_COMMA)) //Left
			usableApp->cameraController.panLeft();
		else if (GetKPBool(VK_OEM_PERIOD)) // Right
			usableApp->cameraController.panRight();
		else if (GetKPBool(0x49)) // Zoom In (I)
			usableApp->cameraController.Zoom(1);
		else if (GetKPBool(0x4F)) // Zoom Out (O)
			usableApp->cameraController.Zoom(-1);
	// }

	//readMouseGUIInput();
	// Add other key handling here
}

void makeFlag(Vector3 &vec, RenderDevice* &rd)
{

	Vector3 up = Vector3(vec.x, vec.y+3, vec.z);
	rd->setColor(Color3::blue());
	rd->beforePrimitive();

		glBegin(GL_LINES);
		glVertex3f(vec.x, vec.y, vec.z);
		glVertex3f(up.x, up.y, up.z);
		glEnd();

		glBegin( GL_TRIANGLES );
		glVertex3f(up.x, up.y-1, up.z);
		glVertex3f(up.x, up.y-0.5, up.z-1);
		glVertex3f(up.x, up.y, up.z);
		
		glVertex3f(up.x, up.y, up.z);
		glVertex3f(up.x, up.y-0.5, up.z-1);
		glVertex3f(up.x, up.y-1, up.z);

		glEnd();
	rd->afterPrimitive();
	rd->setColor(Color3::white());
	//I know how i will approach this now
}



bool mouseInArea(float point1x, float point1y, float point2x, float point2y)
{
	

	if(dataModel->mousex >= point1x && dataModel->mousey >= point1y)
	{
		if(dataModel->mousex < point2x && dataModel->mousey < point2y)
		{
			return true;
		}
	}
	return false;
}


void drawButtons(RenderDevice* rd)
{
	rd->pushState();
	rd->beforePrimitive();
			dataModel->getGuiRoot()->render(rd);
	rd->afterPrimitive();
	rd->popState();
}

void drawOutline(Vector3 from, Vector3 to, RenderDevice* rd, LightingParameters lighting, Vector3 size, Vector3 pos, CoordinateFrame c)
{

	Color3 outline = Color3::cyan();//Color3(0.098F,0.6F,1.0F);
	float offsetSize = 0.05F;
	//X
	Draw::box(c.toWorldSpace(Box(Vector3(from.x - offsetSize, from.y + offsetSize, from.z + offsetSize), Vector3(to.x + offsetSize, from.y - offsetSize, from.z - offsetSize))), rd, outline, Color4::clear()); 
	Draw::box(c.toWorldSpace(Box(Vector3(from.x - offsetSize, to.y + offsetSize, from.z + offsetSize), Vector3(to.x + offsetSize, to.y - offsetSize, from.z - offsetSize))), rd, outline, Color4::clear()); 
	Draw::box(c.toWorldSpace(Box(Vector3(from.x - offsetSize, to.y + offsetSize, to.z + offsetSize), Vector3(to.x + offsetSize, to.y - offsetSize, to.z - offsetSize))), rd, outline, Color4::clear()); 
	Draw::box(c.toWorldSpace(Box(Vector3(from.x - offsetSize, from.y + offsetSize, to.z + offsetSize), Vector3(to.x + offsetSize, from.y - offsetSize, to.z - offsetSize))), rd, outline, Color4::clear()); 
	//Y
	Draw::box(c.toWorldSpace(Box(Vector3(from.x + offsetSize, from.y - offsetSize + 0.1, from.z + offsetSize), Vector3(from.x - offsetSize, to.y + offsetSize - 0.1, from.z - offsetSize))), rd, outline, Color4::clear()); 
	Draw::box(c.toWorldSpace(Box(Vector3(to.x + offsetSize, from.y - offsetSize + 0.1, from.z + offsetSize), Vector3(to.x - offsetSize, to.y + offsetSize - 0.1, from.z - offsetSize))), rd, outline, Color4::clear()); 
	Draw::box(c.toWorldSpace(Box(Vector3(to.x + offsetSize, from.y - offsetSize + 0.1, to.z + offsetSize), Vector3(to.x - offsetSize, to.y + offsetSize-0.1, to.z - offsetSize))), rd, outline, Color4::clear()); 
	Draw::box(c.toWorldSpace(Box(Vector3(from.x + offsetSize, from.y - offsetSize + 0.1, to.z + offsetSize), Vector3(from.x - offsetSize, to.y + offsetSize - 0.1, to.z - offsetSize))), rd, outline, Color4::clear()); 

	//Z
	Draw::box(c.toWorldSpace(Box(Vector3(from.x + offsetSize, from.y + offsetSize, from.z - offsetSize), Vector3(from.x - offsetSize, from.y - offsetSize, to.z + offsetSize))), rd, outline, Color4::clear()); 
	Draw::box(c.toWorldSpace(Box(Vector3(from.x + offsetSize, to.y + offsetSize, from.z - offsetSize), Vector3(from.x - offsetSize, to.y - offsetSize, to.z + offsetSize))), rd, outline, Color4::clear()); 
	Draw::box(c.toWorldSpace(Box(Vector3(to.x + offsetSize, from.y + offsetSize, from.z - offsetSize), Vector3(to.x - offsetSize, from.y - offsetSize, to.z + offsetSize))), rd, outline, Color4::clear()); 
	Draw::box(c.toWorldSpace(Box(Vector3(to.x + offsetSize, to.y + offsetSize, from.z - offsetSize), Vector3(to.x - offsetSize, to.y - offsetSize, to.z + offsetSize))), rd, outline, Color4::clear()); 
	
	if(mode == ARROWS)
	{
		rd->setLight(0, NULL);
		rd->setAmbientLightColor(Color3(1,1,1));
		
		AABox box;
		c.toWorldSpace(Box(from, to)).getBounds(box);
		float max = box.high().y - pos.y;

		Draw::arrow(pos, Vector3(0, 1.5+max, 0), rd);
		Draw::arrow(pos, Vector3(0, (-1.5)-max, 0), rd);
		
		max = box.high().x - pos.x;

		Draw::arrow(pos, Vector3(1.5+max, 0, 0), rd);
		Draw::arrow(pos, Vector3((-1.5)-max, 0, 0), rd);

		max = box.high().z - pos.z;

		Draw::arrow(pos, Vector3(0, 0, 1.5+max), rd);
		Draw::arrow(pos, Vector3(0, 0, (-1.5)-max), rd);



		rd->setAmbientLightColor(lighting.ambient);
		rd->setLight(0, GLight::directional(lighting.lightDirection, lighting.lightColor));
	}
	else if(mode == RESIZE)
	{
		Color3 sphereColor = outline;
		rd->setLight(0, NULL);
		rd->setAmbientLightColor(Color3(1,1,1));
		Vector3 gamepoint = pos;
		Vector3 camerapoint = rd->getCameraToWorldMatrix().translation;
		float distance = pow(pow((double)gamepoint.x - (double)camerapoint.x, 2) + pow((double)gamepoint.y - (double)camerapoint.y, 2) + pow((double)gamepoint.z - (double)camerapoint.z, 2), 0.5);
		if(distance < 200)
		{
			
			float multiplier = distance * 0.025F/2;
			if(multiplier < 0.25F)
				multiplier = 0.25F;
			Vector3 position = pos + (c.lookVector()*((size.z/2)+1));
			Draw::sphere(Sphere(position, multiplier), rd, sphereColor, Color4::clear());
			position = pos - (c.lookVector()*((size.z/2)+1));
			Draw::sphere(Sphere(position, multiplier), rd, sphereColor, Color4::clear());

			position = pos + (c.rightVector()*((size.x/2)+1));
			Draw::sphere(Sphere(position, multiplier), rd, sphereColor, Color4::clear());
			position = pos - (c.rightVector()*((size.x/2)+1));
			Draw::sphere(Sphere(position, multiplier), rd, sphereColor, Color4::clear());

			position = pos + (c.upVector()*((size.y/2)+1));
			Draw::sphere(Sphere(position, multiplier), rd, sphereColor, Color4::clear());
			position = pos - (c.upVector()*((size.y/2)+1));
			Draw::sphere(Sphere(position, multiplier), rd, sphereColor, Color4::clear());
		}
		rd->setAmbientLightColor(lighting.ambient);
		rd->setLight(0, GLight::directional(lighting.lightDirection, lighting.lightColor));
	}
	
}

void Demo::exitApplication()
{
	//endApplet = true;
    //endProgram = true;
}


void Demo::onGraphics(RenderDevice* rd) {
	
	G3D::uint8 num = 0;
	POINT mousepos;
	mouseOnScreen = true;
	if (GetCursorPos(&mousepos))
	{
	if (ScreenToClient(_hWndMain, &mousepos))
	{
		//mouseOnScreen = true;
		
		if(mousepos.x < 1 || mousepos.y < 1 || mousepos.x >= rd->getViewport().width()-1 || mousepos.y >= rd->getViewport().height()-1)
		{
			mouseOnScreen = false;
			//ShowCursor(true);
			_window->setMouseVisible(true);
			//rd->window()->setInputCaptureCount(0);
		}
		else
		{
			mouseOnScreen = true;
			//SetCursor(NULL);
			_window->setMouseVisible(false);
			//rd->window()->setInputCaptureCount(1);
		}
		
	}
	}
	
	if(Globals::useMousePoint)
	{
		mousepos = Globals::mousepoint;
		ScreenToClient(_hWndMain, &mousepos);
	}
	
    LightingParameters lighting(G3D::toSeconds(11, 00, 00, AM));
    renderDevice->setProjectionAndCameraMatrix(*cameraController.getCamera());
	
    // Cyan background
    //renderDevice->setColorClearValue(Color3(0.0f, 0.5f, 1.0f));

    renderDevice->clear(sky.isNull(), true, true);
    if (sky.notNull()) {
        sky->render(renderDevice, lighting);
    }
	
    // Setup lighting
    renderDevice->enableLighting();

	renderDevice->setShadeMode(RenderDevice::SHADE_SMOOTH);
	renderDevice->setAmbientLightColor(Color3(1,1,1));

	renderDevice->setLight(0, GLight::directional(lighting.lightDirection, lighting.lightColor));
	renderDevice->setAmbientLightColor(lighting.ambient);
	
	rd->beforePrimitive();
	dataModel->getWorkspace()->render(rd);
	rd->afterPrimitive();

	if(selectedInstance != NULL)
	{
		PhysicalInstance* part = (PhysicalInstance*)selectedInstance;
		Vector3 size = part->getSize();
		Vector3 pos = part->getPosition();
		drawOutline(Vector3(0+size.x/4, 0+size.y/4, 0+size.z/4) ,Vector3(0-size.x/4,0-size.y/4,0-size.z/4), rd, lighting, Vector3(size.x/2, size.y/2, size.z/2), Vector3(pos.x/2, pos.y/2, pos.z/2), part->getCFrameRenderBased());
	}
	

	//Vector3 gamepoint = Vector3(0, 5, 0);
	//Vector3 camerapoint = rd->getCameraToWorldMatrix().translation;
	//float distance = pow(pow((double)gamepoint.x - (double)camerapoint.x, 2) + pow((double)gamepoint.y - (double)camerapoint.y, 2) + pow((double)gamepoint.z - (double)camerapoint.z, 2), 0.5);
	//if(distance < 50 && distance > -50)
	
	//{
	//	if(distance < 0)
	//	distance = distance*-1;
	//	fntdominant->draw3D(rd, "Testing", CoordinateFrame(rd->getCameraToWorldMatrix().rotation, gamepoint), 0.04*distance, Color3::yellow(), Color3::black(), G3D::GFont::XALIGN_CENTER, G3D::GFont::YALIGN_CENTER);
	//}

    renderDevice->disableLighting();

    if (sky.notNull()) {
        sky->renderLensFlare(renderDevice, lighting);
    }
	renderDevice->push2D();
		if(System::time() - 3 < messageTime)
		{
			fntdominant->draw2D(rd, message, Vector2((rd->getWidth()/2)-(fntdominant->get2DStringBounds(message, 20).x/2),(rd->getHeight()/2)-(fntdominant->get2DStringBounds(message, 20).y/2)), 20, Color3::yellow(), Color3::black());
		}

		//TODO--Move these to their own instance

		std::stringstream stream;
		stream << std::fixed << std::setprecision(1) << dataModel->getWorkspace()->timer;
		fntdominant->draw2D(rd, "Timer: " + stream.str(), Vector2(rd->getWidth() - 120, 25), 20, Color3::fromARGB(0x81C518), Color3::black());
		fntdominant->draw2D(rd, "Score: " + Convert(dataModel->getWorkspace()->score), Vector2(rd->getWidth() - 120, 50), 20, Color3::fromARGB(0x81C518), Color3::black());
		
		//GUI Boxes	
		Draw::box(G3D::Box(Vector3(0,25,0),Vector3(80,355,0)),rd,Color4(0.6F,0.6F,0.6F,0.4F), Color4(0,0,0,0));
		Draw::box(G3D::Box(Vector3(rd->getWidth() - 120,rd->getHeight() - 120,0),Vector3(rd->getWidth(),rd->getHeight(),0)),rd,Color4(0.6F,0.6F,0.6F,0.4F), Color4(0,0,0,0));
		
		//Camera menu title
		fntlighttrek->draw2D(rd, "CameraMenu", Vector2(rd->getWidth()-(fntlighttrek->get2DStringBounds("CameraMenu", 14).x+1),rd->getHeight() - 120), 14, Color3::white(), Color4(0.5F,0.5F,0.5F,0.5F));

		//Tools menu
		Draw::box(G3D::Box(Vector3(5, 210,0),Vector3(75, 210,0)),rd,Color4(0.6F,0.6F,0.6F,0.4F), Color4(0.6F,0.6F,0.6F,0.4F));
		fntlighttrek->draw2D(rd,"MENU", Vector2(10,332), 14, Color3::white(), Color4(0.5F,0.5F,0.5F,0.5F));

		
		drawButtons(rd);

		dataModel->drawMessage(rd);
		rd->pushState();
			rd->beforePrimitive();

			if(Globals::showMouse && mouseOnScreen)
			{
			glEnable( GL_TEXTURE_2D );
			glEnable(GL_BLEND);// you enable blending function
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
			
			
			glBindTexture( GL_TEXTURE_2D, cursorid);

			
			glBegin( GL_QUADS );
			glTexCoord2d(0.0,0.0);
			glVertex2f(mousepos.x-40, mousepos.y-40);
			glTexCoord2d( 1.0,0.0 );
			glVertex2f(mousepos.x+40, mousepos.y-40);
			glTexCoord2d(1.0,1.0 );
			glVertex2f(mousepos.x+40, mousepos.y+40 );
			glTexCoord2d( 0.0,1.0 );
			glVertex2f( mousepos.x-40, mousepos.y+40 );
			glEnd();

			glDisable( GL_TEXTURE_2D );
			}

			rd->afterPrimitive();
		rd->popState();
	renderDevice->pop2D();
}

void Demo::onKeyPressed(int key)
{
	if(key==VK_DELETE)
	{
		deleteInstance();
	}
}
void Demo::onKeyUp(int key)
{

}

void Demo::onMouseLeftPressed(HWND hwnd,int x,int y)
{
	SetFocus(hwnd);

	std::cout << "Click: " << x << "," << y << std::endl;

	bool onGUI = false;
	std::vector<Instance*> instances_2D = dataModel->getGuiRoot()->getAllChildren();
	for(size_t i = 0; i < instances_2D.size(); i++)
	{
		if(instances_2D.at(i)->getClassName() == "TextButton" || instances_2D.at(i)->getClassName() == "ImageButton")
		{
			BaseButtonInstance* button = (BaseButtonInstance*)instances_2D.at(i);
			if(button->mouseInButton(x,y, renderDevice))
			{
				onGUI = true;
				break;
			}
		}
	}
	if(!onGUI)
	{
		selectedInstance = NULL;
		testRay = cameraController.getCamera()->worldRay(dataModel->mousex, dataModel->mousey, renderDevice->getViewport());
		float nearest=std::numeric_limits<float>::infinity();
		Vector3 camPos = cameraController.getCamera()->getCoordinateFrame().translation;
		std::vector<Instance*> instances = dataModel->getWorkspace()->getAllChildren();
        for(size_t i = 0; i < instances.size(); i++)
		{
			if(instances.at(i)->getClassName() == "Part")
			{
				PhysicalInstance* test = (PhysicalInstance*)instances.at(i);
				float time = testRay.intersectionTime(test->getBox());
				if (time != inf()) 
				{
					if (nearest>time)
					{
						nearest=time;
						selectedInstance = test;
						//message = "Dragging = true.";
						//messageTime = System::time();
						//dragging = true;
					}
				}
			}
		}
	}
}
void Demo::onMouseLeftUp(int x,int y)
{
	//std::cout << "Release: " << x << "," << y << std::endl;

	dragging = false;
	//message = "Dragging = false.";
	//messageTime = System::time();
	std::vector<Instance*> instances_2D = dataModel->getGuiRoot()->getAllChildren();
	std::vector<Instance*> instances = dataModel->getWorkspace()->getAllChildren();
	for(size_t i = 0; i < instances_2D.size(); i++)
	{
		if(instances_2D.at(i)->getClassName() == "TextButton" || instances_2D.at(i)->getClassName() == "ImageButton")
		{
			BaseButtonInstance* button = (BaseButtonInstance*)instances_2D.at(i);
			if(button->mouseInButton(x, y, renderDevice))
			{
				button->onMouseClick();
			}
		}
	}
}
void Demo::onMouseRightPressed(int x,int y)
{
}
void Demo::onMouseRightUp(int x,int y)
{
}
void Demo::onMouseMoved(int x,int y)
{
	oldMouse = dataModel->getMousePos();
	dataModel->mousex = x;
	dataModel->mousey = y;

}
void Demo::onMouseWheel(int x,int y,short delta)
{
	if (mouseOnScreen==true)
	if (cameraController.onMouseWheel(x, y, delta))
	{
		AudioPlayer::playSound(cameraSound);
	}
}

/*
void App::main() {
	setDebugMode(false);
	debugController.setActive(false);
    applet->run();
}
*/

/*
App::App(const GAppSettings& settings, GWindow* wnd,HWND tempMainHWnd, SDLWindow* wndSDL) : GApp(settings, wnd) {

	applet = new Demo(this);
    hwnd = wndSDL->win32HWND();
    mainHWnd = tempMainHWnd;

}

App::~App() {
    delete applet;
}
*/

void Boop()
{
	OnError(1, "Test");
}




LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Demo *app = (Demo *)GetWindowLongPtr(hwnd, GWL_USERDATA);
	if (app==NULL)
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
    switch(msg)
    {
		case WM_KEYDOWN:
			if ((HIWORD(lParam)&0x4000)==0) // single key press
			{
				app->onKeyPressed(wParam);
			}
		break;
		case WM_KEYUP:
		{
			app->onKeyUp(wParam);
		}
		break;
		case WM_MOUSEWHEEL:
			app->onMouseWheel(LOWORD(lParam),HIWORD(lParam),HIWORD(wParam));
		break;
		case WM_SIZE:
			app->resizeWithParent(hwnd);
		break;
        default:
		{
            return DefWindowProc(hwnd, msg, wParam, lParam);
		}
    }
    return 0;
}

LRESULT CALLBACK ToolboxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    //Demo *app = (Demo *)GetWindowLongPtr(hwnd, GWL_USERDATA);
	MessageBox(NULL, (LPCSTR)wParam, (LPCSTR)lParam, 1);
	//if (app==NULL)
	//{
		//return DefWindowProc(hwnd, msg, wParam, lParam);
	//}
	switch(msg)
    {
		case WM_SIZE:
		break;
        default:
		{
            return DefWindowProc(hwnd, msg, wParam, lParam);
		}
    }
    return 0;
}

LRESULT CALLBACK G3DProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Demo *app = (Demo *)GetWindowLongPtr(hwnd, GWL_USERDATA);
	if (app==NULL)
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
    switch(msg)
    {
        case WM_QUIT:
			app->QuitApp();
        break;
        case WM_DESTROY:
            app->QuitApp();
        break;
		case WM_LBUTTONDOWN:
			app->onMouseLeftPressed(hwnd,LOWORD(lParam),HIWORD(lParam));
		break;
		case WM_LBUTTONUP:
			app->onMouseLeftUp(LOWORD(lParam),HIWORD(lParam));
		break;
		case WM_RBUTTONDOWN:
			app->onMouseRightPressed(LOWORD(lParam),HIWORD(lParam));
		break;
		case WM_RBUTTONUP:
			app->onMouseRightUp(LOWORD(lParam),HIWORD(lParam));
		break;
		case WM_MOUSEMOVE:
			app->onMouseMoved(LOWORD(lParam),HIWORD(lParam));
		break;
		case WM_SIZE:
		{
			app->onGraphics(app->renderDevice);
		}
		break;
        default:
		{
            return DefWindowProc(hwnd, msg, wParam, lParam);
		}
    }

    return 0;
}
void Demo::run() {
	usableApp = this;
	//setDebugMode(false);
	//debugController.setActive(false);
	/*
	if (!createWindowClass("ToolWindowClass",ToolProc,GetModuleHandle(0)))
	{
		return;
	}

    HWND propertyHWnd = CreateWindowEx(
    WS_EX_TOOLWINDOW,"ToolWindowClass", "ToolWindow",
    WS_SYSMENU | WS_VISIBLE | WS_CHILD,
	0, 0, 800, 64,
    hWndMain, NULL, GetModuleHandle(0), NULL);

	ShowWindow(propertyHWnd,SW_SHOW);
	*/
	UpdateWindow(_hWndMain);

    // Load objects here=
	cursor = Texture::fromFile(GetFileInPath("/content/cursor2.png"));
	Globals::surface = Texture::fromFile(GetFileInPath("/content/images/surfacebr.png"));
	Globals::surfaceId = Globals::surface->getOpenGLID();
	fntdominant = GFont::fromFile(GetFileInPath("/content/font/dominant.fnt"));
	fntlighttrek = GFont::fromFile(GetFileInPath("/content/font/lighttrek.fnt"));
	cameraSound = GetFileInPath("/content/sounds/SWITCH3.wav");
	clickSound = GetFileInPath("/content/sounds/switch.wav");
	dingSound = GetFileInPath("/content/sounds/electronicpingshort.wav");
    sky = Sky::create(NULL, ExePath() + "/content/sky/");
	cursorid = cursor->openGLID();

	RealTime	now=0, lastTime=0;
	double		simTimeRate = 1.0f;
	float		fps=30.0f;

	RealTime	desiredFrameDuration=1.0/fps;
	onInit();
	RealTime    lastWaitTime=0;

	MSG			messages;
	//RECT cRect;
	//GetClientRect(_hWndMain,&cRect);
	//renderDevice->notifyResize(cRect.right,cRect.bottom);
	//Rect2D viewportRect = Rect2D::xywh(0,0,cRect.right,cRect.bottom);
	//renderDevice->setViewport(viewportRect);
	//window()->setInputCaptureCount(1);
	resizeWithParent(_hWndMain);

	while (!quit)
	{
		
		lastTime = now;
		now = System::getTick();
		RealTime timeStep = now - lastTime;
		
		m_userInputWatch.tick();
			onUserInput(userInput);
			//m_moduleManager->onUserInput(userInput);
		m_userInputWatch.tock();
		
		m_simulationWatch.tick();
			//debugController.doSimulation(clamp(timeStep, 0.0, 0.1));
			//g3dCamera.setCoordinateFrame
				//(debugController.getCoordinateFrame());

			double rate = simTimeRate;    
			RealTime rdt = timeStep;
			SimTime  sdt = timeStep * rate;
			SimTime  idt = desiredFrameDuration * rate;

			onSimulation(rdt,sdt,idt);
		m_simulationWatch.tock();

		m_waitWatch.tick();
		{
			RealTime now = System::time();
			// Compute accumulated time
			System::sleep(max(0.0, desiredFrameDuration - (now - lastWaitTime)));
			lastWaitTime = System::time();
		}
		m_waitWatch.tock();

		m_graphicsWatch.tick();
			renderDevice->beginFrame();
				renderDevice->pushState();
					onGraphics(renderDevice);
				renderDevice->popState();
				//renderDebugInfo();
			renderDevice->endFrame();
			//debugText.clear();
		m_graphicsWatch.tock();

		while (PeekMessage (&messages, NULL, 0, 0,PM_REMOVE))
		{
			if (IsDialogMessage(_hWndMain, &messages) == 0)
			{
				TranslateMessage(&messages);
				DispatchMessage(&messages);
			}
		}
	}
	onCleanup();
}
void Demo::resizeWithParent(HWND parentWindow)
{
	RECT rect;
	GetClientRect(parentWindow,&rect);
	SetWindowPos(_hwndRenderer,NULL,0,0,rect.right,rect.bottom-60,SWP_NOMOVE);
	SetWindowPos(_hwndToolbox,NULL,0,rect.bottom-60,rect.right,60,SWP_NOACTIVATE | SWP_SHOWWINDOW);
	GetClientRect(_hwndRenderer,&rect);
	int viewWidth=rect.right;
	int viewHeight=rect.bottom;
	renderDevice->notifyResize(viewWidth,viewHeight);
	Rect2D viewportRect = Rect2D::xywh(0,0,viewWidth,viewHeight);
	renderDevice->setViewport(viewportRect);

}

void Demo::QuitApp()
{
	PostQuitMessage(0);
	quit=true;
}
void Demo::onCreate(HWND parentWindow)
{
	//SetWindowLongPtr(hwndRenderer,GWL_USERDATA,(LONG)this);
	//SetWindowLongPtr(hwndToolbox,GWL_USERDATA,(LONG)this);
	//SetWindowLongPtr(hwndMain,GWL_USERDATA,(LONG)&demo);
}

int main(int argc, char** argv) {
	try{
		hresult = OleInitialize(NULL);
		if (!AXRegister())
			return 0;

		tempPath = ((std::string)getenv("temp")) + "/"+PlaceholderName;
		CreateDirectory(tempPath.c_str(), NULL);
	    
		message = tempPath;
		messageTime = System::time();
		AudioPlayer::init();
		GAppSettings settings;
		settings.window.resizable = true;
		settings.writeLicenseFile = false;
		settings.logFilename = tempPath + "/g3dlog.txt";
		settings.window.center = true;
		HMODULE hThisInstance = GetModuleHandle(NULL);
		if (!createWindowClass("mainHWND",WndProc,hThisInstance))
			return false;
		if (!createWindowClass("toolboxHWND",ToolboxProc,hThisInstance))
			return false;
		if (!createWindowClass("G3DWindow",G3DProc,hThisInstance))
			return false;
		
		HWND hwndMain = CreateWindowEx(
			WS_EX_ACCEPTFILES,
			"mainHWND",
			"Main test",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			800,
			660,
			NULL, // parent
			NULL, // menu
			hThisInstance,
			NULL
		);

		if(hwndMain == NULL)
		{
			MessageBox(NULL, "Failed to create HWND", (PlaceholderName + " Crash").c_str() , MB_OK);
			return 0;
		}
		SendMessage(hwndMain, WM_SETICON, ICON_BIG,(LPARAM)LoadImage(GetModuleHandle(NULL), (LPCSTR)MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE));
		
		
		Demo demo = Demo(settings,hwndMain);
		demo.run();	
	}
	catch(...)
	{
		OnError(-1);
	}
    return 0;
}
