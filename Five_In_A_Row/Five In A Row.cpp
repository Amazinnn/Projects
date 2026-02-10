/*Five In A Row*/
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include <conio.h>
#include <stdbool.h>
#include <graphics.h> 
#define LENGTH 15
#define INTMAX 0x3f3f3f3f

typedef struct {
	int table[LENGTH][LENGTH];
} T;

int step=8;
int l5=100000,
	h4=10000,
	c4=5000,
	h3=2000,
	m3=800,
	h2=100,
	m2=10;
IMAGE back_pic,
	  whitechess,
	  blackchess;
bool PlayerChess;			//1白0黑 
bool move_first;
int dis=500/(LENGTH+1);



void GetAIChess(T,int*,int*);
bool IsNear(int,int,T);
int minimax(T,int,int,int,bool);
int Evaluate_Board(T);
bool InBoard(int x,int y);


void GetAIChess(T curr, int *x, int *y) {
    int bestX = -1, bestY = -1;
    int bestScore = -INTMAX;
    
    // 生成候选移动
    typedef struct {
        int x, y, score;
    } Move;
    
    Move moves[LENGTH * LENGTH];
    int moveCount = 0;
    
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < LENGTH; j++) {
            if (curr.table[i][j] == 0 && IsNear(i, j, curr)) {
                moves[moveCount].x = i;
                moves[moveCount].y = j;
                moveCount++;
            }
        }
    }
    
    // 如果没有候选，选择中心位置
    if (moveCount == 0) {
        *x = LENGTH / 2;
        *y = LENGTH / 2;
        return;
    }
    
    // 对每个候选位置进行minimax搜索
    for (int m = 0; m < moveCount; m++) {
        int i = moves[m].x, j = moves[m].y;
        
        curr.table[i][j] = 1; // AI落子
        int score = minimax(curr, step, -INTMAX, INTMAX, false);
        curr.table[i][j] = 0;
        
        if (score > bestScore) {
            bestScore = score;
            bestX = i;
            bestY = j;
        }
    }
    
    *x = bestX;
    *y = bestY;
}



/*previous version
void GetAIChess(T curr,int *x,int *y)
{
	int bestX,bestY;
	int bestScore=Evaluate_Board(curr);
	for (int i=0;i<LENGTH;i++){
		for (int j=0;j<LENGTH;j++){
			if (IsNear(i,j,curr)){
				curr.table[i][j]=1;
				int score=minimax(curr,step,-INT_MAX,INT_MAX,1);
				if (score>bestScore){
					bestScore=score;
					bestX=i,bestY=j;
				}
				curr.table[i][j]=0;
			}
		}
	}
	*x=bestX,*y=bestY;
}
*/



bool IsNear(int x,int y,T board)
{
	for (int i=-1;i<=1;i++){
		for (int j=-1;j<=1;j++){
			if (
			x+i>=0&&x+i<LENGTH&&
			y+j>=0&&y+j<LENGTH&&
			board.table[x][y]==0&&
			board.table[x+i][y+j]!=0
			){
				return true;
			}
		}
	}
	return false;
}

bool InBoard(int x,int y)
{
	return (x>=0&&x<LENGTH&&y>=0&&y<LENGTH);
}


int IsGameOver(T board)
{
    int dir[4][2]={{0,1},{1,0},{1,1},{1,-1}};
    for(int i=0;i<LENGTH;i++){
        for(int j=0;j<LENGTH;j++){
            if(board.table[i][j]==0)continue;
            int player=board.table[i][j];
            for(int k=0;k<4;k++){
                int count=1;
                int x=i+dir[k][0],y=j+dir[k][1];
                while(x>=0&&x<LENGTH&&y>=0&&y<LENGTH&&board.table[x][y]==player){
                    count++;
                    x+=dir[k][0];
                    y+=dir[k][1];
                }
                if(count>=5)return player;
            }
        }
    }
    return 0;
}


