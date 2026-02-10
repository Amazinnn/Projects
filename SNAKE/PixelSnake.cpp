#include <stdio.h>
#include <stdbool.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <time.h>

#define WIDTH 80
#define HEIGHT 40


typedef struct node{
	int x;
	int y;
	struct node *next;
	struct node *last;
} node;

typedef struct food{
	int x;
	int y;
	long int existtime;
	struct food *next;
} food;


node *Head,*Tail;
int length=1;
char direction='d';
int speed=90;
int difficulty_level=1;
food *FoodHead,*FoodTail;
food *PoisonHead,*PoisonTail;
int SpawnBoundaryX=4,SpawnBoundaryY=4;
int FoodExistTime=120;
int PoisonExistTime=70;
int PossibilityToSpawnFood=90;
int PossibilityToSpawnPoison=150;
int level[5];

bool InSnake(int,int);
bool InFood(int,int);
bool InPoison(int,int);
bool NearSnake(int,int);

void LevelUp(void)
{
	bool Upgrade=false;
	for (int i=0;i<5;i++){
	if ((length>2&&length<=5&&level[0]==0)||
	(length>5&&length<=10&&level[1]==0)||
	(length>10&&length<=20&&level[2]==0)||
	(length>20&&length<=35&&level[3]==0)||
	(length>35&&level[4]==0)){
		Upgrade=true;
		break;
		level[i]=1;
		}
	}
	if (Upgrade){
		PossibilityToSpawnFood=(int)ceil(PossibilityToSpawnFood/1.15);
		PossibilityToSpawnPoison=(int)ceil(PossibilityToSpawnPoison/1.13);
		FoodExistTime=ceil(FoodExistTime/1.01);
		PoisonExistTime=ceil(PoisonExistTime/1.03);
	}

}


void InitSnake(void)
{
	Head=(node *)malloc(sizeof(node));
	Head->last=NULL;
	node *FirstNode=(node *)malloc(sizeof(node));
	FirstNode->last=Head;
	Head->next=FirstNode;
	FirstNode->next=NULL;
	FirstNode->x=WIDTH/2;
	FirstNode->y=HEIGHT/2;
	Tail=FirstNode;
}

node *CreateNode(void)
{
	node *NewNode=(node *)malloc(sizeof(node));
	node *FormerHead=Head->next;
	int i,dir[4][2]={-1,0,1,0,0,-1,0,1};
	switch (direction){
		case 'a':i=0;break;
		case 'd':i=1;break;
		case 'w':i=2;break;
		case 's':i=3;break;
	}
	NewNode->x=FormerHead->x+dir[i][0];
	NewNode->y=FormerHead->y+dir[i][1];
	FormerHead->last=NewNode;
	NewNode->next=FormerHead;
	Head->next=NewNode;
	NewNode->last=Head;
	return NewNode;
}

void RemoveNode(void)
{
	Tail=Tail->last;
	free(Tail->next);
	Tail->next=NULL;
}

char Move(char op)
{
	if (direction=='a'&&op=='d'||
	direction=='d'&&op=='a'||
	direction=='w'&&op=='s'||
	direction=='s'&&op=='w') ;
	else if (op=='a'||op=='d'||op=='w'||op=='s') direction=op;
	node *NewNode=CreateNode();
	RemoveNode();
	return direction;
}

bool InSnake(int x,int y)
{
	node *curr=Head->next;
	while (curr!=NULL&&!(curr->x==x&&curr->y==y)){
		curr=curr->next;
	}
	if (curr==NULL) return false;
	else return true;
}

bool NearSnake(int x,int y)
{
	node *head=Head->next;
	for (int i=-2;i<=2;i++){
		for (int j=-2;j<=2;j++){
			if (x+i==head->x&&y+j==head->y){
				return true;
			}
		}
	}
	return false;
}

bool IsHead(int x,int y)
{
	node *head=Head->next;
	return ((x==head->x)&&(y==head->y));
}


