#include "structures.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include "net_include.h"

/*Allocate memory for a node in the list*/
node* get_node(list_type type){

	void *data;
	node* linked_list_node;
	switch(type){

		case LIST_UPDATE:data= (update*)malloc(sizeof(update));

				 break;
		case LIST_META:	data= (meta*)malloc(sizeof(meta));

				break;
		case LIST_LINE:	data= (line*)malloc(sizeof(line));

				break;

	}

	linked_list_node = (node*)malloc(sizeof(node));
	linked_list_node->data=data; //i think this should be memcpy
	linked_list_node->next=NULL;
	return linked_list_node;
}

/*Returns a linkedlist of a specified type*/
linked_list* get_linked_list(list_type type){
	linked_list *new_list= 	malloc(sizeof(linked_list));
	new_list->head=NULL;
	new_list->tail=NULL;
	new_list->linked_list_type=type;
	return new_list;
}

/*Appends a node to the end of a linkedlist*/
void append(linked_list *list, node* new_node){

	printf("\nEntered Append\n");
	if(is_empty(list)){
		list->head=new_node;


	}
	else{


		list->tail->next=new_node;
	}
	list->tail=new_node;

	printf("\nLeaving append\n");
}

/*Checks if a list is empty*/
int is_empty(linked_list *list){

	printf("\nIn is empty\n");
	if (list->head==NULL)
		return 1;
	else
		return 0;

}

/*Create a node of type meta*/
node* create_meta(char* user_name){

	printf("\n entered create_meta");
	node *new_node=get_node(LIST_META);	
	meta* my_meta;
	my_meta=(meta*)new_node->data;
	sprintf(my_meta->meta_user,user_name);
	return new_node;

}

/*Creates a node which is of type update for the update list and returns that node*/
node* create_update( request update_type, LTS lts, char* chat_room, union_update_data data){

	static int i=2;
	node *new_node=get_node(LIST_UPDATE);	
	update* my_data=(update*)new_node->data;
	sprintf(my_data->update_chat_room,chat_room);
	my_data->update_type=update_type;
	my_data->update_lts.LTS_counter= i;//lts.LTS_counter;
	i++;
	my_data->update_lts.LTS_server_id=lts.LTS_server_id;

	//allocate space for the union 
	//union_update_data* union_data = (union_update_data*)malloc(sizeof(union_update_data));

	// this will be switched by type
	memcpy(&(my_data->update_data.data_like),&data,sizeof(union_update_data));
	
	//my_data->update_data
	return new_node;


}

/*Allocates space for a line by calling getnode and assigns the values
 * specific to that line*/
node* create_line(char* user, char* message, int likes, int line_no, LTS lts){

	static int i=1;
	node *new_node=get_node(LIST_LINE);
	line* my_data=(line*)new_node->data;
	sprintf(my_data->line_content.line_packet_user,user);
	sprintf(my_data->line_content.line_packet_message,message);
	my_data->line_content.line_packet_likes=likes;
	my_data->line_content.line_packet_line_no=line_no;
	my_data->line_content.line_packet_lts.LTS_counter=i;//lts.LTS_counter;
	i++;
	my_data->line_content.line_packet_lts.LTS_server_id=lts.LTS_server_id;

	linked_list* meta_ll=get_linked_list(LIST_META);
	my_data->line_meta=meta_ll;
	return new_node;


}

/*Seek for a location in the list, where a node can be inserted*/
node* seek(linked_list *list, LTS lts){

	node* prev=list->head;
	node* temp=prev;
	while(temp->next!=NULL){

	
		switch(list->linked_list_type){
		
			case LIST_LINE:
				;
				line* my_data;
				my_data=(line*)temp->data;
				if(my_data->line_content.line_packet_lts.LTS_counter>lts.LTS_counter){
			
					//check if this is the first node
					if(prev==list->head){
						return NULL;
					} 
					else{
						return prev;	
					}
				}
				else if(my_data->line_content.line_packet_lts.LTS_counter == lts.LTS_counter){

					if(my_data->line_content.line_packet_lts.LTS_server_id > lts.LTS_server_id){
	
					if(prev==list->head){
						return NULL;
					}
				}
				else{
					return prev;
				}
		}
				break;
		}
		prev= temp;
		temp=temp->next;

	}
	return list->tail;

}
