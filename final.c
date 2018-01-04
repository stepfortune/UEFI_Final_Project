#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimplePointer.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimpleFileSystem.h>
#include <guid/FileInfo.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void constructGameFile(); //为我们的游戏资源文件添加到模拟器环境的文件系统中
void showWelcome();  //显示欢迎界面
void showMenu();     //显示菜单界面
void showGame();     //将游戏界面显示出来, 进入一个游戏界面时,只会调用一次
void showTips();    //在游戏右侧显示一些说明
void showSteps();   //显示步数
void showMap();  
void showWin();     //显示通关的界面
void showSelectLevel();  //显示选关的界面

void loadMap();    //读取游戏关卡地图文件信息到二维数组中
void exitGame();   
void updateKey();  //相应键盘按键事件, 设置游戏状态位

void render();     //根据游戏的按键及相应的状态 进行下一步操作
void init();       //初始化游戏状态位
void clearScreen();//清屏
void selectLevel();//选关
void playGame();   //操纵人物
void initMap();    //得到人物的初始位置 和 箱子 目的地的 位置信息
void basicShow(char *str) { gST->ConOut->OutputString(gST->ConOut, str); }
void getFileName(int level, char* buf); 

void drawMan();    //绘制人物
void drawDestination(); //绘制箱子目的地
void drawBox();    //绘制箱子
void drawFloor();  
void drawLine(int start_x, int start_y, int end_x, int end_y, int width);

void drawRectangle(int start_x, int start_y, int width, int length   //绘制单色矩形
	,EFI_GRAPHICS_OUTPUT_BLT_PIXEL* BltBuffer) {

	EFI_GRAPHICS_OUTPUT_PROTOCOL *gGraphicsOutput;
	EFI_STATUS _status;
	_status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
		NULL, (VOID**)&gGraphicsOutput);
	if (EFI_ERROR(_status)) {
		Print(L"draw Fail!!!");
	}

	_status = gGraphicsOutput->Blt(gGraphicsOutput,
		BltBuffer,
		EfiBltVideoFill,
		0, 0,
	    start_x, start_y,
		width, length,
		0);

}

void copyRectangle(int s_x, int s_y,  //拷贝屏幕区域(矩形)到指定的位置
	int e_x, int e_y,int width, int length) {
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gGraphicsOutput;
	EFI_STATUS _status;
	_status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
		NULL, (VOID**)&gGraphicsOutput);
	_status = gGraphicsOutput->Blt(gGraphicsOutput,
		0,
		EfiBltVideoToVideo,
		s_x, s_y,
		e_x, e_y,
		width, length,
		0);
}



typedef enum _game_status{ 
	WELCOME, MENU, SELECT_LEVEL, GAMING, PAUSE, EXIT, BACK, WIN
}Game_Status;                    //游戏状态

typedef enum _directory {
	UP, DOWN, LEFT, RIGHT
}Directions;                      //人物的移动方向


typedef struct {
	int x;
	int y;
	char text[3];
}Node;



typedef struct {
	int x;
	int y;
}Direction;                      //人物移动方向的详细信息


static int isSelectLevel = 0;   //是否在选关
static int currentLevel = 1;    //当前是第几关
static int isRefresh = 0;       //判断是否调用render()函数
static Game_Status game_status;//游戏的状态信息
static Directions Move_Directions;//当前的移动方向
static int Steps = 0;           //本关游戏的步数
static int cols = 0;            //游戏地图的列数
static int rows = 0;            //游戏地图的行数
static int boxNum = 0;          //本关的箱子数量
static Node MAN;                //存放玩家的位置信息
static int X_Pos[10] = { 0 };   //存储箱子目的地的位置信息(x轴坐标)
static int Y_Pos[10] = { 0 };   //同上, 存y轴坐标
static const char BAR = '#';    //墙
static const char DESTINATION = 'X'; //箱子目的地
static const char BOX = 'O';    //箱子
static const char MERGE = 'Q';  //箱子推到目的地
static const char EMPTY = ' ';   //空位置
static const char MAN_char = '@'; //玩家


