#include<stdio.h>
#include<stdlib.h>
#include<string.h>


/*Declarations of various types of nodes here*/
typedef struct node{
	
	void* data;
	node* next;
}node;

typedef struct linked_list{
	node* head;
	node* tail;
	list_type linked_list_type;
}linked_list;
