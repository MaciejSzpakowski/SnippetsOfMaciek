#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <math.h>

int addBlock();
int blockCollision(int dir);

typedef struct _block
{
	int rot;
	char type;
	float fall;
	COORD* fields;
	int count; //how many squares/fields there are
}block;
#define FPS 20
#define SIZE_X 10
#define SIZE_Y 24
#define ACTIVE 1
#define GAME_OVER 9
HANDLE g_consoleBuffer;
int g_keyPrev[256];
int g_key[256];
int g_gameTime;
float g_speed;
char g_board[SIZE_Y][SIZE_X];
char g_string[500];
char g_debugString[50];
int g_debugCount;
char g_auxStr[20];
int g_state;
int g_nextFlag; //since quick fall only if key is down, in order to prevent quick fall when new block is added, this flag is used
block* g_fallingBlock; //0 if there is no falling block, 1 if there is
int g_timer1;
int g_timer2;

#define debug(...) g_debugCount = sprintf(g_debugString,__VA_ARGS__)

int boardToString()
{
	int index = 0;	
	for(int i=0;i<SIZE_Y;i++)
	for(int j=0;j<SIZE_X;j++)
	{
		g_string[index] = !g_board[i][j] ? '.' : '#';
		for(int k=0;k<4;k++)
		{
			if(g_fallingBlock && g_fallingBlock->fields[k].X == j && g_fallingBlock->fields[k].Y == i)
				g_string[index] = '@';
			else if(g_fallingBlock && g_fallingBlock->fields[k].X == j && g_string[index] != '@')
				g_string[index] = !g_board[i][j] ? '-' : '#';
		}
		//at the end of this line insert \n
		if(j == SIZE_X-1)
			g_string[++index] = '\n';
		index++;
	}	
	//gameover
	if(g_state == GAME_OVER)
		memcpy(g_string+(SIZE_X+1)*10,"GAME  OVER",10);
	return index;
}

int render()
{
	COORD zero;
	zero.X = 0;
	zero.Y = 0;
	LPDWORD number; //leave this alone
	SetConsoleCursorPosition(g_consoleBuffer, zero);
	int length = boardToString();
	WriteConsole(g_consoleBuffer,g_string,length,number,NULL);
	//debug
	WriteConsole(g_consoleBuffer,g_debugString,g_debugCount,number,NULL);
	return 1;
}

int isKeyDown(int key)
{
	if(g_key[key])
		return 1;
	return 0;
}

int isKeyUp(int key)
{
	if(!g_key[key])
		return 1;
	return 0;
}

int isKeyPressed(int key)
{
	if(g_key[key] && !g_keyPrev[key])
		return 1;
	return 0;
}

int isKeyReleased(int key)
{
	if(!g_key[key] && g_keyPrev[key])
		return 1;
	return 0;
}

//checks if space is free so i can rotate
int tryRotate1(int y1,int x1,int y2,int x2,int y3,int x3,int y4,int x4)
{
	int arr[4][2] = {{y1,x1},{y2,x2},{y3,x3},{y4,x4}};
	for(int i=0;i<4;i++)
	{
		if(arr[i][0]<0 || arr[i][0] > SIZE_Y-1 || arr[i][1] < 0 || arr[i][1] > SIZE_X-1 ||
			g_board[arr[i][0]][arr[i][1]])
		{
			return 0;
		}
	}	
	g_fallingBlock->fields[0].Y = y1;
	g_fallingBlock->fields[0].X = x1;
	g_fallingBlock->fields[1].Y = y2;
	g_fallingBlock->fields[1].X = x2;
	g_fallingBlock->fields[2].Y = y3;
	g_fallingBlock->fields[2].X = x3;
	g_fallingBlock->fields[3].Y = y4;
	g_fallingBlock->fields[3].X = x4;
	return 1;
}