// 下面定义了一些颜色的像素信息
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL BLACK[1] = { 0,0,0,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL SKIN[1] = { 216,229,247,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL WHITE[1] = { 255,255,255,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL LIGHT_BROWN[1] = { 47,81,175,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL DARK_BROWN[1] = { 35,40,147,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL LIGHT_BLUE[1] = { 229,183,159,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL DARK_BLUE[1] = { 161,128,44,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL LIGHT_RED[1] = { 108,127,247,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL PINK[1] = { 201,181,255,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL RED[1] = { 0,0,255,0 };
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL YELLOW[1] = {0,255,255,0 };



static char MAP[50][50]; //存储当前关卡的地图信息


static UINT8 HU[23][21] = {
	{0,0,0,0,0,0,0,0,1,1,1,1,1},
	{0,0,0,0,0,0,1,1,1,0,0,0,1,1,1},
	{0,0,0,0,0,1,1,0,0,0,1,0,0,0,1,1},
	{0,0,0,0,1,1,0,0,1,1,1,1,1,0,0,1,1},
	{0,0,1,1,1,0,0,1,0,0,0,0,0,1,0,0,1,1,1},
	{1,1,1,0,0,0,1,0,0,1,1,1,0,0,1,0,0,0,1,1,1},
	{1,1,1,1,1,1,0,0,1,1,0,0,1,0,0,1,1,1,1,1,1},
	{0,0,0,0,0,0,0,1,1,0,1,0,1,1},
	{1,1,1,1,1,1,0,0,1,0,0,1,1,0,0,1,1,1,1,1,1},
	{1,1,1,0,0,0,1,0,0,1,1,1,0,0,1,0,0,0,1,1,1},
	{0,0,1,1,1,0,0,1,0,0,0,0,0,1,0,0,1,1,1},
	{0,0,0,0,1,1,0,0,1,1,1,1,1,0,0,1,1},
	{0,0,0,0,0,1,1,0,0,0,1,0,0,0,1,1},
	{0,0,0,0,0,0,1,1,1,0,0,0,1,1,1},
	{0,0,0,0,0,0,0,0,1,1,1,1,1},
	{0,0,0,0,0,0,0,0,0,1,0,1},
	{0,0,0,0,0,0,0,0,0,1,0,1},
	{0,0,0,0,0,0,0,0,0,1,0,1,0,0,1,1},
	{0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,0,1},
	{0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,1,1},
	{0,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,0,0,1},
	{0,0,0,0,1,1,0,0,1,0,0,0,1,1,1,0,0,1,1},
	{0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1}
};


static UINT8 MAN_PIC[20][13] = {   //人物的图像信息
	{0,0,0,1,1,1,1,1,1,1},
	{0,0,1,2,2,2,2,2,2,2,1},
	{0,1,2,2,3,3,3,3,3,2,2,1},
	{0,1,3,4,4,4,4,4,4,4,3,1},
	{0,1,3,4,1,4,4,4,1,4,3,1},
	{0,1,3,4,1,4,4,4,1,4,3,1},
	{0,1,3,5,4,4,4,4,4,5,3,1},
    {0,1,3,4,1,4,4,4,1,4,3,1},
	{0,1,3,4,4,1,1,1,4,4,3,1},
	{0,0,1,4,4,4,4,4,4,4,1},
	{0,1,6,6,7,7,7,7,7,6,6,1},
	{1,6,6,6,6,7,7,7,6,6,6,6,1},
	{1,6,1,6,6,7,7,7,6,6,1,6,1},
	{1,7,1,6,6,7,7,7,6,6,1,7,1},
	{1,4,1,6,6,7,7,7,6,6,1,4,1},
	{0,1,1,7,7,7,7,7,7,7,1,1},
	{0,0,1,8,9,9,9,9,9,8,1},
	{0,0,1,9,9,1,1,1,9,9,1},
	{0,0,1,6,6,1,6,1,6,6,1},
	{0,0,0,1,1,0,0,0,1,1}
};

static UINT8 BOX_PIC[15][15] = {  //箱子的图像信息
	{ 0,0,0,1,1,1,1,1,1,1,1,1 },
	{ 0,0,1,2,2,2,2,2,2,2,2,2,1 },
	{ 0,1,2,2,2,2,2,2,2,2,2,2,2,1 },
	{ 1,2,2,2,2,2,2,2,2,2,2,2,2,2,1 },
	{ 1,2,2,2,2,2,2,2,2,2,2,2,2,2,1 },
	{ 1,2,2,2,2,2,2,2,2,2,2,2,2,2,1 },
	{ 1,2,2,2,2,2,1,1,1,2,2,2,2,2,1 },
	{ 1,1,1,1,1,1,1,3,1,1,1,1,1,1,1 },
	{ 1,3,3,3,3,3,1,1,1,3,3,3,3,3,1 },
	{ 1,3,3,3,3,3,3,3,3,3,3,3,3,3,1 },
	{ 1,3,3,3,3,3,3,3,3,3,3,3,3,3,1 },
	{ 1,3,3,3,3,3,3,3,3,3,3,3,3,3,1 },
	{ 0,1,3,3,3,3,3,3,3,3,3,3,3,1 },
	{ 0,0,1,3,3,3,3,3,3,3,3,3,1 },
	{ 0,0,0,1,1,1,1,1,1,1,1,1 },
};

static UINT8 DESTINATION_PIC[19][15] = {   //箱子目的地的图像信息
	{1,1,0,0,0,0,0,0,0,0,0,1,1},
	{1,1,1,0,0,0,0,0,0,0,1,1,1},
	{1,2,2,1,1,0,0,0,1,1,2,2,1},
	{1,2,2,2,2,1,1,1,2,2,2,2,1},
	{0,1,2,2,2,2,2,2,2,2,2,1},
	{0,0,1,2,2,2,2,2,2,2,1},
	{0,1,2,2,1,2,2,2,1,2,2,1},
	{0,1,2,2,1,2,1,2,1,2,2,1,1,1,1},
	{0,1,2,3,2,1,2,1,2,3,2,1,2,2,1},
	{0,0,1,1,2,2,2,2,2,1,1,2,2,2,1},
	{0,1,2,2,1,2,2,2,1,2,2,1,2,1,1},
	{0,1,2,2,1,2,2,2,1,2,2,1,2,1},
	{0,1,2,2,2,2,2,2,2,2,2,1,2,1 },
	{0,1,4,2,2,2,2,2,2,2,4,1,2,1 },
	{0,1,2,2,2,2,2,2,2,2,2,1,1},
	{0,1,4,2,2,2,2,2,2,2,4,1},
	{0,1,2,2,2,2,2,2,2,2,2,1},
	{0,0,1,2,2,1,1,1,2,2,1},
	{0,0,0,1,1,0,0,0,1,1}
};

int main
    (IN int Argc
    , IN char **Argv) 
{
	EFI_STATUS Status;
    EFI_SIMPLE_TEXT_OUTPUT_MODE SavedConsoleMode;
     // Save the current console cursor position and attributes
    CopyMem(&SavedConsoleMode, gST->ConOut->Mode, sizeof(EFI_SIMPLE_TEXT_OUTPUT_MODE));
    Status = gST->ConOut->EnableCursor(gST->ConOut, FALSE); //设置光标是否可用
    ASSERT_EFI_ERROR(Status);
    Status = gST->ConOut->ClearScreen(gST->ConOut);    //清屏
    ASSERT_EFI_ERROR(Status);
    Status = gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));//设置屏幕输出的样式
    ASSERT_EFI_ERROR(Status);
    Status = gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);   //设置光标位置
    ASSERT_EFI_ERROR(Status);
	
	
	constructGameFile();
    init();      
	while(1){        //使用一个循环还控制游戏的进行
		updateKey(); //updateKey() 和 render()函数 将键盘事件的读取和游戏状态的设置 与 相应的逻辑处理 进行了分离, 使整个项目逻辑更清晰
		if (isRefresh == 1)
			render();
		if (game_status == EXIT)
			break;
	};

	gST->ConOut->EnableCursor(gST->ConOut, SavedConsoleMode.CursorVisible);
    gST->ConOut->SetCursorPosition(gST->ConOut, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
    gST->ConOut->SetAttribute(gST->ConOut, SavedConsoleMode.Attribute);
    gST->ConOut->ClearScreen(gST->ConOut);
    return 0;
}

void init() {
	game_status = WELCOME;
	isRefresh = 1;
	currentLevel = 1;
}

void updateKey() {
	EFI_STATUS _status;
	EFI_INPUT_KEY key;
	if (!isSelectLevel) { //如果是在选关状态, 则这个函数不处理键盘事件, 改由选关函数专门处理
		_status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
		ASSERT_EFI_ERROR(_status);
		if (EFI_ERROR(_status))
			return;
	}

	switch (game_status) {
	case WELCOME:	
		if (key.ScanCode == SCAN_ESC) //退出游戏
			exitGame();
		else if (key.ScanCode == SCAN_F1) { //进入菜单界面
			showMenu();
			game_status = MENU;
		}
   		break;
	case GAMING:
		if (key.ScanCode == SCAN_UP) { //向上移动
			Move_Directions = UP;
			isRefresh = 1;
		}
		else if (key.ScanCode == SCAN_DOWN) {//向下移动
			Move_Directions = DOWN;
			isRefresh = 1;
		}
		else if (key.ScanCode == SCAN_LEFT) {//向左移动
			Move_Directions = LEFT;
			isRefresh = 1;
		}
		else if (key.ScanCode == SCAN_RIGHT) {//向右移动
			Move_Directions = RIGHT;
			isRefresh = 1;
		}
		else if (key.ScanCode == SCAN_F1) {//重玩本关
			showGame();
			isRefresh = 0;
		}
		else if (key.ScanCode == SCAN_ESC) {//退回到菜单界面
			game_status = MENU;
			showMenu();
			isRefresh = 1;
		}
		break;
	case MENU:
		if (key.ScanCode == SCAN_F1) { //直接开始游戏
			showGame();
			game_status = GAMING;
		}
		else if (key.ScanCode == SCAN_F2) { //选关
			game_status = SELECT_LEVEL;
			isSelectLevel = 1;
		}
		else if (key.ScanCode == SCAN_ESC)//退出游戏
			game_status = EXIT;
		break;

	case SELECT_LEVEL:
		selectLevel();       //处理键盘事件
		showGame();          //显示游戏界面
		game_status = GAMING;
		isSelectLevel = 0;   //让updatekey()函数接管键盘事件的响应处理
		break;
	case PAUSE:
		break;
	case WIN:
		if (key.ScanCode == SCAN_ESC)        // exit the game
			game_status = EXIT;
		else if (key.ScanCode == SCAN_F1) {    // select game level
			game_status = SELECT_LEVEL;
			isSelectLevel = 1;
		}
		else if (key.ScanCode == SCAN_F2) {  // 重玩本关
			showGame();
			game_status = GAMING;
		}
		else if (key.ScanCode == SCAN_F3) {  // 下一关
			currentLevel++;
			if (currentLevel > 35)
				currentLevel = 1;
			showGame();
			game_status = GAMING;
		}
		break;
	case EXIT:
		exitGame();
	}
}

void render() {
	isRefresh = 0;
	switch (game_status)
	{
	case WELCOME:
		showWelcome();
		break;
	case GAMING:
		playGame();
		break;
	case MENU:
		break;
	default:
		break;
	}
}

void exitGame() {
	game_status = EXIT;
}

void showMenu() {
	clearScreen();
	Print(L"\n\n\n\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t*************************** Menu ***************************\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t*                        START GAME  (F1)                  *\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t*                       SELECT LEVEL (F2)                  *\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t*                           EXIT     (F3)                  *\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t*************************** Menu ***************************\n\r");
}

void getLevel() {
	scanf("%d", &currentLevel);
}
void selectLevel() {
	clearScreen();
	Print(L"\n\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\tPlease Enter A Level(1~35):");
	do {
		getLevel();
	} while (currentLevel > 35 || currentLevel < 1);
}

void showWelcome(VOID) {
	gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
	Print(L"\n\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t************************** SOKOBAN *************************\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t*                  Welcome to UEFI Gameing                 *\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t*                   By huzehao & yinyuhan                  *\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t*                         MENU   (F1)                      *\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t*                         EXIT   (ESC)                     *\n\r");
	Print(L"\t\t\t\t\t\t\t\t\t\t\t************************** SOKOBAN *************************\n\r");
}

void clearScreen() {
	gST->ConOut->ClearScreen(gST->ConOut);
}


void getFileName(int num, CHAR16* buf) { //只接受最多两位数字, 在buf 中存放需要打开的文件名, 比如 num = 1 , 则buf = "1.txt"
	int bigThenTen = num / 10;
	CHAR16 a = (CHAR16)(bigThenTen + 48); //十位
	CHAR16 b = (CHAR16)(num - bigThenTen * 10 + 48); //个位
	if (bigThenTen == 0) {
		buf[0] = b;
		buf[1] = '.';
		buf[2] = 't';
		buf[3] = 'x';
		buf[4] = 't';
		buf[5] = '\0';
	}
	else {
		buf[0] = a;
		buf[1] = b;
		buf[2] = '.';
		buf[3] = 't';
		buf[4] = 'x';
		buf[5] = 't';
		buf[6] = '\0';
	}
}

void constructGameFile() {
	EFI_STATUS status;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem; //文件系统句柄
	EFI_FILE_PROTOCOL *Root; //文件系统根目录句柄
	EFI_FILE_PROTOCOL *Dir;  //文件系统, 游戏资源文件夹句柄
	EFI_FILE_PROTOCOL *File; //游戏文件句柄
	CHAR16 * TextBuf = (CHAR16*)L"CREATE SUCCESSFULLY!!!";
	CHAR16 *BUFFER;         
	UINTN Buffer_Size = sizeof(CHAR16) * 65;
	status = gBS->AllocatePool(EfiBootServicesCode, Buffer_Size, (VOID**)&BUFFER);
	if (!BUFFER || EFI_ERROR(status)) {
		Print(L"ALLOCATE FAILURE");
	}

	UINTN W_BufSize = 0;
	UINTN R_BufSize = 64;
	status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid,
		NULL, (VOID**)&SimpleFileSystem);
	if (EFI_ERROR(status))
	{
		Print(L"1");
	}
	status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root);
	status = Root->Open(Root,
		&Dir,
		(CHAR16*)L"Sokoban",
		EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
		EFI_FILE_DIRECTORY);
	status = Dir->Open(Dir,
		&File,
		(CHAR16*)L"config.txt",
		EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
		0);

	if (File && !EFI_ERROR(status))
	{
		status = File->Read(File, &R_BufSize, BUFFER); //从config.txt文件中读信息, 如果读取到的字节数为0(第一次游戏), 则创建游戏资源文件 
		//Print(L"R_BufSize: %d\n\r", R_BufSize);      //第二次游戏及以后不需要重新创建资源文件
		if (R_BufSize == 0) {
			W_BufSize = StrLen(TextBuf) * 2;
			status = File->Write(File, &W_BufSize, TextBuf);
			if (EFI_ERROR(status))
				Print(L"Write Failure");
			status = File->Close(File);
			CHAR16 filename[10];
			int i;
			for (i = 1; i <= 35; i++) {//在Sokoban文件夹下 创建1.txt ~ 35.txt
				getFileName(i, filename);
				status = Dir->Open(Dir, &File, filename,
					EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE, 0);
				status = File->Write(File, &W_BufSize, TextBuf);
				status = File->Close(File);
			}
		}
		else {
			//BUFFER[R_BufSize] = 0;
			//Print(L"config Read: %s\n\r", BUFFER);
			status = File->Close(File);
		}
	}
	else {
		File->Close(File);
		Print(L"config.txt not exist");
	}
	status = Dir->Close(Dir);
	status = Root->Close(Root);
	if (EFI_ERROR(status))
		Print(L"Close root Failure");
	status = gBS->FreePool(BUFFER);
	if (EFI_ERROR(status))
		Print(L"Free error");
}

void loadMap() {
	CHAR16 level[10];
	getFileName(currentLevel, level);
	EFI_STATUS status;
	EFI_FILE_PROTOCOL *Dir = 0;
	EFI_FILE_PROTOCOL *Root;              //NT32根目录句柄
	EFI_FILE_PROTOCOL *File;              //推箱子地图文件的句柄
	CHAR8 R_BUF[500];                      //存放读取的文件中的内容
	UINTN R_SIZE = 64;                      //存放一次读取的字节数, 一次最多读64个字节
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;//UEFI文件系统句柄
	status = gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid,
		NULL, (VOID**)&SimpleFileSystem); //获取UEFI文件系统句柄
	if (EFI_ERROR(status)) {
		Print(L"SimpleFileSystem");
	}
	status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root); //获取NT32根目录句柄
	
	status = Root->Open(                 //获取NT32根目录下Sokoban文件夹的句柄                              
		Root,
		&Dir,
		(CHAR16*)(L"Sokoban"),
		EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,         
		EFI_FILE_DIRECTORY
	);
	status = Dir->Open(Dir, &File, level,
		EFI_FILE_MODE_CREATE | EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE , 0);

	status = File->Read(File, &R_SIZE, R_BUF);
	UINTN SUM = R_SIZE;
	//gST->ConOut->SetCursorPosition(gST->ConOut, 0, 40);
	//Print(L"SIZE: %d\n\r", R_SIZE);
	//R_BUF[65] = 0;
	//Print(L"r_buf: \n\r %s", R_BUF);
	while (R_SIZE == 64) { //循环读入游戏资源文件到 R_BUF , 一次最多读入64个字节
		status = File->Read(File, &R_SIZE, R_BUF + SUM);
		SUM += R_SIZE;
	}

	char c; 
	int _row = 0;
	int _col = 0;
	for (UINTN i = 0; i < SUM; i++) { //将 R_BUF中的信息映射到MAP中
		c = R_BUF[i];
		if (c != NULL) {
			if (c == '\n'){
				_row++;
				_col = 0;
			}
			else MAP[_row][_col++] = c;
		}
	}
	rows = _row + 1;   //初始化本关的地图的行数和列数
	cols = _col;
	/*for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			Print(L"%c", MAP[i][j]);
		}
		Print(L"\n");
	}*/
	
	status = Root->Close(Root);
	status = Dir->Close(Dir);
	status = File->Close(File);
}

int filrate() {   // "掩膜法" , 对箱子目的地的情况刷新一次 
	int i;
	int j = 0;
	char c;
	for (i = 0;  i < boxNum; i++) {
		c = MAP[Y_Pos[i]][X_Pos[i]];
		if (c == ' ')    // 本来是箱子目的地但在playGame()函数的操作中被弄成了空位置, 则将其重新设置成箱子目的地
			MAP[Y_Pos[i]][X_Pos[i]] = 'X';

		if (c == 'O'|| c== 'Q') { //本来是箱子目的地, 而此时放的是箱子, 则将该位置表示为箱子推到了目的地
			MAP[Y_Pos[i]][X_Pos[i]] = 'Q';
			j++;
		}

	}
	if (j == boxNum) // 如果, 所有的箱子都到了目的地, 则pass
		return 1;
	else return 0;
}

void updateMap(int x, int y, char c) { //更新游戏界面的地图
	
	switch (c)
	{
	case 'X':
		copyRectangle(61, 0, 30 * x, 30 * y, 29, 30);
		break;
	case 'Q':
	case 'O':
		copyRectangle(90, 0, 30 * x, 30 * y, 30, 30);
		break;
	case '@':
		copyRectangle(31, 0, 30 * x, 30 * y, 29, 30);
		break;
	case ' ':
		drawRectangle(30 * x, 30 * y, 30, 30, BLACK);
		break;
	case '#':
		copyRectangle(0, 0, 30 * x, 30 * y, 30, 30);
		break;
	}

}

void playGame() {
	Steps += 1;  
	char thing_front_man;
	char thing_front_box;
	Node old_pos;
	old_pos.x = MAN.x;
	old_pos.y = MAN.y;
	int _move = 0;
	int _x = 0; //人物前面的东西的x轴坐标
	int _y = 0;  //人物前面的东西的y轴坐标
	int __x = 4; //人物前面的前面的东西的x轴坐标
	int __y = 3; //...

	Direction direction;
	switch (Move_Directions)
	{
	case UP:
		direction.x = 0;
		direction.y = -1;
		break;
	case DOWN:
		direction.x = 0;
		direction.y = 1;
		break;
	case LEFT:
		direction.x = -1;
		direction.y = 0;
		break;
	case RIGHT:
		direction.x = 1;
		direction.y = 0;
		break;
	}

    _x = MAN.x + direction.x;
	_y = MAN.y + direction.y;
	thing_front_man = MAP[_y][_x];
	if (thing_front_man != '#') {  //如果人物的前面不是墙
		if (thing_front_man == ' ' || thing_front_man == 'X') { //如果人物的前面是空位置或者箱子的目的地
			MAN.y += direction.y;
			MAN.x += direction.x;
			MAP[MAN.y][MAN.x] = '@';
			MAP[old_pos.y][old_pos.x] = ' ';
			_move = 1;
		}
		else {  //如果人物的前面是箱子
			__x = _x + direction.x;
			__y = _y + direction.y;
			thing_front_box = MAP[__y][__x];
			if (thing_front_box == 'X' || thing_front_box == ' ') {//箱子的前面是空地或目的地
				MAP[__y][__x] = 'O';
				MAP[_y][_x] = '@';
				MAP[MAN.y][MAN.x] = ' ';
				MAN.x += direction.x;
				MAN.y += direction.y;
				_move = 1;
				
			}
		}
	}
	if (_move == 1) {
		if (filrate()) {
			showWin();
			game_status = WIN;
			return;
		} //人物每移动一次最多改变地图中三个位置的信息, 我们只对这三个位置在游戏界面上进行刷新 ,而不是一下子刷新整个界面
		updateMap(old_pos.x, old_pos.y, MAP[old_pos.y][old_pos.x]);
		updateMap(_x, _y, MAP[_y][_x]);
		if (__x != 0)   //如果人物的前面有箱子, 并且箱子的前面是空地或目的地的时, 要对人物的前面的前面的位置进行刷新
			updateMap(__x, __y, MAP[__y][__x]);
	}
	showSteps();   //更新界面的步数
}

void showGame() {
	clearScreen();
	Steps = 0;
	loadMap();
	initMap();
	showMap();
	showTips();
}

void initMap() {             //将二维矩阵转换为一维矩阵, 将二维矩阵隐含的位置信息导出
	char tmp;
	boxNum = 0;
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			tmp = MAP[i][j];
			if (tmp == 'X' || tmp == 'Q') { //得到所有目的地的坐标和箱子的数量
				X_Pos[boxNum] = j;
				Y_Pos[boxNum] = i;
				boxNum++;
			}
			if (tmp == '@') { //得到人物的初始位置信息
				MAN.x = j;
				MAN.y = i;
				MAN.text[0] = tmp;
				MAN.text[1] = '\0';
			}
		}
	}
}

void showWin() {
	clearScreen();
	gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
	Print(L"\n\n\r");
	Print(L"\t\t\t\t******************************  YOU WIN !!! *****************************\n\r");
	Print(L"\t\t\t\t*                             SELECT LEVEL (F1)                         *\n\r");
	Print(L"\t\t\t\t*                             PLAY AGAIN   (F2)                         *\n\r");
	Print(L"\t\t\t\t*                             NEXT LEVEL   (F3)                         *\n\r");
	Print(L"\t\t\t\t*                                 EXIT     (ESC)                        *\n\r");
	Print(L"\t\t\t\t******************************  YOU WIN !!! *****************************\n\r");
}

void showTips() {
	gST->ConOut->SetCursorPosition(gST->ConOut, 50, 2);
	gST->ConOut->OutputString(gST->ConOut, L"Controll:  ←↑↓→");
	showSteps();
	gST->ConOut->SetCursorPosition(gST->ConOut, 50, 6);
	gST->ConOut->OutputString(gST->ConOut, L"Replay(F1)");
	gST->ConOut->SetCursorPosition(gST->ConOut, 50, 8);
	gST->ConOut->OutputString(gST->ConOut, L"Exit(ESC)");
}

void showSteps() {
	gST->ConOut->SetCursorPosition(gST->ConOut, 50, 4);
	Print(L"Step: %d", Steps);
}

void showMap() { //在进入每一关游戏时只会调用一次, 绘制游戏地图界面
	drawRectangle(1, 1, 28, 13,DARK_BROWN);   //画墙
	drawRectangle(0, 16, 14, 13,DARK_BROWN);  //画墙
	drawRectangle(17, 16, 14, 13,DARK_BROWN); //画墙
	drawDestination();
	drawMan();
	drawBox();
	int i, j;
	for (i = 0; i < rows; i++) {
		for (j = 0; j < cols; j++) {
			char tmp = MAP[i][j];
			if (tmp == '#') {
				copyRectangle(0, 0, 30 * j, 30 * i, 30, 30);
			}
			else if (tmp == 'X') {
				copyRectangle(61, 0, 30 * j, 30 * i, 30, 30);
			}
			else if (tmp == 'O' || tmp == 'Q' ) {
				copyRectangle(91, 0, 30 * j, 30 * i, 30, 30);
			}
			else if (tmp == '@') {
				copyRectangle(31, 0, 30 * j, 30 * i, 30, 30);
			}
		}
	}

}

//下面的函数是绘制 箱子,人物, 箱子目的地 的函数, 在进入每关时只会调用一次, 函数的编写有点不设计模式, 以后再改...
//遍历表示图像的数组, 一个像素一个像素的绘制图像
void drawBox() {
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gGraphicsOutput;
	EFI_STATUS _status;
	_status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
		NULL, (VOID**)&gGraphicsOutput);
	if (EFI_ERROR(_status)) {
		Print(L"draw Fail!!!");
	}
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL COMMON[1] = { 0,0,0,0 };
	for (int i = 0; i < 15; i++) {
		for (int j = 0; j < 15; j++) {
			switch (BOX_PIC[i][j])
			{
			case 1:
				COMMON[0] = BLACK[0];
				break;
			case 2:
				COMMON[0] = RED[0];
				break;
			case 3:
				COMMON[0] = WHITE[0];
				break;
			}

			_status = gGraphicsOutput->Blt(gGraphicsOutput,
				COMMON,
				EfiBltVideoFill,
				0, 0,
				j + 97, i + 7,
				1, 1,
				0);

		}
	}
}

