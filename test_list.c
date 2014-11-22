#include<stdio.h>
//#include "linked_list.h"
#include "structures.h"
#include <string.h>

node* get_node(list_type);
linked_list* get_linked_list(list_type);
node* create_meta(char* meta);
void test_meta_list();
void append(linked_list*, node*);
node* create_line(char* user, char* message, int likes, int line_no, LTS lts);

LTS create_LTS(int counter, int server_id);

like_packet create_like_packet(int line_no, LTS lts, char* user);
void print_meta(linked_list *meta_ll);

void test_update_list();
node* create_update( request update_type, LTS lts, char* chat_room, union_update_data data);

void print_update(linked_list *update_ll);

void print_line(linked_list *line_ll);

void test_line_list();
node* seek(linked_list*, LTS);

int main(){

	test_meta_list();
	test_update_list();
	test_line_list();
	return 0;


}


void test_meta_list(){

	printf("\n In test_meta_list\n");	
	linked_list* meta_ll;
	//put 6 elements in meta_ll
	char* user_list[]={"johhny","dexter","captain","jane","marvin","peanuts"};
	int i=0;
	node* temp;
	/*Create list*/
	meta_ll=(linked_list*)get_linked_list(LIST_META);
	printf("List type: %d",meta_ll->linked_list_type);

	/*Add element to list one after another*/
	while(i<6){

		temp=create_meta(user_list[i]);
		append(meta_ll,temp);
		i++;
	}

	/*Print the list*/
	print_meta(meta_ll);
}

like_packet create_like_packet(int line_no, LTS lts, char* user){

	like_packet like_pkt;
	like_pkt.like_packet_line_no=line_no;
	like_pkt.like_packet_line_no_lts.LTS_counter=lts.LTS_counter;
	like_pkt.like_packet_line_no_lts.LTS_server_id=lts.LTS_server_id;
	strcpy(like_pkt.like_packet_user,user);
	return like_pkt;

}
void test_update_list(){


	printf("\n In test_update_list\n");	
	linked_list *update_ll;
	update_ll=(linked_list*)get_linked_list(LIST_UPDATE);
	int i=0;
	int lts_cnt=0;
	node* node_update;
	/*Create LTS*/
	LTS newlts=create_LTS(1,1);

	/*LIKE UPDATE*/
	union_update_data l_packet;
	l_packet.data_like=create_like_packet(12, newlts,"Tom");	

	/*LINE UPDATE*/

	/*JOIN UPDATE*/


	/*LTS of update*/
	LTS lts=create_LTS(34,3);
	/*Create list of like updates*/
	while(i<6){
		node_update=(node*)create_update(LIKE,lts,"chat_room1",l_packet);
		append(update_ll,node_update);
		i++;
	}

	print_update(update_ll);

}

void test_line_list(){

	printf("\n In test_line_list\n");	
	linked_list *line_ll;
	line_ll=(linked_list*)get_linked_list(LIST_LINE);
	int i=0;
	int lts_cnt=0;
	node* node_line;
	/*Create LTS*/
	LTS newlts=create_LTS(1,1);


	/*LIKE UPDATE*/
	//union_update_data l_packet;
	//l_packet.data_like=create_like_packet(12, newlts,"Tom");	

	/*LINE UPDATE*/

	/*JOIN UPDATE*/


	/*LTS of update*/
	LTS lts=create_LTS(34,3);
	/*Create list of like updates*/
	while(i<6){
		node_line=(node*)create_line("Sponge Bob","This is my first message",4,1,lts);
		append(line_ll,node_line);
		i++;
	}

	print_line(line_ll);

	/*test seek function*/
	node* seeked_node=(node*)seek(line_ll,create_LTS(5,3));
	line* my_data=(line*)seeked_node->data;
	printf("\n\nSeeked LTS counter:%d Seeked LTS server ID:%d ",my_data->line_content.line_packet_lts.LTS_counter,my_data->line_content.line_packet_lts.LTS_server_id);


}

LTS create_LTS(int counter, int server_id){

	LTS new_lts;
	new_lts.LTS_counter=counter;
	new_lts.LTS_server_id=server_id;
	return new_lts;
}


void print_meta(linked_list *meta_ll){

	printf("\nIn print meta\n");
	node* temp=meta_ll->head;
	printf("\nYour list is:\n");
	while(temp!=NULL){
		printf("\n%s",temp->data); //wonder how this prints!!!
		temp=temp->next;
	}

}

void print_update(linked_list *update_ll){

	printf("\n In print update list\n");
	node *temp=update_ll->head;
	printf("\nYour list is:\n");
	while(temp!=NULL){
		update* my_data2=(update*)temp->data;

		printf("\nPrinting update msg's data\n");

		switch(my_data2->update_type){
			case LIKE:
				printf("\nUpdate LTS counter:%d Server ID:%d",my_data2->update_lts.LTS_counter,my_data2->update_lts.LTS_server_id);
				printf("\nUpdate is in chat room:%s",my_data2->update_chat_room);
				printf("\nUpdate line:%d",my_data2->update_data.data_like.like_packet_line_no);
				printf("\nUpdate LTS counter :%d, LTS server id: %d",my_data2->update_data.data_like.like_packet_line_no_lts.LTS_counter, my_data2->update_data.data_like.like_packet_line_no_lts.LTS_server_id);
				printf("\nUser sending update:%s",my_data2->update_data.data_like.like_packet_user);
				break;
		}
		temp=temp->next;
	}
}

void print_line(linked_list *line_ll){

	printf("\n In Line  list\n");
	node *temp=line_ll->head;
	printf("\nYour list is:\n");
	while(temp!=NULL){

		line* my_data2=(line*)temp->data;

		printf("\nPrinting line node's data\n");

		printf("LTS %d, %d Line no: %d ",my_data2->line_content.line_packet_lts.LTS_counter, my_data2->line_content.line_packet_lts.LTS_server_id,my_data2->line_content.line_packet_line_no);
		printf("Message: %s",my_data2->line_content.line_packet_message);
		printf("Likes: %d",my_data2->line_content.line_packet_likes);
		printf("People who liked this:");
		print_meta(my_data2->line_meta);
		temp=temp->next;
	}
}