int minimax(T board, int depth, int alpha, int beta, bool IsMaximizingPlayer) {
    if (depth == 0 || IsGameOver(board)) return Evaluate_Board(board);
    
    typedef struct { int x, y, score; } Move;
    Move moves[LENGTH * LENGTH];
    int moveCount = 0;
    
    // 生成候选移动
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < LENGTH; j++) {
            if (board.table[i][j] == 0 && IsNear(i, j, board)) {
                moves[moveCount].x = i; moves[moveCount].y = j;
                board.table[i][j] = IsMaximizingPlayer ? 1 : -1;
                moves[moveCount].score = Evaluate_Board(board);
                board.table[i][j] = 0;
                moveCount++;
            }
        }
    }
    
    if (moveCount == 0) return Evaluate_Board(board);
    
    // 排序移动
    for (int i = 0; i < moveCount - 1; i++) {
        for (int j = i + 1; j < moveCount; j++) {
            if ((IsMaximizingPlayer && moves[i].score < moves[j].score) ||
                (!IsMaximizingPlayer && moves[i].score > moves[j].score)) {
                Move temp = moves[i]; moves[i] = moves[j]; moves[j] = temp;
            }
        }
    }
    
    int searchLimit = (moveCount > 8) ? 8 : moveCount;
    
    if (IsMaximizingPlayer) {
        int bestValue = -INTMAX;
        for (int m = 0; m < searchLimit; m++) {
            int i = moves[m].x, j = moves[m].y;
            board.table[i][j] = 1;
            int value = minimax(board, depth - 1, alpha, beta, false);
            board.table[i][j] = 0;
            if (value > bestValue) bestValue = value;
            if (value > alpha) alpha = value;
            if (alpha >= beta) break;
        }
        return bestValue;
    } else {
        int bestValue = INTMAX;
        for (int m = 0; m < searchLimit; m++) {
            int i = moves[m].x, j = moves[m].y;
            board.table[i][j] = -1;
            int value = minimax(board, depth - 1, alpha, beta, true);
            board.table[i][j] = 0;
            if (value < bestValue) bestValue = value;
            if (value < beta) beta = value;
            if (alpha >= beta) break;
        }
        return bestValue;
    }
}

/*previous version
int minimax(
T board,int depth,
int alpha,int beta,
bool IsMaximazingPlayer)
{
	int bestValue;
	if (depth==0||IsGameOver(board)){
		return Evaluate_Board(board);
	}
	else if (IsMaximazingPlayer){
		bestValue=-INTMAX;
		for (int i=0;i<LENGTH;i++){
			for (int j=0;j<LENGTH;j++){
				if (IsNear(i,j,board)){
					board.table[i][j]=1;
					int temp=minimax(board,depth-1,alpha,beta,!IsMaximazingPlayer);
					if (temp>bestValue) bestValue=temp;
					if (temp>alpha) alpha=temp;
					if (alpha>=beta) goto end;
					board.table[i][j]=0;
				}
			}
		}
	}
	else {
		bestValue=INTMAX;
		for (int i=0;i<LENGTH;i++){
			for (int j=0;j<LENGTH;j++){
				if (IsNear(i,j,board)){
					board.table[i][j]=-1;
					int temp=minimax(board,depth-1,alpha,beta,!IsMaximazingPlayer);
					if (temp<bestValue) bestValue=temp;
					if (temp<beta) beta=temp;
					if (alpha>=beta) goto end;
					board.table[i][j]=0;
				}
			}
		}
	}
	end:
		return bestValue;
}
*/