void drawDestination() {
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gGraphicsOutput;
	EFI_STATUS _status;
	_status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
		NULL, (VOID**)&gGraphicsOutput);
	if (EFI_ERROR(_status)) {
		Print(L"draw Fail!!!");
	}
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL COMMON[1] = { 0,0,0,0 };
	for (int i = 0; i < 19; i++) {
		for (int j = 0; j < 15; j++) {

			switch (DESTINATION_PIC[i][j])
			{
			case 1:
				COMMON[0] = BLACK[0];
				break;
			case 2:
				COMMON[0] = YELLOW[0];
				break;
			case 3:
				COMMON[0] = RED[0];
				break;
			case 4:
				COMMON[0] = DARK_BROWN[0];
				break;
			default:
				break;
			}
			_status = gGraphicsOutput->Blt(gGraphicsOutput,
				COMMON,
				EfiBltVideoFill,
				0, 0,
				j + 67, i + 5,
				1, 1,
				0);
		}
	}
}

void drawMan() {
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gGraphicsOutput;
	EFI_STATUS _status;
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL COMMON[1] = { 0,0,0,0 };
	_status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid,
		NULL, (VOID**)&gGraphicsOutput);
	if (EFI_ERROR(_status)) {
		Print(L"draw Fail!!!");
	}

	for (int i = 0; i < 19; i++) {
		for (int j = 0; j < 15; j++) {
			switch (MAN_PIC[i][j]) {
			case 1:
				COMMON[0] = BLACK[0];
				break;
			case 2:
				COMMON[0] = LIGHT_BROWN[0];
				break;
			case 3:
				COMMON[0] = DARK_BROWN[0];
				break;
			case 4:
				COMMON[0] = SKIN[0];
				break;
			case 5:
				COMMON[0] = LIGHT_RED[0];
				break;
			case 6:
				COMMON[0] = WHITE[0];
				break;
			case 7:
				COMMON[0] = PINK[0];
				break;
			case 8:
				COMMON[0] = DARK_BLUE[0];
				break;
			case 9:
				COMMON[0] = LIGHT_BLUE[0];
				break;
			}
			
			_status = gGraphicsOutput->Blt(gGraphicsOutput,
				COMMON,
				EfiBltVideoFill,
				0, 0,
				j + 38, i + 4,
				1, 1,
				0);
		}
	}
}

void drawFloor() {

}