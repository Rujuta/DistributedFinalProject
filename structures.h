#include<stdio.h>

/*Different types of user requests user makes*/
typedef enum request{
	MSG, LIKE, UNLIKE, JOIN, LEAVE, HISTORY, VIEW
}request;

typedef enum list_type{
	LIST_UPDATE, LIST_LINE, LIST_META 
}list_type;


/*Update structure, for all kinds of updates*/
typedef struct update{

	request update_type;
	LTS update_lts;
	char* update_chat_room;
	union_update_data update_data;

	
}update;

typedef union union_update_data{
	line_packet data_line;
	like_unlike_packet data_like;
	join_leave_packet data_join;

}union_update_data;

/*Update structure for like/unlike a line number*/
typedef struct like_packet{

	int like_packet_line_no;
	LTS like_packet_line_no_lts;
	char *like_packet_user;
}like_packet;

/*Update structure for joining/leaving a packet*/
typedef struct join_packet{

	char *join_packet_user;
}join_packet;

/*Lamport Timestamp*/
typedef struct LTS{
	int LTS_counter;
	int LTS_server_id;

}LTS;

/*Structure of message/line. We maintain a linkedlist of these 
 * per chat group*/
typedef struct line{

	line_packet line_content;
	linked_list* line_meta;
}line;

/*Line packet which consists of the portion of the packet that needs to be sent
 * to the clients*/
typedef struct line_packet{

	char* line_packet_user;
	char line_packet_message[80];
	int line_packet_likes;
	int line_packet_line_no;
	LTS line_packet_lts;
}line_packet;

/*Data to be inserted into line descriptor node*/
typedef struct meta{

	//user ID of user who has liked a particular message
	char* meta_user;

}meta;








