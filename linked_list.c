#include "linked_list.h"
#include "net_include_common.h"

/*Allocate memory for a node in the list*/
node* get_node(list_type type,void *content){

	void *data=NULL;
	node* linked_list_node = NULL;
	switch(type){

		case LIST_UPDATE:data= (update*)malloc(sizeof(update));

				 break;
		case LIST_META:	data= (meta*)malloc(sizeof(meta));

				break;
		case LIST_LINE:	data= (line*)malloc(sizeof(line));

				break;

	}

	data->next=NULL;
	linked_list_node = malloc(sizeof(node));
	linked_list_node->data=data;
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

/*Allocates space for a line by calling getnode and assigns the values
 * specific to that line*/
node* create_line(char* user, char* message, int likes, int line_no, LTS lts){

	node *new_node=get_node(LIST_LINE);	
	sprintf(new_node->data->line_content.line_packet_user,user);
	sprintf(new_node->data->line_content.message,message);
	new_node->data->line_content.line_packet_likes=likes;
	new_node->data->line_content.line_packet_line_no=line_no;
	new_node->data->line_content.line_packet_LTS.LTS_counter=lts.LTS_counter;
	new_node->data->line_content.line_packet_LTS.LTS_server_id=lts.LTS_server_id;

	linked_list* line_meta=get_linked_list(LIST_META);
	new_node->data->line_meta=line_meta;
	return new_node;


}

/*Appends a node to the end of a linkedlist*/
void append(linked_list *list, node* new_node){

			if(is_empty(list)){
		list->head=new_node;

	}
	else{


		list->tail->next=new_node;
	}
	list->tail=new_node;


}

/*Creates a node which is of type update for the update list and returns that node*/
node* create_update( request update_type, LTS lts, char* chat_room, union_update_data data){

	node *new_node=get_node(LIST_UPDATE);	
	sprintf(new_node->data->update_chat_room,chat_room);
	new_node->data->update_type=update_type;
	new_node->data->update_LTS.LTS_counter=lts.LTS_counter;
	new_node->data->update_LTS.LTS_server_id=lts.LTS_server_id;

	union_update_data* union_data = (union_update_data*)malloc(sizeof(union_update_data));
		
	memcpy(&(union_data),data,sizeof(union_data));
	return new_node;


}

node* create_meta(char* meta){
	
	node *new_node=get_node(LIST_META);	
	sprintf(new_node->data->meta_user,meta);
	return new_node;

}
/*Checks if a list is empty*/
int is_empty(linked_list *list){

	if (list->head!=NULL)
		return 0;
	else
		return 1;

}

/*Deletes a node from the list. Arguments are the list and 
 * the node before the node to be deleted*/
void delete_from_list(linked_list *list, node* new_node){

	node* prev=new_node;
	prev->next=prev->next->next;
	free(new_node);

}




/*This will only be a linkedlist of type meta*/
void delete_meta(linked_list *list,char* username){
	
	node* temp= list->head;
	node* prev=temp;
	while(temp->next!=NULL){
	
			if(strcmp(username,temp->data->meta_user)==0){
				if(temp->next == NULL){
					prev->next == NULL;
					list->tail == prev;
					if(temp == list->head){
						list->head= NULL;
						list->tail=NULL;
					}

				}else{

				prev->next=temp->next;
				if(temp == list->head){
					list->head = temp->next;

				}
				}
				
				free(temp);
				
				break;
			}else{

				prev = temp;
				temp = temp->next;
			}
	
	}
	
}

/*insert into chat group msgs*/
void insert_line(linked_list *list, node* new_node, node* location){

	/*first node insertion*/
	if(location==NULL){
		
		new_node->next=list->head;
		list->head=new_node;
	
	}
	else{
		new_node->next=location->next;
		location->next=new_node;
	}
	if(list->tail==location){
		list->tail=new_node;	
	}

}