// 简化的评估函数，专注于关键棋形
int Evaluate_Board(T board) {
    int score = 0;
    
    // 检查四个方向：水平、垂直、两个对角线
    int dir[4][2] = {{0,1}, {1,0}, {1,1}, {1,-1}};
    
    for (int i = 0; i < LENGTH; i++) {
        for (int j = 0; j < LENGTH; j++) {
            if (board.table[i][j] != 0) {
                int player = board.table[i][j];
                
                for (int d = 0; d < 4; d++) {
                    int dx = dir[d][0], dy = dir[d][1];
                    
                    // 检查连续棋子
                    int count = 1;
                    int block_ends = 0;
                    
                    // 向前检查
                    int x = i + dx, y = j + dy;
                    while (InBoard(x, y) && board.table[x][y] == player) {
                        count++;
                        x += dx; y += dy;
                    }
                    // 检查是否被阻挡
                    if (!InBoard(x, y) || board.table[x][y] != 0) {
                        block_ends++;
                    }
                    
                    // 向后检查
                    x = i - dx; y = j - dy;
                    while (InBoard(x, y) && board.table[x][y] == player) {
                        count++;
                        x -= dx; y -= dy;
                    }
                    // 检查是否被阻挡
                    if (!InBoard(x, y) || board.table[x][y] != 0) {
                        block_ends++;
                    }
                    
                    // 根据连续数量和阻挡情况评分
                    if (count >= 5) {
                        score += (player == 1) ? l5 : -l5;
                    } else if (count == 4) {
                        if (block_ends == 0) score += (player == 1) ? h4 : -h4;  // 活四
                        else if (block_ends == 1) score += (player == 1) ? c4 : -c4; // 冲四
                    } else if (count == 3) {
                        if (block_ends == 0) score += (player == 1) ? h3 : -h3;  // 活三
                        else if (block_ends == 1) score += (player == 1) ? m3 : -m3; // 眠三
                    } else if (count == 2) {
                        if (block_ends == 0) score += (player == 1) ? h2 : -h2;  // 活二
                        else if (block_ends == 1) score += (player == 1) ? m2 : -m2; // 眠二
                    }
                }
            }
        }
    }
    return score;
}


/*previous version
int Evaluate_Board(T board)
{
	int maxiScore=0,miniScore=0;
	int dir[8][2]={{0,1},{0,-1},{1,0},{-1,0},{1,1},{-1,-1},{-1,1},{1,-1}};
	int scanned[LENGTH][LENGTH][4]={0};
	for (int i=0;i<LENGTH;i++){
		for (int j=0;j<LENGTH;j++){
			if (IsNear(i,j,board)){
				for (int k=0;k<8;k++){
					int distance=0,
						startX=i,
						startY=j,
						Player=0;
						if (!InBoard(i+dir[k][0],j+dir[k][1])||
						board.table[i+dir[k][0]][j+dir[k][1]]==0||
						scanned[i+dir[k][0]][j+dir[k][1]][k/2]==1)
							continue ;
					while (++distance<=6){
						startX=i+distance*dir[k][0];
						startY=j+distance*dir[k][1];
						if (!InBoard(startX,startY)){
								break;
						}
						Player=(Player==0&&board.table[startX][startY]!=0)?
								(board.table[startX][startY]):Player;
						if (Player!=0&&board.table[startX][startY]!=Player)
							break;
					}
					for (int m=1;m<=distance-1;m++){
						scanned[i+m*dir[k][0]][j+m*dir[k][1]][k/2]=1;
					}
					distance--;
					if (Player==1) switch (distance){
						case 5:{
							maxiScore+=l5;
							break;
						}
						case 4:{
							if (InBoard(startX,startY)&&
							board.table[startX][startY]==0)
								maxiScore+=h4;
							else maxiScore+=c4;
							break;
						}
						case 3:{
							if (InBoard(startX,startY)&&
							board.table[startX][startY]==0)
								maxiScore+=h3;
							else maxiScore+=m3;
							break;
						}
						case 2:{
							if (InBoard(startX,startY)&&
							board.table[startX][startY]==0)
								maxiScore+=h2;
							else maxiScore+=m2;
							break;
						}
					}
					else switch (distance){
						case 5:{
							miniScore+=l5;
							break;
						}
						case 4:{
							if (InBoard(startX,startY)&&
							board.table[startX][startY]==0)
								miniScore+=h4;
							else miniScore+=c4;
							break;
						}
						case 3:{
							if (InBoard(startX,startY)&&
							board.table[startX][startY]==0)
								miniScore+=h3;
							else miniScore+=m3;
							break;
						}
						case 2:{
							if (InBoard(startX,startY)&&
							board.table[startX][startY]==0)
								miniScore+=h2;
							else miniScore+=m2;
							break;
						}
					}
				}
			}
		}
	}
	return maxiScore-miniScore;
}
*/