int rotate1()
{
	//natural relative coords
	#define coords(a,b,c,d,e,f,g,h) fields[0].Y-(b),fields[0].X+(a),fields[1].Y-(d),fields[1].X+(c),\
								fields[2].Y-(f),fields[2].X+(e),fields[3].Y-(h),fields[3].X+(g)
	char type = g_fallingBlock->type;
	COORD* fields = g_fallingBlock->fields;
	int* rot = &(g_fallingBlock->rot);
	if(type == 'O')
		return 0;
	if(type == 'I')
	{
		if(*rot == 0 && tryRotate1(coords(-2,-2,-1,-1,0,0,1,1)))
			*rot = 1;
		else if(*rot == 1 && tryRotate1(coords(1,2,0,1,-1,0,-2,-1)))
			*rot = 2;
		else if(*rot == 2 && tryRotate1(coords(-1,-1,0,0,1,1,2,2)))
			*rot = 3;
		else if(*rot == 3 && tryRotate1(coords(2,1,1,0,0,-1,-1,-2)))
			*rot = 0;			
	}
	if(type == 'L')
	{
		if(*rot == 0 && tryRotate1(coords(1,-1,0,-2,-1,-1,-2,0)))
			*rot = 1;
		else if(*rot == 1 && tryRotate1(coords(0,-1,-1,0,0,1,1,2)))
			*rot = 2;
		else if(*rot == 2 && tryRotate1(coords(-2,0,-1,1,0,0,1,-1)))
			*rot = 3;
		else if(*rot == 3 && tryRotate1(coords(1,2,2,1,1,0,0,-1)))
			*rot = 0;			
	}
	if(type == 'J')
	{
		if(*rot == 0 && tryRotate1(coords(0,-2,1,-1,0,0,-1,1)))
			*rot = 1;
		else if(*rot == 1 && tryRotate1(coords(-1,0,0,-1,1,0,2,1)))
			*rot = 2;
		else if(*rot == 2 && tryRotate1(coords(-1,1,-2,0,-1,-1,0,-2)))
			*rot = 3;
		else if(*rot == 3 && tryRotate1(coords(2,1,1,2,0,1,-1,0)))
			*rot = 0;
	}
	if(type == 'S')
	{
		if(*rot == 0 && tryRotate1(coords(-1,-2,0,-1,-1,0,0,1)))
			*rot = 1;
		else if(*rot == 1 && tryRotate1(coords(1,2,0,1,1,0,0,-1)))
			*rot = 0;
	}
	if(type == 'Z')
	{
		if(*rot == 0 && tryRotate1(coords(1,0,0,-1,-1,0,-2,-1)))
			*rot = 1;
		else if(*rot == 1 && tryRotate1(coords(-1,0,0,1,1,0,2,1)))
			*rot = 0;
	}
	if(type == 'T')
	{
		if(*rot == 0 && tryRotate1(coords(2,0,1,-1,0,-2,0,0)))
			*rot = 1;
		else if(*rot == 1 && tryRotate1(coords(0,-2,-1,-1,-2,0,0,0)))
			*rot = 2;
		else if(*rot == 2 && tryRotate1(coords(-2,0,-1,1,0,2,0,0)))
			*rot = 3;
		else if(*rot == 3 && tryRotate1(coords(0,2,1,1,2,0,0,0)))
			*rot = 0;
	}
	return 1;
}

int control()
{
	g_speed = 0.1f;
	int dir = 0; //-1 left, 1 right
	for(int i=0;i<256;i++)
	{
		g_keyPrev[i] = g_key[i];
		g_key[i] = GetAsyncKeyState(i);
	}
	if(isKeyDown(VK_ESCAPE))
		return 0;
	if(isKeyUp(VK_DOWN))
		g_nextFlag = 1;
	if(!g_fallingBlock)
		return 1;
	else if(isKeyPressed(VK_UP))
		rotate1();
	else if(isKeyDown(VK_DOWN) && g_nextFlag)
		g_speed = 1.4f;
	else if(isKeyPressed(VK_LEFT))
	{
		dir = -1;
		for(int i=0;i<g_fallingBlock->count;i++)
			g_fallingBlock->fields[i].X--;
	}
	else if(isKeyPressed(VK_RIGHT))
	{
		dir = 1;
		for(int i=0;i<g_fallingBlock->count;i++)
			g_fallingBlock->fields[i].X++;
	}
	if(g_fallingBlock)
	{
		blockCollision(dir);
	}
	return 1;
}

int blockFall()
{
	if(!g_fallingBlock)
		return 0;
	g_fallingBlock->fall += g_speed;
	if(g_fallingBlock->fall >= 1)
	{
		g_fallingBlock->fall = 0;
		for(int i=0;i<g_fallingBlock->count;i++)
			g_fallingBlock->fields[i].Y++;
	}
	return 1;
}

