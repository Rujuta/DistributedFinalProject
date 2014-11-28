#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef enum list_type{
	LIST_UPDATE, LIST_LINE, LIST_META 
}list_type;


/*Declarations of various types of nodes here*/
typedef struct node{
	
	void* data;
	struct node* next;
}node;

typedef struct linked_list{
	node* head;
	node* tail;
	list_type linked_list_type;
}linked_list;
