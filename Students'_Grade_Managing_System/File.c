#ifndef FILE_H
#define FILE_H 1

#include <stdio.h>
#include <stdlib.h>

void printData(AVLnode *root, FILE *fp)
{
	if (root==NULL) return ;
	Student stu=root->data;
	printData(root->left, fp);
	fprintf(fp, "%4d %s %d\n", stu.id, stu.name, stu.score);
	printData(root->right, fp);
	return ;
}

void outputData(AVLnode *root)
{
	FILE *fp;
	fp=fopen("StudentData.txt","w");
	if (fp==NULL){
		printf("无法打开文件！\n");
		exit(1);
	}
	printData(root, fp);
	fclose(fp);
	printf("已完成输出。\n");
	fclose(fp);
	return ;
}

#endif
