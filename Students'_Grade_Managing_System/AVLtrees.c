#ifndef AVL_H
#define AVL_H 1
typedef struct Student {
	int id;
	char name[50];
	int score;
} Student;

typedef struct AVLnode {
	Student data;
	int height;
	struct AVLnode *left;
	struct AVLnode *right;
} AVLnode;

int getHeight(AVLnode *root)
{
	if (root==NULL) return -1;
	else {
		int lh=getHeight(root->left);
		int rh=getHeight(root->right);
		return ((lh>=rh)?lh:rh)+1;
	}
}

int getBalance(AVLnode *root)
{
	return getHeight(root->right)-getHeight(root->left);
}

AVLnode *SinRotateLeft(AVLnode *root)
{
	AVLnode *rSon,*lGrnson;
	rSon=root->right;
	lGrnson=rSon->left;
	rSon->left=root;
	root->right=lGrnson;
	root->height=getHeight(root);
	rSon->height=getHeight(rSon);
	return rSon;
}

AVLnode *SinRotateRight(AVLnode *root)
{
	AVLnode *lSon,*rGrnson;
	lSon=root->left;
	rGrnson=lSon->right;
	lSon->right=root;
	root->left=rGrnson;
	root->height=getHeight(root);
	lSon->height=getHeight(lSon);
	return lSon;
}

AVLnode *RotateLeftRight(AVLnode *root)
{
	root->left=SinRotateLeft(root->left);
	return SinRotateRight(root);
}

AVLnode *RotateRightLeft(AVLnode *root)
{
	root->right=SinRotateRight(root->right);
	return SinRotateLeft(root);
}

AVLnode *insertNode(AVLnode *root,Student stu)
{
	if (root==NULL){
		AVLnode *leaf=(AVLnode *)malloc(sizeof(AVLnode));
		root=leaf;
		leaf->data=stu;
		leaf->left=leaf->right=NULL;
		leaf->height=0;
	}
	else {
		if (stu.id<root->data.id){								//往左寻路 
			root->left=insertNode(root->left,stu);
			if (getBalance(root)==-2){
				if (getBalance(root->left)<=0) root=SinRotateRight(root);
				else root=RotateLeftRight(root);
			}
		}
		else if (stu.id>root->data.id){							//往右寻路 
			root->right=insertNode(root->right,stu);
			if (getBalance(root)==2){
				if (getBalance(root->right)>=0) root=SinRotateLeft(root);
				else root=RotateRightLeft(root); 
			}
		}
	}
	root->height=getHeight(root);
	return root;
}

AVLnode *searchMaxNode(AVLnode *root)					//找到直接前驱节点 
{
	if (root->right!=NULL) return searchMaxNode(root->right);
	else return root;
}

AVLnode *deleteNode(AVLnode *root,int id)
{
	if (root==NULL) return NULL;
	else if (id<root->data.id){
		root->left=deleteNode(root->left,id);			//向左寻路并删除
	}
	else if (id>root->data.id){
		root->right=deleteNode(root->right,id);			//向右寻路并删除
	}
	else {												//找到要删除的节点 
		if (root->left==NULL&&root->right==NULL){
			free(root);
			root=NULL;
			return NULL; 
		}
		else if (root->left==NULL){						//只有右节点 
			AVLnode *temp=root;
			root=root->right;
			free(temp);
		}
		else if (root->right==NULL){					//只有左节点 
			AVLnode *temp=root;
			root=root->left;
			free(temp);
		}
		else {											//左右节点均有
			AVLnode *predecessor=searchMaxNode(root->left);
			root->data=predecessor->data;
			root->left=deleteNode(root->left,predecessor->data.id); 	//删除左子树中的前驱节点 
		}
	}
	root->height=getHeight(root);
	if (getBalance(root)==2){
		if (getBalance(root->right)>=0)
			root=SinRotateLeft(root);
		else
			root=RotateRightLeft(root);
	}
	else if (getBalance(root)==-2){
		if (getBalance(root->left)<=0)
			root=SinRotateRight(root);
		else
			root=RotateLeftRight(root);
	}
	root->height=getHeight(root);
	return root;
}

void searchNode(AVLnode *root,int id,AVLnode **MaxNode,AVLnode **MinNode)
{
	if (root==NULL) return;
	else if (root->data.id==id){
		*MaxNode= *MinNode=root;
		return ;
	}
	else if (id<root->data.id){
		*MaxNode=root;
		searchNode(root->left,id,MaxNode,MinNode);
	}
	else {
		*MinNode=root;
		searchNode(root->right,id,MaxNode,MinNode);
		return;
	}
}

void inOrderTraversal(AVLnode *root)				//中序遍历打印 
{
	if (root==NULL) return ;
	else {
		Student info=root->data;
		inOrderTraversal(root->left);
		printf("%8d %s %3d\n",info.id,info.name,info.score);
		inOrderTraversal(root->right);
	}
}

#endif





