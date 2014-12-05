#include "/home/cs437/exercises/ex3/include/sp.h"



#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>

#include <errno.h>
#include <time.h>

#define PORT         10150


#define MAX_MESS_LEN 1400
#define SPREAD_PORT "10150"
#define MAX_VSSETS      10
#define MAX_MEMBERS     100
#define SIZE 80
#define LINES_ON_SCREEN 3



#define BUF_SIZE 6500

/*Spread USER*/
//#define SPREAD_USER ""
#define GROUP "rdeshpa33amehta26"

#define SERVER_GRP "server_grp"

//#include "linked_list.h"
/*Different types of user requests user makes*/
typedef enum request{
	MSG, LIKE, UNLIKE, JOIN, LEAVE, HISTORY, VIEW,CONNECT, DISCONNECT
}request;

typedef enum response{
	R_ACK=10, R_MSG, R_HISTORY, R_VIEW, R_JOIN, R_LEAVE
}response;

typedef enum list_type{
	LIST_UPDATE=20, LIST_LINE, LIST_META ,LIST_CHATROOM
}list_type;

typedef enum client_states{
	NOT_CONNECTED=30, LOGGED_IN, IN_CHATROOM, LOG_CONN,CONNECTED 
}client_states;


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

	//int like_packet_line_no;
	struct LTS like_packet_line_no_lts;
	char like_packet_user[20];
}like_packet;

/*Update structure for joining/leaving a packet*/
typedef struct join_packet{

	char join_packet_user[20];
}join_packet;

/*Line packet which consists of the portion of the packet that needs to be sent
 * to the clients*/
typedef struct line_packet{

	char line_packet_user[20];
	char line_packet_message[80];
	int line_packet_likes;
	//int line_packet_line_no;
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
	char update_chat_room[SIZE];
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
	char meta_user[20];
	int cnt; //this will be zero when list indicates users who liked something

}meta;

typedef struct chatroom{

	char chatroom_name[SIZE];
	linked_list *chatroom_msgs;
	linked_list *users;
	int counter;
	node* start;
}chatroom;



/*My variables data structure client*/
typedef struct my_variables_client{

	int machine_id;
	int my_ip;
	sp_time timeout;
	char private_group[80];
	char username[20];
	//char my_chatroom[SIZE];
	chatroom *my_chatroom;
	//linked_list *users_in_room;
	//char my_server[SIZE];
	int my_server;
	client_states my_state;
}client_variables;

/*My variables data structure server*/
typedef struct my_variables_server{

	int machine_id;
	int my_ip;
	sp_time timeout;
	char private_group[80];
	linked_list *chat_lists;
	LTS my_lts;
	linked_list *update_list;
	linked_list *undelivered_update_list;
	LTS my_vector[5];
}server_variables;


typedef struct request_packet{
	request request_packet_type;
	LTS request_packet_lts;
	char request_packet_data[SIZE];
	char request_packet_user[SIZE];
	char request_packet_chatroom[SIZE];

}request_packet;

typedef union response_data{

	line_packet line;
	int server_list[6];
	char users[50][20];

}response_data;


typedef struct response_packet{
	response response_packet_type;
	response_data data;

}response_packet;


node* get_node(list_type);
linked_list* get_linked_list(list_type);
node* create_meta(char* meta);
void test_meta_list();
void append(linked_list*, node*);
node* create_line(char* user, char* message, int likes, LTS lts);

LTS create_LTS(int counter, int server_id);

like_packet create_like_packet(LTS lts, char* user);
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

node* seek_chatroom(linked_list *list,char* name,int *ret_val);

void print_line(linked_list *line_ll);


node* create_line(char* user, char* message, int likes,LTS lts);

node* create_chatroom(char*  name);