void InitUI(void)
{
    system("cls");
    for (int y = 1; y <= HEIGHT; y++) {
        for (int x = 1; x <= WIDTH; x++) {
            if (x == 1 || x == WIDTH || y == 1 || y == HEIGHT) {
                printf("#");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
}

void getCursorPosition(int* x, int* y) {
    CONSOLE_SCREEN_BUFFER_INFO csbi; // 用于存储控制台屏幕信息的变量
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE); // 获取标准输出句柄

    if (GetConsoleScreenBufferInfo(hStdOut, &csbi)) {
        // 成功获取信息后，从结构体中提取光标坐标
        *x = csbi.dwCursorPosition.X;
        *y = csbi.dwCursorPosition.Y;
    } else {
        // 如果获取失败，可以返回一个错误值（如-1）
        *x = -1;
        *y = -1;
    }
}

void setCursorPosition(int x, int y) 
{
    COORD coord;
    coord.X = x - 1;
    coord.Y = y - 1;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void hideCursor() 
{
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 100;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}


void InitPoison(void)
{
	PoisonHead=(food *)malloc(sizeof(food));
	PoisonTail=(food *)malloc(sizeof(food));
	PoisonHead->next=NULL;
	PoisonTail=PoisonHead;
}

void SpawnPoison(int num)
{
	int X=(num%(WIDTH-2*SpawnBoundaryX))+SpawnBoundaryX;
	int Y=(num/(WIDTH-2*SpawnBoundaryX))+SpawnBoundaryY;
	if (InSnake(X,Y)||InFood(X,Y)||InPoison(X,Y)||NearSnake(X,Y)) return ;
	food *NewPoison=(food *)malloc(sizeof(food));
	NewPoison->existtime=PoisonExistTime;
	NewPoison->next=NULL;
	NewPoison->x=X;
	NewPoison->y=Y;
	PoisonTail->next=NewPoison;
	PoisonTail=NewPoison;
	setCursorPosition(PoisonTail->x,PoisonTail->y);
	printf("*");
}

void PoisonCountDown(void)
{
	food *curr=PoisonHead->next,*prev=PoisonHead;
	while (curr!=NULL){
		curr->existtime--;
		if (curr->existtime<=0){
			food *Delete=curr;
			curr=curr->next;
			prev->next=curr;
			if (Delete==PoisonTail) PoisonTail=prev;
			setCursorPosition(Delete->x,Delete->y);
			printf(" ");
			free(Delete);
		}
		else {
			prev=curr;
			curr=curr->next;
		}
	}
}

bool InPoison(int x,int y)
{
	food *curr=PoisonHead->next;
	while (curr!=NULL){
		if (curr->x==x&&curr->y==y)
			return true;
		curr=curr->next;
	}
	return false;
}

bool EatPoison()
{
	node *head=Head->next;
	return (InPoison(head->x,head->y));
}


void InitFood(void)
{
	FoodHead=(food *)malloc(sizeof(food));
	FoodTail=(food *)malloc(sizeof(food));
	FoodHead->next=NULL;
	FoodTail=FoodHead;
}

void SpawnFood(int num)
{
	int X=(num%(WIDTH-2*SpawnBoundaryX))+SpawnBoundaryX;
	int Y=(num/(WIDTH-2*SpawnBoundaryX))+SpawnBoundaryY;
	if (InSnake(X,Y)||InFood(X,Y)||InPoison(X,Y)||NearSnake(X,Y)) return ;
	food *NewFood=(food *)malloc(sizeof(food));
	NewFood->existtime=FoodExistTime;
	NewFood->next=NULL;
	NewFood->x=X;
	NewFood->y=Y;
	FoodTail->next=NewFood;
	FoodTail=NewFood;
	setCursorPosition(FoodTail->x,FoodTail->y);
	printf("@");
}

void FoodCountDown(void)
{
	food *curr=FoodHead->next,*prev=FoodHead;
	while (curr!=NULL){
		curr->existtime--;
		if (curr->existtime<=0){
			food *Delete=curr;
			curr=curr->next;
			prev->next=curr;
			if (Delete==FoodTail) FoodTail=prev;
			setCursorPosition(Delete->x,Delete->y);
			printf(" ");
			free(Delete);
		}
		else {
			prev=curr;
			curr=curr->next;
		}
	}
}

bool EatFood(node *head)
{
	food *curr=FoodHead->next,*prev=FoodHead;
	while (curr!=NULL){
		if (curr->x==head->x&&curr->y==head->y){
			food *Delete=curr;
			curr=curr->next;
			prev->next=curr;
			if (Delete==FoodTail) FoodTail=prev;
			free(Delete);
			return true;
		}
		else {
			prev=curr;
			curr=curr->next;
		}
	}
	return false;
}

bool InFood(int x,int y)
{
	food *curr=FoodHead->next;
	while (curr!=NULL){
		if (curr->x==x&&curr->y==y)
			return true;
		curr=curr->next;
	}
	return false;
}




bool HitWall(void)
{
	node *head=Head->next;
	if (head->x==1||head->x==WIDTH) return true;
	else if (head->y==1||head->y==HEIGHT) return true;
	else return false;
}

bool BiteTail(void)
{
	node *head=Head->next,*curr=head->next;
	while (curr!=NULL){
		if (curr->x==head->x&&curr->y==head->y) return true;
		curr=curr->next;
	}
	return false;
}



void InitializeEVERYTHING(void)
{
	speed=90;
	switch (difficulty_level){
		case 0:{
			SpawnBoundaryX=8,SpawnBoundaryY=8;
			FoodExistTime=150;
			PoisonExistTime=70;
			PossibilityToSpawnFood=60;
			PossibilityToSpawnPoison=1000000;
			break;
		}
		case 1:{
			SpawnBoundaryX=4,SpawnBoundaryY=4;
			FoodExistTime=120;
			PoisonExistTime=70;
			PossibilityToSpawnFood=90;
			PossibilityToSpawnPoison=150;
			break;
		}
		case 2:{
			SpawnBoundaryX=2,SpawnBoundaryY=2;
			FoodExistTime=100;
			PoisonExistTime=150;
			PossibilityToSpawnFood=100;
			PossibilityToSpawnPoison=80;
			break;
		}
		case 3:{
			SpawnBoundaryX=1,SpawnBoundaryY=1;
			FoodExistTime=80;
			PoisonExistTime=120;
			PossibilityToSpawnFood=180;
			PossibilityToSpawnPoison=20;
			break;
		}
	}
	for (int i=0;i<5;i++) level[i]=0;
	direction='d';
	InitFood();
	InitPoison();
	InitUI();
	InitSnake();
	hideCursor();
	length=1;
}

int StartMenu(void)
{
	int choice=1;
	char input;
	char GameName[]={"- P I X E L   S N A K E -"};
	char start[]={"       S T A R T      "};
	char settings[]={"     S E T T I N G S     "};
	char end[]={"         E N D         "};
	char prompt1[]={"Press W or S to choose"};
	char prompt2[]={"Press enter to confirm your choice"};
	int startX,startY,settingsX,settingsY,endX,endY;
	system("cls");
	for (int i=1;i<=HEIGHT/4;i++) printf("\n");
	for (int i=1;i<=HEIGHT/2;i++){
		if (i==1){
			for (int j=1;j<=WIDTH/4-1;j++) printf(" ");
			printf(GameName);
			printf("\n");
		}
		else if (i==HEIGHT/6+3){
			for (int j=1;j<=WIDTH/4;j++) printf(" ");
			getCursorPosition(&startX,&startY);
			printf("%s",start);
			printf("\n");
		}
		else if (i==HEIGHT/3){
			for (int j=1;j<=WIDTH/4;j++) printf(" ");
			getCursorPosition(&settingsX,&settingsY);
			printf("%s",settings);
			printf("\n");
		}
		else if (i==HEIGHT/2-3){
			for (int j=1;j<=WIDTH/4;j++) printf(" ");
			getCursorPosition(&endX,&endY);
			printf("%s",end);
			printf("\n");
		}
		else printf("\n");
	}
	for (int i=1;i<=HEIGHT/10;i++) printf("\n");
	for (int j=1;j<=WIDTH/4;j++) printf(" ");
	printf("%s",prompt1);
	for (int i=1;i<=2;i++) printf("\n");
	for (int j=1;j<=WIDTH/6+1;j++) printf(" ");
	printf("%s",prompt2);
	startX++,startY++,settingsX++,settingsY++,endX++,endY++;
	
	while (1){
		if (choice==1){
			setCursorPosition(startX,startY);
			Sleep(200);
			for (int i=1;i<=strlen(start);i++) printf(" ");
			Sleep(200);
			setCursorPosition(startX,startY);
			printf("%s",start);
		}
		else if (choice==2){
			setCursorPosition(settingsX,settingsY);
			Sleep(200);
			for (int i=1;i<=strlen(settings);i++) printf(" ");
			Sleep(200);
			setCursorPosition(settingsX,settingsY);
			printf("%s",settings);
		}
		else if (choice==3){
			setCursorPosition(endX,endY);
			Sleep(200);
			for (int i=1;i<=strlen(end);i++) printf(" ");
			Sleep(200);
			setCursorPosition(endX,endY);
			printf("%s",end);
		}
		if (_kbhit()){
			input=_getch();
			if (choice==3&&input=='w') choice=2;
			else if (choice==2&&input=='w') choice=1;
			else if (choice==1&&input=='s') choice=2;
			else if (choice==2&&input=='s') choice=3;
			else if (input=='\r') break;
		}
	}
	return choice;
}

int GameOverMenu(void)
{
	int choice=1;
	char input;
	char tryagain[]={"   T R Y     A G A I N   "};
	char returnmenu[]={"   R E T U R N   T O   S T A R T   M E N U   "};
	char quitgame[]={"  Q U I T   T H E   G A M E   "};
	char prompt1[]={"Press W or S to choose"};
	char prompt2[]={"Press enter to confirm your choice"};
	int tryX,tryY,returnX,returnY,quitX,quitY;
	
	system("cls");
	for (int i=1;i<=HEIGHT/4;i++) printf("\n");
	for (int i=1;i<=WIDTH/4;i++) printf(" ");
	printf("Final score : %d\n",length);
	
	for (int i=1;i<=HEIGHT/8;i++) printf("\n");
	for (int i=1;i<=WIDTH/4;i++) printf(" ");
	if (length<=2) printf("Practice makes perfect!");
	else if (length<=5) printf("Not bad!");
	else if (length<=10) printf("Well done!");
	else if (length<=20) printf("Excellent!");
	else printf("Legendary!");
	
	for (int i=1;i<=HEIGHT/8;i++) printf("\n");
	for (int j=1;j<=WIDTH/4;j++) printf(" ");
	getCursorPosition(&tryX,&tryY);
	printf("%s",tryagain);
	printf("\n\n\n");
	
	for (int j=1;j<=WIDTH/4;j++) printf(" ");
	getCursorPosition(&returnX,&returnY);
	printf("%s",returnmenu);
	printf("\n\n\n");
	
	for (int j=1;j<=WIDTH/4;j++) printf(" ");
	getCursorPosition(&quitX,&quitY);
	printf("%s",quitgame);
	printf("\n\n");
	
	for (int i=1;i<=HEIGHT/8;i++) printf("\n");
	for (int j=1;j<=WIDTH/4;j++) printf(" ");
	printf("%s",prompt1);
	for (int i=1;i<=2;i++) printf("\n");
	for (int j=1;j<=WIDTH/6+1;j++) printf(" ");
	printf("%s",prompt2);
	
	tryX++,tryY++,returnX++,returnY++,quitX++,quitY++;
	
	while (1){
		if (choice==1){
			setCursorPosition(tryX,tryY);
			Sleep(200);
			for (int i=1;i<=strlen(tryagain);i++) printf(" ");
			Sleep(200);
			setCursorPosition(tryX,tryY);
			printf("%s",tryagain);
		}
		else if (choice==2){
			setCursorPosition(returnX,returnY);
			Sleep(200);
			for (int i=1;i<=strlen(returnmenu);i++) printf(" ");
			Sleep(200);
			setCursorPosition(returnX,returnY);
			printf("%s",returnmenu);
		}
		else if (choice==3){
			setCursorPosition(quitX,quitY);
			Sleep(200);
			for (int i=1;i<=strlen(quitgame);i++) printf(" ");
			Sleep(200);
			setCursorPosition(quitX,quitY);
			printf("%s",quitgame);
		}
		if (_kbhit()){
			input=_getch();
			if (choice==3&&input=='w') choice=2;
			else if (choice==2&&input=='w') choice=1;
			else if (choice==1&&input=='s') choice=2;
			else if (choice==2&&input=='s') choice=3;
			else if (input=='\r') break;
		}
	}
	return choice;
}


void Gaming(void)
{
	system("cls");
	InitializeEVERYTHING();
	srand((unsigned int)time(NULL));
	int lastTailX=0,lastTailY=0;
	while (1){

		char op='\0';
		if (_kbhit()){
			op=_getch();
			while (_kbhit()) _getch();
		}
		if (op=='q') break;
		else if (op=='p'){
			while (1){
				if (_kbhit()){
					op=_getch();
				}
				if (op=='c') break;
			}
		}
		
		lastTailX=Tail->x,lastTailY=Tail->y;
		direction=Move(op);
		FoodCountDown();
		PoisonCountDown();
		LevelUp();

		if (EatFood(Head->next)){
			length++;
			setCursorPosition(Head->next->x,Head->next->y);
			printf("O");
			CreateNode();
		}
		
		
		if (HitWall()||BiteTail()||EatPoison()) break;
		
		
		if (lastTailX>0&&lastTailY>0&&!InSnake(lastTailX,lastTailY)){
			setCursorPosition(lastTailX,lastTailY);
			printf(" ");
		}
		setCursorPosition(Head->next->x,Head->next->y);
		printf("O");
		
		if (rand()%PossibilityToSpawnFood==0){
			SpawnFood(rand()%((HEIGHT-2*SpawnBoundaryY)*(WIDTH-2*SpawnBoundaryX)));
		}
		
		if (rand()%PossibilityToSpawnPoison==0){
			SpawnPoison(rand()%((HEIGHT-2*SpawnBoundaryY)*(WIDTH-2*SpawnBoundaryX)));
		}
		
		
		Sleep(speed);
	}
}


int SettingMenu(void)
{
	int choice=1;
	char input;
	char speedOption[]={"   S N A K E   M O V E   S P E E D   "};
	char difficulty[]={"   G A M E   D I F F I C U L T Y   "};
	char returnmenu[]={"   R E T U R N   T O   S T A R T   M E N U   "};
	char prompt1[]={"Press W or S to choose"};
	char prompt2[]={"Press D or enter to edit, A or enter to confirm"};
	char editing=0;
	int editField=0;
	int tempSpeed=speed;
	int tempDifficulty=difficulty_level;
	char* diffText[]={"EASY","MEDIUM","HARD","EXPERT"};
	int speedX,speedY,diffX,diffY,returnX,returnY;
	
	system("cls");
	for(int i=1;i<=HEIGHT/4;i++) printf("\n");
	for(int i=1;i<=WIDTH/4;i++) printf(" ");
	printf("S E T T I N G S\n");
	
	for(int i=1;i<=HEIGHT/8;i++) printf("\n");
	for(int j=1;j<=WIDTH/4;j++) printf(" ");
	getCursorPosition(&speedX,&speedY);
	printf("%s",speedOption);
	for(int j=1;j<=10;j++) printf(" ");
	printf(" %d ms ",tempSpeed);
	
	printf("\n\n");
	for(int j=1;j<=WIDTH/4;j++) printf(" ");
	getCursorPosition(&diffX,&diffY);
	printf("%s",difficulty);
	for(int j=1;j<=10;j++) printf(" ");
	printf(" %s ",diffText[tempDifficulty]);
	
	printf("\n\n");
	for(int j=1;j<=WIDTH/4;j++) printf(" ");
	getCursorPosition(&returnX,&returnY);
	printf("%s",returnmenu);
	
	for(int i=1;i<=HEIGHT/8;i++) printf("\n");
	for(int j=1;j<=WIDTH/4;j++) printf(" ");
	printf("%s",prompt1);
	for(int i=1;i<=2;i++) printf("\n");
	for(int j=1;j<=WIDTH/6+1;j++) printf(" ");
	printf("%s",prompt2);
	
	speedX++;speedY++;diffX++;diffY++;returnX++;returnY++;
	
	while(1){
		if(!editing){
			if(choice==1){
				setCursorPosition(speedX,speedY);
				Sleep(200);
				for(int i=1;i<=strlen(speedOption)+17;i++) printf(" ");
				Sleep(200);
				setCursorPosition(speedX,speedY);
				printf("%s",speedOption);
				setCursorPosition(speedX+strlen(speedOption)+10,speedY);
				printf(" %d ms ",tempSpeed);
			}
			else if(choice==2){
				setCursorPosition(diffX,diffY);
				Sleep(200);
				for(int i=1;i<=strlen(difficulty)+17;i++) printf(" ");
				Sleep(200);
				setCursorPosition(diffX,diffY);
				printf("%s",difficulty);
				setCursorPosition(diffX+strlen(difficulty)+10,diffY);
				printf(" %s ",diffText[tempDifficulty]);
			}
			else if(choice==3){
				setCursorPosition(returnX,returnY);
				Sleep(200);
				for(int i=1;i<=strlen(returnmenu);i++) printf(" ");
				Sleep(200);
				setCursorPosition(returnX,returnY);
				printf("%s",returnmenu);
			}
		}
		
		if(_kbhit()){
			input=_getch();
			if(!editing){
				if(choice==3&&input=='w') choice=2;
				else if(choice==2&&input=='w') choice=1;
				else if(choice==1&&input=='s') choice=2;
				else if(choice==2&&input=='s') choice=3;
				else if(input=='d'||input=='\r'){
					editing=1;
					editField=choice;
					if(choice==3){
						speed=tempSpeed;
						difficulty_level=tempDifficulty;
						return 0;
					}
					
					if(choice==1){
						setCursorPosition(speedX+strlen(speedOption)+10,speedY);
						printf("              ");
						setCursorPosition(speedX+strlen(speedOption)+10,speedY);
						printf("[%d ms]",tempSpeed);
					}
					else if(choice==2){
						setCursorPosition(diffX+strlen(difficulty)+10,diffY);
						printf("              ");
						setCursorPosition(diffX+strlen(difficulty)+10,diffY);
						printf("[%s]",diffText[tempDifficulty]);
					}
				}
			}else{
				if(editField==1){
					if(input>='0'&&input<='9'){
						if(tempSpeed==0) tempSpeed=input-'0';
						else tempSpeed=tempSpeed*10+(input-'0');
						if(tempSpeed>1000) tempSpeed=1000;
					}else if(input=='\b'){
						tempSpeed=tempSpeed/10;
					}
					setCursorPosition(speedX+strlen(speedOption)+10,speedY);
					printf("              ");
					setCursorPosition(speedX+strlen(speedOption)+10,speedY);
					printf("[%d ms]",tempSpeed);
				}
				else if(editField==2){
					if(input=='w'){
						tempDifficulty=(tempDifficulty+3)%4;
						setCursorPosition(diffX+strlen(difficulty)+10,diffY);
						printf("              ");
						setCursorPosition(diffX+strlen(difficulty)+10,diffY);
						printf("[%s]",diffText[tempDifficulty]);
					}
					else if(input=='s'){
						tempDifficulty=(tempDifficulty+1)%4;
						setCursorPosition(diffX+strlen(difficulty)+10,diffY);
						printf("              ");
						setCursorPosition(diffX+strlen(difficulty)+10,diffY);
						printf("[%s]",diffText[tempDifficulty]);
					}
				}
				
				if(input=='a'||input=='\r'){
					editing=0;
					
					if(editField==1){
						if(tempSpeed<10) tempSpeed=10;
						if(tempSpeed>1000) tempSpeed=1000;
						speed=tempSpeed;
						setCursorPosition(speedX+strlen(speedOption)+10,speedY);
						printf("              ");
						setCursorPosition(speedX+strlen(speedOption)+10,speedY);
						printf(" %d ms ",tempSpeed);
					}
					else if(editField==2){
						difficulty_level=tempDifficulty;
						setCursorPosition(diffX+strlen(difficulty)+10,diffY);
						printf("              ");
						setCursorPosition(diffX+strlen(difficulty)+10,diffY);
						printf(" %s ",diffText[tempDifficulty]);
					}
				}
			}
		}
	}
}




int main(void)
{
	while (1){
		int startChoice = StartMenu();
		if (startChoice == 1) {
			game: Gaming();
			int gameOverChoice = GameOverMenu();
			if (gameOverChoice == 1) goto game; // TRY AGAIN
			else if (gameOverChoice == 2) continue; // RETURN TO START MENU
			else if (gameOverChoice == 3) break; // QUIT THE GAME
		}
		else if (startChoice == 2) {
			SettingMenu();
		}
		else if (startChoice == 3) {
			break;
		}
	}
	printf("\nHave a good day!");
	return 0;
}





