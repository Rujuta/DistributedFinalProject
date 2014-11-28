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

	node *new_node=get_node(LIST_UPDATE);	
	update* my_data=(update*)new_node->data;
	sprintf(my_data->update_chat_room,chat_room);
	my_data->update_type=update_type;
	my_data->update_lts.LTS_counter=lts.LTS_counter;
	my_data->update_lts.LTS_server_id=lts.LTS_server_id;

	//allocate space for the union 
	//union_update_data* union_data = (union_update_data*)malloc(sizeof(union_update_data));

	// this will be switched by type //TRY without specifying union type once
	memcpy(&(my_data->update_data),&data,sizeof(union_update_data));

	//my_data->update_data
	return new_node;


}

/*Allocates space for a line by calling getnode and assigns the values
 * specific to that line*/
node* create_line(char* user, char* message, int likes, int line_no, LTS lts){

	node *new_node=get_node(LIST_LINE);
	line* my_data=(line*)new_node->data;
	sprintf(my_data->line_content.line_packet_user,user);
	sprintf(my_data->line_content.line_packet_message,message);
	my_data->line_content.line_packet_likes=likes;
	my_data->line_content.line_packet_line_no=line_no;
	my_data->line_content.line_packet_lts.LTS_counter=lts.LTS_counter;
	my_data->line_content.line_packet_lts.LTS_server_id=lts.LTS_server_id;

	linked_list* meta_ll=get_linked_list(LIST_META);
	my_data->line_meta=meta_ll;
	return new_node;


}



/*Seek for a location in the list, where a node can be inserted
 * Seek returns node after which insertion is to be done*/
node* seek(linked_list *list, LTS lts , int *ret_val){

	//we set ret_val =1 when value is found else its 0
	printf("\n In seek\n");
	printf("\nLTS server id :%d",lts.LTS_server_id);
	printf("\nLTS counter id :%d",lts.LTS_counter);
node* prev=list->head;
	node* temp=prev;
	*ret_val=0;
	while(temp->next!=NULL){


		switch(list->linked_list_type){

			case LIST_LINE:
				;
				line* my_data;
				my_data=(line*)temp->data;
				printf("\nline LTS counter id :%d",my_data->line_content.line_packet_lts.LTS_counter);
				printf("\nline LTS server id :%d",my_data->line_content.line_packet_lts.LTS_server_id);
	
				if(my_data->line_content.line_packet_lts.LTS_counter>lts.LTS_counter){

					//check if this is the first node
					if(temp==list->head){
						return NULL;
					}
					else{
						return prev;	
					}
				}
				else if(my_data->line_content.line_packet_lts.LTS_counter == lts.LTS_counter){

					printf("\nLine's server id %d",my_data->line_content.line_packet_lts.LTS_server_id);
					if(my_data->line_content.line_packet_lts.LTS_server_id > lts.LTS_server_id){

						printf("\n Server id greater\n");
						if(temp==list->head){
							return NULL;
						}
						else{
							return prev;
						}
					}
					else{
						if(my_data->line_content.line_packet_lts.LTS_server_id == lts.LTS_server_id){

							*ret_val=1;
						}
						return prev;
					}
				}
				break;


			case LIST_UPDATE:
				;
				update* my_data2;
				my_data2=(update*)temp->data;
				if(my_data2->update_lts.LTS_counter>lts.LTS_counter){

					//check if this is the first node
					if(temp==list->head){
						return NULL;
					}
					else{
						return prev;	
					}
				}
				else if(my_data2->update_lts.LTS_counter == lts.LTS_counter){

					if(my_data2->update_lts.LTS_server_id > lts.LTS_server_id){

						if(temp==list->head){
							return NULL;
						}
						else{
							return prev;
						}
					}
					else{
						if(my_data2->update_lts.LTS_server_id == lts.LTS_server_id){

							*ret_val=1;
						}
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

node* seek_user(linked_list *list, char* user,int *ret_val){


	printf("Entering Seek");
	node* prev=NULL;
	node* temp= list->head;
	*ret_val =0;
	while(temp!=NULL){

		meta* my_data = (meta*) temp->data;

		if(strcmp(user, my_data->meta_user)==0){ //user exists

			*ret_val=1;

			return prev;
		}
		else{

			prev= temp;
			temp=temp->next;

		}

	}
}

/*insert into chat group msgs*/
void insert(linked_list *list, node* new_node, node* location){


	if(location==NULL){ //insert as head node

		new_node->next=list->head;
		if(list->tail==NULL){
			list->tail=new_node;
		}
		list->head=new_node;
	}
	else{
		new_node->next=location->next;
		if(list->tail==location){
			list->tail=new_node;	
		}
		location->next=new_node;
	}

}


/*Delete from a list on being passed the location after which is to be deleted*/
void delete(linked_list *list,node* location){

	printf("\n In delete\n");
	if(location==NULL){ //delete head node

		list->head=list->head->next;
		if(list->head==NULL){
			list->tail=NULL;
		}
	}
	else if(location->next->next==NULL){ //insert as head node

		location->next=NULL;
		list->tail=location;


	}
	else{
	
		location->next=location->next->next;
	}
	

}

like_packet create_like_packet(int line_no, LTS lts, char* user){

	like_packet like_pkt;
	like_pkt.like_packet_line_no=line_no;
	like_pkt.like_packet_line_no_lts.LTS_counter=lts.LTS_counter;
	like_pkt.like_packet_line_no_lts.LTS_server_id=lts.LTS_server_id;
	strcpy(like_pkt.like_packet_user,user);
	return like_pkt;

}

/*update* create_like_packet(int line_no, LTS lts, char* user){

	like_packet like_pkt;
	like_pkt.like_packet_line_no=line_no;
	like_pkt.like_packet_line_no_lts.LTS_counter=lts.LTS_counter;
	like_pkt.like_packet_line_no_lts.LTS_server_id=lts.LTS_server_id;
	strcpy(like_pkt.like_packet_user,user);
	return like_pkt;

}
*/