//check for lines
int tetris()
{
	//first elements will store how many lines are there to remove
	g_nextFlag = 0;
	int lines[4] = {-1,-1,-1,-1};
	int index = 0;
	for(int i=0;i<SIZE_Y;i++)
	for(int j=0;j<SIZE_X;j++)
	{
		if(!g_board[i][j])
			break;
		else if(j == SIZE_X-1)
		{			
			lines[index] = i;
			index++;
		}
	}
	for(int i=0;i<index;i++)
		if(lines[i] != -1)
		{			
			for(int j=0;j<SIZE_X;j++)
				g_board[lines[i]][j] = 0;
			//collapse for each line
			for(int k=lines[i];k>=0;k--)
			for(int j=0;j<SIZE_X;j++)
			{
				g_board[k][j] = g_board[k-1][j];
				g_board[k-1][j] = 0;
			}
		}
	return 1;
}

int blockCollision(int dir)
{	
	if(!g_fallingBlock)
		return -1;
	for(int i=0;i<g_fallingBlock->count;i++)
	{
		//if any is too far right, move back
		if(dir == 1)
		if(g_fallingBlock->fields[i].X>SIZE_X-1 || 
			g_board[g_fallingBlock->fields[i].Y][g_fallingBlock->fields[i].X])
		{
			for(int j=0;j<g_fallingBlock->count;j++)
				g_fallingBlock->fields[j].X--;
		}
		//if any is too far left, move back
		if(dir == -1)
		if(g_fallingBlock->fields[i].X<0 ||
			g_board[g_fallingBlock->fields[i].Y][g_fallingBlock->fields[i].X])
		{
			for(int j=0;j<g_fallingBlock->count;j++)
				g_fallingBlock->fields[j].X++;
		}
		//if ground level then stop
		if(dir == 0)
		if(g_fallingBlock->fields[i].Y == SIZE_Y)
		{
			for(int j=0;j<g_fallingBlock->count;j++)
					g_fallingBlock->fields[j].Y--;
			for(int j=0;j<g_fallingBlock->count;j++)
				g_board[g_fallingBlock->fields[j].Y][g_fallingBlock->fields[j].X] = 1;
			free(g_fallingBlock);
			free(g_fallingBlock->fields);
			g_fallingBlock = 0;
			tetris();
			return 1;
		}
		//if collision with bricks
		if(dir == 0)
		if(g_board[g_fallingBlock->fields[i].Y][g_fallingBlock->fields[i].X])
		{
			for(int j=0;j<g_fallingBlock->count;j++)
					g_fallingBlock->fields[j].Y--;
			for(int j=0;j<g_fallingBlock->count;j++)
				g_board[g_fallingBlock->fields[j].Y][g_fallingBlock->fields[j].X] = 1;
			free(g_fallingBlock);
			free(g_fallingBlock->fields);
			g_fallingBlock = 0;
			tetris();
			return 1;
		}
	}
	return 0;
}

int logic()
{
	if(g_state == GAME_OVER)
		return 0;
	if(!g_fallingBlock && addBlock() == GAME_OVER)
		return 0;
	else if(g_fallingBlock)
	{
		blockFall();
		blockCollision(0);
	}
	return 1;
}

int init()
{
	system("cls");
	srand(time(0));
	g_consoleBuffer = GetStdHandle( STD_OUTPUT_HANDLE );
	g_gameTime = clock();
	g_state = ACTIVE;
	for(int i=0;i<SIZE_Y;i++)
	for(int j=0;j<SIZE_X;j++)
		g_board[i][j] = 0;
	g_fallingBlock = 0;
	g_speed = 0.1f;
	for(int i=0;i<50;i++)
		g_debugString[i] = ' ';
	for(int i=0;i<20;i++)
		g_auxStr[i] = ' ';
	for(int i=0;i<256;i++)
	{
		g_keyPrev[i] = 0;
		g_key[i] = 1;
	}
	g_debugCount = 0;
	g_nextFlag = 1;
	return 1;
}

