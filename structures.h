#include<stdio.h>
//#include "linked_list.h"
/*Different types of user requests user makes*/
typedef enum request{
	MSG, LIKE, UNLIKE, JOIN, LEAVE, HISTORY, VIEW
}request;

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
/*Lamport Timestamp*/
typedef struct LTS{
	int LTS_counter;
	int LTS_server_id;

}LTS;

/*Update structure for like/unlike a line number*/
typedef struct like_packet{

	int like_packet_line_no;
	struct LTS like_packet_line_no_lts;
	char like_packet_user[80];
}like_packet;

/*Update structure for joining/leaving a packet*/
typedef struct join_packet{

	char *join_packet_user;
}join_packet;

/*Line packet which consists of the portion of the packet that needs to be sent
 * to the clients*/
typedef struct line_packet{

	char line_packet_user[80];
	char line_packet_message[80];
	int line_packet_likes;
	int line_packet_line_no;
	struct LTS line_packet_lts;
}line_packet;




typedef union union_update_data{
	struct line_packet data_line;
	struct like_packet data_like;
	struct join_packet data_join;

}union_update_data;


/*Update structure, for all kinds of updates*/
typedef struct update{

	request update_type;
	struct LTS update_lts;
	char update_chat_room[80];
	union union_update_data update_data;

	
}update;


/*Structure of message/line. We maintain a linkedlist of these 
 * per chat group*/
typedef struct line{

	struct line_packet line_content;
	linked_list* line_meta;
}line;

/*Data to be inserted into line descriptor node*/
typedef struct meta{

	//user ID of user who has liked a particular message
	char meta_user[80];

}meta;








