#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "AVLtrees.c"
#include "File.c"
AVLnode *inputData(AVLnode *root)
{
    printf("请按照“四位数ID,姓名,成绩”格式输入，\n"
           "一个同学的一行，\n"
           "最后以单独一行\"end\"结束输入。\n");
    
    char buffer[100];
    Student person;
    
    while (1) {
        // 使用fgets读取整行
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break; // 读取失败或EOF
        }
        
        // 移除换行符
        buffer[strcspn(buffer, "\n")] = '\0';
        
        // 检查是否输入了"end"
        if (strcmp(buffer, "end") == 0) {
            break;
        }
        
        // 解析输入
        if (sscanf(buffer, "%d,%49[^,],%d", 
                   &person.id, person.name, &person.score) == 3) {
            root = insertNode(root, person);
            printf("成功添加学生: ID=%d, 姓名=%s, 成绩=%d\n", 
                   person.id, person.name, person.score);
        } else {
            printf("输入格式错误，请按照\"ID,姓名,成绩\"格式重新输入: \n");
        }
    }
    
    printf("输入完成。\n");
    return root;
}

AVLnode *FindData(AVLnode *root)
{
	while (1){
		printf("请输入学生ID，或者输入end结束查找：\n");
		char buffer[100];
		int num;
		if (fgets(buffer,sizeof(buffer),stdin)==NULL){
			printf("读取错误！请重新输入：\n");
			continue;
		}
		buffer[strcspn(buffer,"\n")]='\0';
		if (strcmp(buffer,"end")==0) break;
		sscanf(buffer,"%d",&num);
		AVLnode *MinNode=NULL,*MaxNode=NULL;
		searchNode(root,num,&MaxNode,&MinNode);
		if (MaxNode!=NULL&&num==MaxNode->data.id){
			Student Max=MaxNode->data;
			printf("该学生的信息为：\
			\nID:%d  姓名:%s  成绩:%d\n",Max.id,Max.name,Max.score);
		}
		else {
			printf("未找到该同学。\n");
			if (MaxNode!=NULL||MinNode!=NULL){
			printf("您或许在找以下ID临近的同学：\n");
				if (MinNode!=NULL){
					Student Min=MinNode->data;
					printf("ID:%4d\t姓名:%s\t成绩:%d\n",Min.id,Min.name,Min.score);
				}
				if (MaxNode!=NULL){
					Student Max=MaxNode->data;
					printf("ID:%4d\t姓名:%s\t成绩:%d\n",Max.id,Max.name,Max.score);
				}
			}
			else 
				printf("系统未存储学生信息。\n");
		}
	}
	printf("查找完成。\n");
	return root;
}

AVLnode *deleteData(AVLnode *root)
{
	while (1){
		printf("输入您需要删除学生信息对应的ID，\
		\n或者输入end结束删除操作：\n");
		char buffer[100];
		if (fgets(buffer,sizeof(buffer),stdin)==NULL){
			printf("读取错误！请重新输入：\n");
			continue;
		}
		buffer[strcspn(buffer,"\n")]='\0';
		if (strcmp(buffer,"end")==0){
			break;
		}
		int del;
		sscanf(buffer,"%d",&del);
		AVLnode *MaxNode=NULL,*MinNode=NULL;
		searchNode(root,del,&MaxNode,&MinNode);
		if (MaxNode!=NULL&&MaxNode->data.id==del){
			root=deleteNode(root,del);
			printf("找到并成功删除。\n");
		}
		else {
			printf("该学生的信息不在其中，无法删除。\n");
		}
	}
	printf("删除完成。\n");
	return root;
}

int main(void)
{
	AVLnode *root=NULL;
	bool saveDocument=false;
	char input;
	printf("欢迎使用学生成绩管理系统。\n");
	while (1){
		bool end=false;
		printf("您现在需要进行什么操作？\
		\nA.输入学生成绩  B.查找学生成绩\
		\nC.删除学生成绩  D.打印学生成绩\
		\nE.以txt格式输出学生成绩  F.结束操作\n");
		scanf("%c",&input);getchar();
		if (input>='a'&&input<='z') input=input-'a'+'A';
		if (input<'A'||input>'Z'){
			printf("输入有误，请重新输入。\n");
			continue;
		} 
		switch (input){
			case 'A':{
				root=inputData(root);
				break;
			}
			case 'B':{
				root=FindData(root);
				break;
			}
			case 'C':{
				root=deleteData(root);
				break;
			}
			case 'D':{
				inOrderTraversal(root);
				break;
			}
			case 'E':{
				outputData(root);
				break;
			}
			case 'F':{
				end=true;
				break;
			}
		}
		if (end) break;
	}
	printf("感谢使用！\n");
	return 0;
}