int addBlock()
{
	int type = rand()%7;
	g_fallingBlock = (block*)malloc(sizeof(block));
	g_fallingBlock->fall = 0;
	g_fallingBlock->rot = 0;
	g_speed = 0.1f;
	if(type == 0)
	{	//O
		g_fallingBlock->count = 4;
		g_fallingBlock->fields = (COORD*)malloc(sizeof(COORD)*4);
		g_fallingBlock->fields[0].X = 0;
		g_fallingBlock->fields[0].Y = 0;
		g_fallingBlock->fields[1].X = 1;
		g_fallingBlock->fields[1].Y = 0;
		g_fallingBlock->fields[2].X = 0;
		g_fallingBlock->fields[2].Y = 1;
		g_fallingBlock->fields[3].X = 1;
		g_fallingBlock->fields[3].Y = 1;
		g_fallingBlock->type = 'O';
	}
	if(type == 1)
	{
		//L
		g_fallingBlock->count = 4;
		g_fallingBlock->fields = (COORD*)malloc(sizeof(COORD)*4);
		g_fallingBlock->fields[0].X = 0;
		g_fallingBlock->fields[0].Y = 0;
		g_fallingBlock->fields[1].X = 1;
		g_fallingBlock->fields[1].Y = 0;
		g_fallingBlock->fields[2].X = 1;
		g_fallingBlock->fields[2].Y = 1;
		g_fallingBlock->fields[3].X = 1;
		g_fallingBlock->fields[3].Y = 2;
		g_fallingBlock->type = 'L';
	}
	if(type == 2)
	{
		//J
		g_fallingBlock->count = 4;
		g_fallingBlock->fields = (COORD*)malloc(sizeof(COORD)*4);
		g_fallingBlock->fields[0].X = 1;
		g_fallingBlock->fields[0].Y = 0;
		g_fallingBlock->fields[1].X = 0;
		g_fallingBlock->fields[1].Y = 0;
		g_fallingBlock->fields[2].X = 0;
		g_fallingBlock->fields[2].Y = 1;
		g_fallingBlock->fields[3].X = 0;
		g_fallingBlock->fields[3].Y = 2;
		g_fallingBlock->type = 'J';
	}
	if(type == 3)
	{
		//I
		g_fallingBlock->count = 4;
		g_fallingBlock->fields = (COORD*)malloc(sizeof(COORD)*4);
		g_fallingBlock->fields[0].X = 0;
		g_fallingBlock->fields[0].Y = 0;
		g_fallingBlock->fields[1].X = 0;
		g_fallingBlock->fields[1].Y = 1;
		g_fallingBlock->fields[2].X = 0;
		g_fallingBlock->fields[2].Y = 2;
		g_fallingBlock->fields[3].X = 0;
		g_fallingBlock->fields[3].Y = 3;
		g_fallingBlock->type = 'I';
	}
	if(type == 4)
	{
		//T
		g_fallingBlock->count = 4;
		g_fallingBlock->fields = (COORD*)malloc(sizeof(COORD)*4);
		g_fallingBlock->fields[0].X = 0;
		g_fallingBlock->fields[0].Y = 0;
		g_fallingBlock->fields[1].X = 1;
		g_fallingBlock->fields[1].Y = 0;
		g_fallingBlock->fields[2].X = 2;
		g_fallingBlock->fields[2].Y = 0;
		g_fallingBlock->fields[3].X = 1;
		g_fallingBlock->fields[3].Y = 1;
		g_fallingBlock->type = 'T';
	}
	if(type == 5)
	{
		//Z
		g_fallingBlock->count = 4;
		g_fallingBlock->fields = (COORD*)malloc(sizeof(COORD)*4);
		g_fallingBlock->fields[0].X = 0;
		g_fallingBlock->fields[0].Y = 0;
		g_fallingBlock->fields[1].X = 1;
		g_fallingBlock->fields[1].Y = 0;
		g_fallingBlock->fields[2].X = 1;
		g_fallingBlock->fields[2].Y = 1;
		g_fallingBlock->fields[3].X = 2;
		g_fallingBlock->fields[3].Y = 1;
		g_fallingBlock->type = 'Z';
	}
	if(type == 6)
	{
		//S
		g_fallingBlock->count = 4;
		g_fallingBlock->fields = (COORD*)malloc(sizeof(COORD)*4);
		g_fallingBlock->fields[0].X = 2;
		g_fallingBlock->fields[0].Y = 0;
		g_fallingBlock->fields[1].X = 1;
		g_fallingBlock->fields[1].Y = 0;
		g_fallingBlock->fields[2].X = 1;
		g_fallingBlock->fields[2].Y = 1;
		g_fallingBlock->fields[3].X = 0;
		g_fallingBlock->fields[3].Y = 1;
		g_fallingBlock->type = 'S';
	}
	//check if game over
	for(int i=0;i<g_fallingBlock->count;i++)
	{
		g_fallingBlock->fields[i].X+=4;		
		if(g_board[g_fallingBlock->fields[i].Y][g_fallingBlock->fields[i].X])
		{
			g_state = GAME_OVER;
			return GAME_OVER;
		}
	}
	return 1;
}

int main()
{
	init();
	while(1)
	{
		if(clock() - g_gameTime > (CLOCKS_PER_SEC/FPS))
		{
			logic();
			render();
			g_gameTime = clock();
		}
		if(!control())
			break;
	}
	return 0;
}