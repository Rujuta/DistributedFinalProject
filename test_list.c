#include<stdio.h>
//#include "linked_list.h"
#include "structures.h"
#include <string.h>

/*node* get_node(list_type);
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
void delete(linked_list *list,node* location);
void insert(linked_list *list, node* new_node, node* location);
node* seek_user(linked_list *list, char* user,int *ret_val);
node* seek(linked_list *list, LTS lts , int *ret_val);
*/
int main(){

	//test_meta_list();
//	test_update_list();
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

	int ret_val;
	node* seeked= seek_user(meta_ll,user_list[3], &ret_val); //search for first item
	/*Print the list*/
	print_meta(meta_ll);
	//if()
	if(seeked==NULL)
			printf("Duh");
	printf("\nretval : %d\n",ret_val);
	if(ret_val==1){
		if(seeked==NULL)
			printf("Duh");
		delete(meta_ll,seeked);
	}
	print_meta(meta_ll);
}

/*like_packet create_like_packet(int line_no, LTS lts, char* user){

	like_packet like_pkt;
	like_pkt.like_packet_line_no=line_no;
	like_pkt.like_packet_line_no_lts.LTS_counter=lts.LTS_counter;
	like_pkt.like_packet_line_no_lts.LTS_server_id=lts.LTS_server_id;
	strcpy(like_pkt.like_packet_user,user);
	return like_pkt;

}*/


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
	union_update_data line_pkt;
	
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
	linked_list *line_ll, *line_u;
	line_ll=(linked_list*)get_linked_list(LIST_LINE);
	line_u=(linked_list*)get_linked_list(LIST_UPDATE);
	int i=1;
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
	/*Create list of like updates*/
	int updatelts=6;
	while(i<6){

		LTS lts=create_LTS(i,3);
		node_line=(node*)create_line("Sponge Bob","This is my first message",4,1,lts);
		append(line_ll,node_line);
		LTS ults= create_LTS(updatelts,3);
		line* lp= (line*)node_line->data;
		union_update_data up;
		up.data_line=lp->line_content;
		node* new_update=create_update(MSG,ults,"chatroom1",up);
		append(line_u,new_update);
		i++;
		updatelts++;
	}

	print_line(line_ll);
 	
	print_update(line_u);
	
	//print_update(line_u);
	/*test seek function*/
	int ret_val;
	line * my_data;
	node* seeked_node=(node*)seek(line_ll,create_LTS(5,1),&ret_val);
	
	node *  nn;

	



	if(seeked_node == NULL){
		printf("\nreturned null by seek --ret val = %d\n",ret_val);
	}else{
	my_data=(line*)seeked_node->data;
	printf("\n\nSeeked LTS counter:%d Seeked LTS server ID:%d and the ret val is %d",my_data->line_content.line_packet_lts.LTS_counter,my_data->line_content.line_packet_lts.LTS_server_id,ret_val);
 	}
	
	 nn=(node*)create_line("Square Pants","This is my new line message",6,1,create_LTS(5,1));

	insert(line_ll,nn,seeked_node);
	
	print_line(line_ll);

	seeked_node=(node*)seek(line_ll,create_LTS(5,3),&ret_val);
	

	 if(seeked_node == NULL){
                printf("\nreturned null by seek --ret val = %d\n",ret_val);
        }else{
        my_data=(line*)seeked_node->data;
        printf("\n\nSeeked LTS counter:%d Seeked LTS server ID:%d and the ret val is %d",my_data->line_content.line_packet_lts.LTS_counter,my_data->line_content.line_packet_lts.LTS_server_id,ret_val);
	}

         nn=(node*)create_line("Square Pants","This is my new line message",6,1,create_LTS(5,3));

        //insert(line_ll,nn,seeked_node);

        print_line(line_ll);


 	seeked_node=(node*)seek(line_ll,create_LTS(5,4),&ret_val);
       
	 if(seeked_node == NULL){
                printf("\nreturned null by seek --ret val = %d\n",ret_val);
        }else{
	 my_data=(line*)seeked_node->data;
         printf("\n\nSeeked LTS counter:%d Seeked LTS server ID:%d and the ret val is %d",my_data->line_content.line_packet_lts.LTS_counter,my_data->line_content.line_packet_lts.LTS_server_id,ret_val);
	}

         nn=(node*)create_line("Square Pants","This is my new line message",6,1,create_LTS(5,4));

        insert(line_ll,nn,seeked_node);

        print_line(line_ll);

	
	seeked_node=(node*)seek(line_ll,create_LTS(6,4),&ret_val);

         if(seeked_node == NULL){
                printf("\nreturned null by seek --ret val = %d\n",ret_val);
        }else{
         my_data=(line*)seeked_node->data;
         printf("\n\nSeeked LTS counter:%d Seeked LTS server ID:%d and the ret val is %d",my_data->line_content.line_packet_lts.LTS_counter,my_data->line_content.line_packet_lts.LTS_server_id,ret_val);
        }

         nn=(node*)create_line("Square Pants","This is my new line message",6,1,create_LTS(6,4));

        insert(line_ll,nn,seeked_node);

        print_line(line_ll);


	printf("\n\n-------Testing update -------\n\n");	
	
	/*Seek for something in Update*/
	node* s ;
	node * ns;
	line* lp= (line*)nn->data;

	update * my_data2;
	union_update_data up;
        up.data_line=lp->line_content;



	s=(node*)seek(line_u,create_LTS(6,2),&ret_val);

         if(s== NULL){
                printf("\nreturned null by seek --ret val = %d\n",ret_val);
        }else{
         my_data2=(update*)s->data;
         printf("\n\nSeeked LTS counter:%d Seeked LTS server ID:%d and the ret val is %d",my_data2->update_lts.LTS_counter,my_data2->update_lts.LTS_server_id,ret_val);
        }


	ns=create_update(MSG,create_LTS(6,2),"chatroom2",up);
	insert(line_u,ns,s);
	print_update(line_u);


	 s=(node*)seek(line_u,create_LTS(6,3),&ret_val);

         if(s== NULL){
                printf("\nreturned null by seek --ret val = %d\n",ret_val);
        }else{
         my_data2=(update*)s->data;
         printf("\n\nSeeked LTS counter:%d Seeked LTS server ID:%d and the ret val is %d",my_data2->update_lts.LTS_counter,my_data2->update_lts.LTS_server_id,ret_val);
        }

	 ns=create_update(MSG,create_LTS(6,2),"chatroom2",up);
       // insert(line_u,ns,s);
        print_update(line_u);

	

	 s=(node*)seek(line_u,create_LTS(6,4),&ret_val);

         if(s== NULL){
                printf("\nreturned null by seek --ret val = %d\n",ret_val);
        }else{
         my_data2=(update*)s->data;
         printf("\n\nSeeked LTS counter:%d Seeked LTS server ID:%d and the ret val is %d",my_data2->update_lts.LTS_counter,my_data2->update_lts.LTS_server_id,ret_val);
        }

	 ns=create_update(MSG,create_LTS(6,4),"chatroom2",up);
        insert(line_u,ns,s);
        print_update(line_u);



	 s=(node*)seek(line_u,create_LTS(5,4),&ret_val);

         if(s== NULL){
                printf("\nreturned null by seek --ret val = %d\n",ret_val);
        }else{
         my_data2=(update*)s->data;
         printf("\n\nSeeked LTS counter:%d Seeked LTS server ID:%d and the ret val is %d",my_data2->update_lts.LTS_counter,my_data2->update_lts.LTS_server_id,ret_val);
        }

	 ns=create_update(MSG,create_LTS(5,4),"chatroom2",up);
        insert(line_u,ns,s);
        print_update(line_u);
	

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
		
		printf("\nUpdate LTS counter:%d Server ID:%d",my_data2->update_lts.LTS_counter,my_data2->update_lts.LTS_server_id);
		printf("\nUpdate is in chat room:%s",my_data2->update_chat_room);
		printf("\nUpdate line:%d",my_data2->update_data.data_like.like_packet_line_no);
				
		switch(my_data2->update_type){
			case LIKE:
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
		//printf("People who liked this:");
		//print_meta(my_data2->line_meta);
		temp=temp->next;
	}
}