void PrintTable(void)
{
	putimage(0,0,&back_pic);
	setlinecolor(BROWN);
	for (int i=1;i<=LENGTH;i++){
		line(dis,i*dis,LENGTH*dis,i*dis);
		line(i*dis,dis,i*dis,LENGTH*dis);
	}
}

void GetMode(void)
{
	printf("Enter your favorite chess color:\n"
	"b(black) or w(white)\n");
	char ch=getchar();getchar();
	if (ch=='B'||ch=='W') ch=ch-'A'+'a';
	while (ch!='b'&&ch!='w'){
		printf("Your color isn't exist !\n"
		"Try again.\n");
		ch=getchar();getchar();
	}
	PlayerChess=(ch=='w');		//默认为白色 
	printf("Do you need to move first?[Y/N]\n");
	ch=getchar();getchar();
	while (ch!='Y'&&ch!='N'&&ch!='y'&&ch!='n'){
	printf("Your answer is invalid !\n"
	"Try again.\n");
	ch=getchar();getchar();
	}
	move_first=(ch=='Y'||ch=='y')?true:false;
}

bool MatchClickToBoard(short x,short y,int *X,int *Y)
{
	int MatchSuccess=0;
	for (int i=1;i<=LENGTH;i++){
		if (abs(x-i*dis)<dis/3){
			MatchSuccess++;
			*X=i-1;
			break;
		}
	}
	for (int i=1;i<=LENGTH;i++){
		if (abs(y-i*dis)<dis/3){
			MatchSuccess++;
			*Y=i-1;
			break;
		}
	}
	if (MatchSuccess==2) return true;
	else return false;
}


void GetPlayerChess(T board,int *x,int *y)
{
	printf("Place click to place your next move.\n");
	MOUSEMSG Msg;
	while (1){
		Msg=GetMouseMsg();
		if (Msg.mkLButton){
			int X,Y;
			bool success=MatchClickToBoard(Msg.x,Msg.y,&X,&Y);
			if (success&&InBoard(X,Y)&&board.table[X][Y]==0){
				*x=X,*y=Y;
				break;
			}
			else {
				printf("Your next move is invalid!\n"
				"Try again.\n");
			}
		}
	}
	printf("Got your click!\n");
}

void PrintChess(int x,int y,bool white)
{
	int X=(x+1)*dis-dis/3,
		Y=(y+1)*dis-dis/3;
	if (white) putimage(X,Y,&whitechess);
	else putimage(X,Y,&blackchess);
}


void Ending(int result)
{
	IMAGE end;
	if (result==1){
		loadimage(&end,"congratulations.jpg",250,200);
	}
	else {
		loadimage(&end,"loss.jpg",250,200);
	}
	putimage(125,150,&end);
}




int main(void)
{
	initgraph(500,500,SHOWCONSOLE);
	loadimage(&back_pic,"background_picture(1).jpg",500,500);
	loadimage(&whitechess,"whitechess.jpg",2*dis/3,2*dis/3);
	loadimage(&blackchess,"blackchess.jpg",2*dis/3,2*dis/3);
	PrintTable();
	GetMode();
	bool AI_first=false;
	if (move_first==false) AI_first==true; 
	T current;
	int result;
	for (int i=0;i<LENGTH;i++){
		for (int j=0;j<LENGTH;j++){
			current.table[i][j]=0;
		}
	}
	

	while (1){
		if (move_first){
			int x,y;
			move_first=false;
			GetPlayerChess(current,&x,&y);
			current.table[x][y]=1;
			PrintChess(x,y,PlayerChess);
		}
		else {
			int x,y;
			move_first=true;
			if (AI_first){
				AI_first=false;
				x=LENGTH/2,y=LENGTH/2;
			}
			else GetAIChess(current,&x,&y);
			current.table[x][y]=-1;
			PrintChess(x,y,!PlayerChess);
			printf("Opponent's chess placed.\n");
		}
		if (result=IsGameOver(current)) break;
	}
	Ending(result);
	Sleep(5000);
	return 0;
}












